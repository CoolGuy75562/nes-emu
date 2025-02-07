#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "nescontext.h"
#include "nestablemodel.h"
#include "nescontroller.h"
#include "nesscreen.h"

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
/* This is a wrapper for the callback fucntions e.g.
 * on_cpu_state_update so that they can send signals */
class NESCallbackForwarder : public QObject {

  Q_OBJECT

public:
  explicit NESCallbackForwarder(QObject *parent = nullptr) : QObject(parent) {};
  
signals:
  void cpu_state_update(cpu_state_s);
  void ppu_state_update(ppu_state_s);
  void memory_state_update(cycle);
};

/*============================================================*/
/* When the emulator is running (not paused or single step mode)
 * the NESCallbackForwarder sends the logging data here,
 * which lives in a separate thread to the gui and the emulator,
 * and sends the logging data in a batch to the gui thread every
 * so often. */
class NESCallbackBuffer : public QObject {

  Q_OBJECT

public:
  explicit NESCallbackBuffer(QObject *parent = nullptr);

public slots:
  void cpu_state_update(cpu_state_s);
  void ppu_state_update(ppu_state_s);
  void memory_update(cycle);

  void start();
  void stop();
  
signals:
  void batch_cpu_states(ringbuffer<cpu_state_s>);
  void batch_ppu_states(ringbuffer<ppu_state_s>);
  void batch_memory_states(ringbuffer<cycle>);

  void cb_buffer_stopped();
private:
  QTimer *batch_timer;

  ringbuffer<cpu_state_s> cpu_state_buffer;
  ringbuffer<ppu_state_s> ppu_state_buffer;
  ringbuffer<cycle> memory_state_buffer;
};

/*============================================================*/

class CPUTableModel : public NESTableModel<cpu_state_s> {

  Q_OBJECT

public:
  explicit CPUTableModel(QWidget *parent = nullptr);

public slots:
  void addState(cpu_state_s);
  void addState(ringbuffer<cpu_state_s>);
  
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
  void addState(ppu_state_s);
  void addState(ringbuffer<ppu_state_s>);
  
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
  void addState(cycle);
  void addState(ringbuffer<cycle>);
  
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
 * the graphical output of the emulator, etc
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
  void nes_button_pressed(int key);
  void nes_button_released(int key);
  
  void pause_button_clicked();
  void play_button_clicked();
  void step_button_clicked();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

private:
  void init_callback_buffer();
  void init_callback_forwarder();
  void init_nes_controller();
  void init_nes_context(const std::string &rom_filename, NESScreen *s);
  void show_hexdump_dialog(const char *dump_data);

  Ui::MainWindow *ui;

  // GUI/main thread
  CPUTableModel *cpu_model;
  PPUTableModel *ppu_model;
  MemoryTableModel *memory_model;

  // NES thread
  QThread *nes_thread;
  NESContext *nes_context;
  NESController *nes_controller;
  NESCallbackForwarder *callback_forwarder;

  // Buffer thread
  QThread *cb_buffer_thread;
  NESCallbackBuffer *callback_buffer;

  bool paused;
		    
private slots:
  void cb_single_step_mode();
  void cb_buffer_mode();

  // Buttons
  void on_pauseButton_clicked();
  void on_playButton_clicked();
  void on_stepButton_clicked();
  void on_memoryDumpButton_clicked();
  void on_VRAMDumpButton_clicked();
  void on_patternTableButton_clicked();
};

#endif // MAINWINDOW_H
