#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "nescontext.h"
#include "nestablemodel.h"

#include <QMainWindow>

void show_error(QWidget *parent, NESError &e);

struct cycle {
  cycle(uint16_t addr, uint8_t val, char r_or_w)
      : addr(addr), val(val), r_or_w(r_or_w) {}
  cycle(const cycle &other)
      : addr(other.addr), val(other.val), r_or_w(other.r_or_w) {}
  cycle() : addr(0), val(0), r_or_w('0') {}
  uint16_t addr;
  uint8_t val;
  char r_or_w;
};

/*============================================================*/
/* this class sends signals with the arguments of the cpu, ppu, memory callbacks
   in the nes thread to the memory, cpu, ppu models in the gui thread. */
class NESCallbackForwarder : public QObject {

  Q_OBJECT

public:
  explicit NESCallbackForwarder(QObject *parent = nullptr) : QObject(parent) {}

signals:
  void cpu_state_update(cpu_state_s);
  void ppu_state_update(ppu_state_s);
  void memory_update(cycle);
};

/*============================================================*/

class CPUTableModel : public NESTableModel<cpu_state_s> {

  Q_OBJECT

public:
  explicit CPUTableModel(QWidget *parent = nullptr);

public slots:
  void nes_started();
  void nes_stopped();
  void addState(cpu_state_s);
  
protected:
  QVariant indexToQString(const QModelIndex &index) const override;
  
private:
  static const int rows, cols;
  static const QStringList header_labels;
};

/*============================================================*/

class PPUTableModel : public NESTableModel<ppu_state_s> {

  Q_OBJECT

public:
  explicit PPUTableModel(QWidget *parent = nullptr);

public slots:
  void nes_started();
  void nes_stopped();
  void addState(ppu_state_s);
  
protected:
  QVariant indexToQString(const QModelIndex &index) const override;
  
private:
  static const int rows, cols;
  static const QStringList header_labels;
};

/*============================================================*/

class MemoryTableModel : public NESTableModel<cycle> {

  Q_OBJECT

public:
  explicit MemoryTableModel(QWidget *parent = nullptr);

public slots:
  void nes_started();
  void nes_stopped();
  void addState(cycle);
  
protected:
  QVariant indexToQString(const QModelIndex &index) const override;

private:
  static const int rows, cols;
  static const QStringList header_labels;
};

/*============================================================*/

namespace Ui {
class MainWindow;
}

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
  void show_hexdump_dialog(const char *dump_data);
  
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
  void on_VRAMDumpButton_clicked();
  void on_patternTableButton_clicked();
};

#endif // MAINWINDOW_H
