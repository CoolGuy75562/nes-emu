#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractTableModel>
#include <qobjectdefs.h>
#include "core/cppwrapper.hpp"
#include "openglwidget.h"

void show_error(QWidget *parent, NESError &e);

namespace Ui {
class MainWindow;
}

class NESTableModel : public QAbstractTableModel {

  Q_OBJECT

public:
  NESTableModel(int rows, int cols, QStringList header_labels, QWidget *parent = nullptr);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

protected:
  void add_state_strings(QStringList &state_strings);
  
  const int rows, cols;
  const QStringList header_labels;
  QList<QStringList> table_data;
};

class CPUTableModel : public NESTableModel {

  Q_OBJECT

public:
  explicit CPUTableModel(QWidget *parent = nullptr);

  void addState(cpu_state_s *cpu_state);

private:
  static const int rows, cols;
  static const QStringList header_labels;
};

class PPUTableModel : public NESTableModel {

  Q_OBJECT

public:
  explicit PPUTableModel(QWidget *parent = nullptr);

  void addState(ppu_state_s *ppu_state);

private:
  static const int rows, cols;
  static const QStringList header_labels;
};

class MemoryTableModel : public NESTableModel {

  Q_OBJECT

public:
  explicit MemoryTableModel(QWidget *parent = nullptr);

  void addState(uint16_t addr, uint8_t val, char r_or_w);

private:
  static const int rows, cols;
  static const QStringList header_labels;
};


class MainWindow : public QMainWindow {
  
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void initOpenGLWidgetNESScreen(NESScreen *s);

public slots:
  void done(void);
  void error(NESError &e);
  
signals:
  void pause_button_clicked();
  void play_button_clicked();
  void step_button_clicked();
  
private:
  void init_table_view(std::string which);

  Ui::MainWindow *ui;
  CPUTableModel *cpu_model;
  PPUTableModel *ppu_model;
  MemoryTableModel *memory_model;
			  
private slots:
  void on_pauseButton_clicked();
  void on_playButton_clicked();
  void on_stepButton_clicked();
  void on_memoryDumpButton_clicked();
  /*
  void refresh_cpu_state();
  void refresh_ppu_state();
  */
};

#endif // MAINWINDOW_H
