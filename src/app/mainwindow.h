#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/cppwrapper.hpp"
#include "nescontext.h"
#include <QAbstractTableModel>
#include <QMainWindow>

void show_error(QWidget *parent, NESError &e);

namespace Ui {
class MainWindow;
}

/* this class sends signals with the arguments of the cpu, ppu, memory callbacks
   in the nes thread to the memory, cpu, ppu models in the gui thread. */
class NESCallbackForwarder : public QObject {

  Q_OBJECT

public:
  explicit NESCallbackForwarder(QObject *parent = nullptr) : QObject(parent) {}

signals:
  void cpu_state_update(cpu_state_s *state);
  void ppu_state_update(ppu_state_s *state);
  void memory_update(uint16_t addr, uint8_t val, char r_or_w);
};

/* Base class for cpu, ppu, memory models which go in the cpu, ppu, memory table
 * views in the main window.
 *
 * Only difference in the child classes is the kind of data to be inserted:
 * cpu_state_s, ppu_state_s, or memory stuff.
 */
class NESTableModel : public QAbstractTableModel {

  Q_OBJECT

public:
  NESTableModel(int rows, int cols, QStringList header_labels,
                QWidget *parent = nullptr);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

public slots:
  void nes_started();
  void nes_stopped();
  
protected:
  void add_state_strings(QStringList &state_strings);
  void add_state_strings_buffered(QStringList &state_strings);

  bool nes_running;
  const int rows, cols;
  const QStringList header_labels;
  QList<QStringList> table_data;

private:
  int last_model_update;
};

class CPUTableModel : public NESTableModel {

  Q_OBJECT

public:
  explicit CPUTableModel(QWidget *parent = nullptr);

public slots:
  void addState(cpu_state_s *cpu_state);

private:
  static const int rows, cols;
  static const QStringList header_labels;
};

class PPUTableModel : public NESTableModel {

  Q_OBJECT

public:
  explicit PPUTableModel(QWidget *parent = nullptr);

public slots:
  void addState(ppu_state_s *ppu_state);

private:
  static const int rows, cols;
  static const QStringList header_labels;
};

class MemoryTableModel : public NESTableModel {

  Q_OBJECT

public:
  explicit MemoryTableModel(QWidget *parent = nullptr);

public slots:
  void addState(uint16_t addr, uint8_t val, char r_or_w);

private:
  static const int rows, cols;
  static const QStringList header_labels;
};

/* Main window that everything goes in, namely
 * table views for memory, cpu, ppu data, buttons to start/stop/step
 * emulator, memory hexdump button, and openglwidget containing
 * the graphical output of the emulator
 */
class MainWindow : public QMainWindow {

  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public slots:
  void done(void);
  void error(NESError e);

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

  QThread *nes_thread;
  NESContext *nes_context;
  NESCallbackForwarder *callback_forwarder;

private slots:
  void on_pauseButton_clicked();
  void on_playButton_clicked();
  void on_stepButton_clicked();
  void on_memoryDumpButton_clicked();
};

#endif // MAINWINDOW_H
