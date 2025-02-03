#ifndef NESTABLEMODEL_H_
#define NESTABLEMODEL_H_

#include <QAbstractTableModel>
#include <QWidget>
#include <QQueue>

#include "core/cppwrapper.hpp"

/* Base class for cpu, ppu, memory models which go in the cpu, ppu, memory table
 * views in the main window.
 *
 * Only difference in the child classes is the kind of data to be inserted:
 * cpu_state_s, ppu_state_s, or memory stuff.
 */
template <typename T>
class NESTableModel : public QAbstractTableModel {

  // Q_OBJECT

public:
  NESTableModel(int rows, int cols, QStringList header_labels,
                QWidget *parent = nullptr);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
protected:
  virtual QVariant indexToQString(const QModelIndex &index) const = 0;
  void addState(T&);
  void nes_started();
  void nes_stopped();

  const int rows, cols;

  /* circular buffer of size rows */
  std::vector<T> table_data;
  int table_data_start_idx;
  
private:
  const QStringList header_labels;
  bool nes_running;
  long last_model_update;
};

/*============================================================*/

template <typename T>
NESTableModel<T>::NESTableModel(int rows, int cols, QStringList header_labels,
                             QWidget *parent)
    : QAbstractTableModel(parent), rows(rows), cols(cols),
      header_labels(header_labels) {
  nes_running = false;
  last_model_update = 0;
}

template <typename T>
int NESTableModel<T>::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return rows;
}

template <typename T>
int NESTableModel<T>::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return cols;
}

template <typename T>
QVariant NESTableModel<T>::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() > rows - 1 || index.row() < 0) {
    return QVariant();
  }
  if (index.column() > cols - 1 || index.column() < 0) {
    return QVariant();
  }
  if (role == Qt::DisplayRole) {
    return indexToQString(index);
  }
  return QVariant();
}

template <typename T>
QVariant NESTableModel<T>::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }
  if (orientation != Qt::Horizontal) {
    return QVariant();
  }
  if (section > cols - 1) {
    return QVariant();
  }
  return header_labels.at(section);
}

template <typename T>
void NESTableModel<T>::addState(T &thing) {
  table_data[++table_data_start_idx & (rows - 1)] = thing;
  if (nes_running) {
    if (last_model_update <= 0) {
      last_model_update = rows * 100;
      emit dataChanged(index(0, 0), index(rows - 1, cols - 1));
    } else {
      last_model_update--;
    }
  } else {
    emit dataChanged(index(0, 0), index(rows - 1, cols - 1));
  }
}

template <typename T>
void NESTableModel<T>::nes_started() {
  nes_running = true;
  last_model_update = 0;
}

template <typename T>
void NESTableModel<T>::nes_stopped() {
  nes_running = false;
  emit dataChanged(index(0, 0), index(rows - 1, cols - 1));
}

#endif
