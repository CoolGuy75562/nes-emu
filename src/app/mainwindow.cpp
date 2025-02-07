#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QThread>
#include <QTimer>
#include <QtDebug>
#include <cstring>
#include <iostream>

/* Callbacks */
extern "C" {
void on_cpu_state_update();
void on_ppu_state_update();
void on_memory_fetch();
void on_memory_write();
}

Q_DECLARE_METATYPE(NESError)
Q_DECLARE_METATYPE(cpu_state_s)
Q_DECLARE_METATYPE(ppu_state_s)
Q_DECLARE_METATYPE(cycle)
Q_DECLARE_METATYPE(ringbuffer<cpu_state_s>)
Q_DECLARE_METATYPE(ringbuffer<ppu_state_s>)
Q_DECLARE_METATYPE(ringbuffer<cycle>)

/*============================================================*/

/* Rows currently have to be power of 2 because also used for internal
 * ring buffer */
const QStringList CPUTableModel::header_labels = {
    "PC", "CYC", "A",      "X",           "Y",
    "SP", "P",   "Opcode", "Instruction", "Addressing Mode"};
const int CPUTableModel::rows = 16;
const int CPUTableModel::cols = CPUTableModel::header_labels.size();

const QStringList PPUTableModel::header_labels = {
    "Cycles", "Scanline", "PPUCTRL", "PPUMASK", "PPUSTATUS", "w",         "x",
    "t",      "v",        "NT",      "AT",      "PTT (low)", "PTT (high)"};
const int PPUTableModel::rows = 32;
const int PPUTableModel::cols = PPUTableModel::header_labels.size();

const QStringList MemoryTableModel::header_labels = {"R/W", "Address", "Value"};
const int MemoryTableModel::rows = 64;
const int MemoryTableModel::cols = MemoryTableModel::header_labels.size();

/*============================================================*/

void show_error(QWidget *parent, NESError &e) {
  std::cout << e.what() << std::endl;
  // memory_dump_file(stdout);
  QMessageBox::critical(parent, "Error", e.what());
}

/*============================================================*/

// This and the NESTableModels can definitely be optimised.
// Too much copying and CPU usage. Qt signals probably not the
// right tool for the job.
void on_cpu_state_update(const cpu_state_s *state, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->cpu_state_update(cpu_state_s(*state));
}

void on_ppu_state_update(const ppu_state_s *state, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->ppu_state_update(ppu_state_s(*state));
}

void on_memory_fetch(uint16_t addr, uint8_t val, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->memory_state_update(cycle(addr, val, 'r'));
}

void on_memory_write(uint16_t addr, uint8_t val, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->memory_state_update(cycle(addr, val, 'w'));
}

/*============================================================*/

NESCallbackBuffer::NESCallbackBuffer(QObject *parent)
    : QObject(parent) {

  for (int i = 0; i < 0x100; i++) {
    cpu_state_s cs;
    std::memset(&cs, 0, sizeof(cs));

    ppu_state_s ps;
    std::memset(&ps, 0, sizeof(ps));

    cpu_state_buffer.buf.at(i) = cs;
    ppu_state_buffer.buf.at(i) = ps;
    memory_state_buffer.buf.at(i) = cycle();
  }

  batch_timer = new QTimer(this);
  batch_timer->setInterval(64);
  connect(batch_timer, &QTimer::timeout, this, [this]() {
    emit this->batch_cpu_states(cpu_state_buffer);
    emit this->batch_ppu_states(ppu_state_buffer);
    emit this->batch_memory_states(memory_state_buffer);
  });
}

void NESCallbackBuffer::start(void) { batch_timer->start(); }

void NESCallbackBuffer::stop(void) {
  // Not sure if there is a possibility of more signals
  // queued that get thrown away after the flush
 
  // Flush
  emit batch_cpu_states(cpu_state_buffer);
  emit batch_ppu_states(ppu_state_buffer);
  emit batch_memory_states(memory_state_buffer);
  batch_timer->stop();
  emit cb_buffer_stopped();
}

void NESCallbackBuffer::cpu_state_update(cpu_state_s state) {
  cpu_state_buffer.push(state);
}

void NESCallbackBuffer::ppu_state_update(ppu_state_s state) {
  ppu_state_buffer.push(state);
}

void NESCallbackBuffer::memory_update(cycle state) {
  memory_state_buffer.push(state);
}

/*============================================================*/

CPUTableModel::CPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {
  table_data.reserve(rows);
  cpu_state_s s = {.pc = 0,
                   .cycles = 0,
                   .a = 0,
                   .x = 0,
                   .y = 0,
                   .sp = 0,
                   .p = 0,
                   .opc = 0,
                   .curr_instruction = "N/A",
                   .curr_addr_mode = "N/A"};
  for (int i = 0; i < rows; i++) {
    table_data.push_back(s);
  }
}

PPUTableModel::PPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {
  table_data.reserve(rows);
  ppu_state_s s = {.cycles = 0,
                   .scanline = 0,
                   .ppuctrl = 0,
                   .ppumask = 0,
                   .ppustatus = 0,
                   .w = 0,
                   .x = 0,
                   .t = 0,
                   .v = 0,
                   .nt_byte = 0,
                   .at_byte = 0,
                   .ptt_low = 0,
                   .ptt_high = 0};
  for (int i = 0; i < rows; i++) {
    table_data.push_back(s);
  }
}

MemoryTableModel::MemoryTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {
  table_data.reserve(rows);
  for (int i = 0; i < rows; i++) {
    table_data.push_back(cycle());
  }
}

/*============================================================*/

QVariant CPUTableModel::indexToQString(const QModelIndex &index) const {
  const cpu_state_s *state =
      &table_data.at((table_data_start_idx + rows - index.row()) & (rows - 1));
  switch (index.column()) {
  case 0:
    return QStringLiteral("$%1").arg(state->pc, 4, 16, QLatin1Char('0'));
  case 1:
    return QStringLiteral("%1").arg(state->cycles, 5, 10, QLatin1Char('0'));
  case 2:
    return QStringLiteral("$%1").arg(state->a, 2, 16, QLatin1Char('0'));
  case 3:
    return QStringLiteral("$%1").arg(state->x, 2, 16, QLatin1Char('0'));
  case 4:
    return QStringLiteral("$%1").arg(state->y, 2, 16, QLatin1Char('0'));
  case 5:
    return QStringLiteral("$%1").arg(state->sp, 2, 16, QLatin1Char('0'));
  case 6:
    return QStringLiteral("%1b").arg(state->p, 8, 2, QLatin1Char('0'));
  case 7:
    return QStringLiteral("$%1").arg(state->opc, 2, 16, QLatin1Char('0'));
  case 8:
    return QString(state->curr_instruction);
  case 9:
    return QString(state->curr_addr_mode);
  default:
    return QVariant();
  }
}

QVariant PPUTableModel::indexToQString(const QModelIndex &index) const {
  const ppu_state_s *state =
      &table_data.at((table_data_start_idx + rows - index.row()) & (rows - 1));
  switch (index.column()) {
  case 0:
    return QStringLiteral("%1").arg(state->cycles, 3, 10, QLatin1Char('0'));
  case 1:
    return QStringLiteral("%1").arg(state->scanline, 3, 10, QLatin1Char('0'));
  case 2:
    return QStringLiteral("%1b").arg(state->ppuctrl, 8, 2, QLatin1Char('0'));
  case 3:
    return QStringLiteral("%1b").arg(state->ppumask, 8, 2, QLatin1Char('0'));
  case 4:
    return QStringLiteral("%1b").arg(state->ppustatus, 8, 2, QLatin1Char('0'));
  case 5:
    return QStringLiteral("%1").arg(state->w, 1, 2, QLatin1Char('0'));
  case 6:
    return QStringLiteral("%1").arg(state->x, 1, 10, QLatin1Char('0'));
  case 7:
    return QStringLiteral("$%1").arg(state->t, 4, 16, QLatin1Char('0'));
  case 8:
    return QStringLiteral("$%1").arg(state->v, 4, 16, QLatin1Char('0'));
  case 9:
    return QStringLiteral("$%1").arg(state->nt_byte, 2, 16, QLatin1Char('0'));
  case 10:
    return QStringLiteral("$%1").arg(state->at_byte, 2, 16, QLatin1Char('0'));
  case 11:
    return QStringLiteral("$%1").arg(state->ptt_low, 2, 16, QLatin1Char('0'));
  case 12:
    return QStringLiteral("$%1").arg(state->ptt_high, 2, 16, QLatin1Char('0'));
  default:
    return QVariant();
  }
}

QVariant MemoryTableModel::indexToQString(const QModelIndex &index) const {
  const cycle *the_cycle =
      &table_data.at((table_data_start_idx + rows - index.row()) & (rows - 1));
  switch (index.column()) {
  case 0:
    return (the_cycle->r_or_w == 'r') ? "Read" : "Write";
  case 1:
    return QStringLiteral("$%1").arg(the_cycle->addr, 4, 16, QLatin1Char('0'));
  case 2:
    return QStringLiteral("$%1").arg(the_cycle->val, 2, 16, QLatin1Char('0'));
  default:
    return QVariant();
  }
}

/* Hack because slots don't work with template classes */
void MemoryTableModel::addState(cycle c) { NESTableModel<cycle>::addState(c); }
void CPUTableModel::addState(cpu_state_s s) {
  NESTableModel<cpu_state_s>::addState(s);
}
void PPUTableModel::addState(ppu_state_s s) {
  NESTableModel<ppu_state_s>::addState(s);
}

void MemoryTableModel::addState(ringbuffer<cycle> r) {
  NESTableModel<cycle>::addState(r);
}
void CPUTableModel::addState(ringbuffer<cpu_state_s> r) {
  NESTableModel<cpu_state_s>::addState(r);
}
void PPUTableModel::addState(ringbuffer<ppu_state_s> r) {
  NESTableModel<ppu_state_s>::addState(r);
}
/*============================================================*/

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), ui(new Ui::MainWindow), paused(true) {

  qRegisterMetaType<ringbuffer<cycle>>();
  qRegisterMetaType<cycle>();
  qRegisterMetaType<ringbuffer<cpu_state_s>>();
  qRegisterMetaType<cpu_state_s>();
  qRegisterMetaType<ringbuffer<ppu_state_s>>();
  qRegisterMetaType<ppu_state_s>();
  qRegisterMetaType<NESError>();
  
  /*----------------------Set up gui thread------------------------------*/
  cpu_model = new CPUTableModel(this);
  ppu_model = new PPUTableModel(this);
  memory_model = new MemoryTableModel(this);

  qDebug() << "MainWindow: Setting up ui";
  ui->setupUi(this);

  auto init_view = [](QTableView *view, QAbstractTableModel *model,
                      enum QHeaderView::ResizeMode resize_mode) {
    view->setModel(model);
    view->verticalHeader()->hide();
    view->horizontalHeader()->setSectionResizeMode(resize_mode);
  };

  init_view(ui->cpuStateTableView, cpu_model, QHeaderView::ResizeToContents);
  init_view(ui->ppuStateTableView, ppu_model, QHeaderView::ResizeToContents);
  init_view(ui->memoryTableView, memory_model, QHeaderView::Stretch);

  NESScreen *s = new NESScreen(this);
  ui->openGLWidget->initScreen(s);

  /*----------------------Set up emulator and other threads-----------------*/

  nes_thread = new QThread();
  cb_buffer_thread = new QThread();
  
  // Get .nes file to open
  const std::string rom_filename =
      QFileDialog::getOpenFileName(this, "Open File", "/home",
                                   "NES ROM (*.nes)")
          .toStdString();

  init_callback_buffer();
  init_callback_forwarder();
  init_nes_controller();
  init_nes_context(rom_filename, s);


  // have to wait for emulator to pause and send everything to buffer
  // before buffer is flushed
  connect(nes_context, SIGNAL(nes_paused()), callback_buffer, SLOT(stop()));

  // have to wait for buffer to be done sending signals before disconnecting
  // those signals from buffer
  connect(callback_buffer, SIGNAL(cb_buffer_stopped()), this,
          SLOT(cb_single_step_mode()));

  // above isn't so important for play button case because everything updates
  // so quickly
  connect(this, SIGNAL(play_button_clicked()), this, SLOT(cb_buffer_mode()));
  connect(this, SIGNAL(play_button_clicked()), callback_buffer, SLOT(start()));
 
  /* I think this should delete everything properly when exiting programme */

  /*Nope it segfaults when closing window when nes context running */
  connect(nes_context, SIGNAL(nes_done()), nes_thread, SLOT(quit()));
  connect(nes_context, SIGNAL(nes_done()), nes_context, SLOT(deleteLater()));
  connect(nes_context, SIGNAL(nes_done()), callback_forwarder,
          SLOT(deleteLater()));
  connect(nes_context, SIGNAL(nes_done()), nes_controller, SLOT(deleteLater()));

  connect(nes_thread, SIGNAL(finished()), nes_thread, SLOT(deleteLater()));
  connect(nes_thread, SIGNAL(finished()), this, SLOT(done()));
  qDebug() << "Starting nes thread";
  nes_thread->start();
  cb_buffer_thread->start();
}

MainWindow::~MainWindow() { delete ui; }

/*============================================================*/

void MainWindow::done(void) { QApplication::quitOnLastWindowClosed(); }

void MainWindow::error(NESError e) {
  /*
  refresh_cpu_state();
  refresh_ppu_state();
  */
  show_error(this, e);
  // QApplication::quit();
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
  Q_UNUSED(event);

  qDebug() << "MainWindow::mousePressEvent()";
  setFocus();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  // qDebug() << "Pressed key " << event->key();
  if (!event->isAutoRepeat()) {
    emit nes_button_pressed(event->key());
  }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
  // qDebug() << "Released key " << event->key();
  if (!event->isAutoRepeat()) {
    emit nes_button_released(event->key());
  }
}

/* Switch single step updates with buffer updates */
void MainWindow::cb_buffer_mode() {
  disconnect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s)),
             cpu_model, SLOT(addState(cpu_state_s)));
  disconnect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s)),
             ppu_model, SLOT(addState(ppu_state_s)));
  disconnect(callback_forwarder, SIGNAL(memory_state_update(cycle)),
             memory_model, SLOT(addState(cycle)));

  connect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s)),
          callback_buffer, SLOT(cpu_state_update(cpu_state_s)));
  connect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s)),
          callback_buffer, SLOT(ppu_state_update(ppu_state_s)));
  connect(callback_forwarder, SIGNAL(memory_state_update(cycle)),
          callback_buffer, SLOT(memory_update(cycle)));
}

/* above but reversed */
void MainWindow::cb_single_step_mode() {
  
  disconnect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s)),
             callback_buffer, SLOT(cpu_state_update(cpu_state_s)));
  disconnect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s)),
             callback_buffer, SLOT(ppu_state_update(ppu_state_s)));
  disconnect(callback_forwarder, SIGNAL(memory_state_update(cycle)),
             callback_buffer, SLOT(memory_update(cycle)));

  connect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s)), cpu_model,
          SLOT(addState(cpu_state_s)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s)), ppu_model,
          SLOT(addState(ppu_state_s)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(memory_state_update(cycle)), memory_model,
          SLOT(addState(cycle)), Qt::QueuedConnection);
}

void MainWindow::on_pauseButton_clicked() {
  if (!paused) {
    paused = true;
    emit pause_button_clicked();
  }
}

void MainWindow::on_playButton_clicked() {
  if (paused) {
    paused = false;
    emit play_button_clicked();
  }
}

void MainWindow::on_stepButton_clicked() {
  on_pauseButton_clicked();
  emit step_button_clicked();
}

/* Double check this is ok with threads */
void MainWindow::on_memoryDumpButton_clicked() {
  on_pauseButton_clicked();

  // why not just dump it to a file and read it?
  char dump_data[1 << 19];
  size_t dump_len = 1 << 19;
  if (memory_dump_string(dump_data, dump_len) < 0) {
    qDebug() << "Memory dump didn't work";
    return;
  }

  show_hexdump_dialog(dump_data);
}

void MainWindow::on_VRAMDumpButton_clicked() {
  on_pauseButton_clicked();
  
  char dump_data[1 << 18];
  size_t dump_len = 1 << 18;
  if (memory_vram_dump_string(dump_data, dump_len) < 0) {
    qDebug() << "VRAM dump didn't work";
    return;
  }

  show_hexdump_dialog(dump_data);
}

void MainWindow::on_patternTableButton_clicked() {
  on_pauseButton_clicked();
  
  QDialog pattern_table_dialog(this);

  PatternTableViewer *left_pt_viewer =
      new PatternTableViewer(0, &pattern_table_dialog);
  left_pt_viewer->setMinimumSize(256, 256);
  left_pt_viewer->draw_pattern_table();
  PatternTableViewer *right_pt_viewer =
      new PatternTableViewer(1, &pattern_table_dialog);
  right_pt_viewer->setMinimumSize(256, 256);
  right_pt_viewer->draw_pattern_table();

  QBoxLayout pattern_table_dialog_layout(QBoxLayout::LeftToRight);
  pattern_table_dialog_layout.addWidget(left_pt_viewer);
  pattern_table_dialog_layout.addWidget(right_pt_viewer);
  pattern_table_dialog.setLayout(&pattern_table_dialog_layout);

  pattern_table_dialog.exec();
}

void MainWindow::show_hexdump_dialog(const char *dump_data) {
  QDialog hexdump_dialog(this);
  QPlainTextEdit *hexdump_text_view = new QPlainTextEdit(&hexdump_dialog);

  hexdump_text_view->setReadOnly(true);
  hexdump_text_view->setLineWrapMode(QPlainTextEdit::NoWrap);
  hexdump_text_view->setPlainText(dump_data);
  hexdump_text_view->setFont(QFont("Monospace"));

  QBoxLayout hexdump_dialog_layout(QBoxLayout::TopToBottom);
  hexdump_dialog_layout.addWidget(hexdump_text_view);
  hexdump_dialog.setLayout(&hexdump_dialog_layout);

  hexdump_dialog.exec();
}

void MainWindow::init_callback_buffer() {
  callback_buffer = new NESCallbackBuffer();
  callback_buffer->moveToThread(cb_buffer_thread);
  
connect(callback_buffer, SIGNAL(batch_cpu_states(ringbuffer<cpu_state_s>)),
        cpu_model, SLOT(addState(ringbuffer<cpu_state_s>)),
        Qt::QueuedConnection);
connect(callback_buffer, SIGNAL(batch_ppu_states(ringbuffer<ppu_state_s>)),
        ppu_model, SLOT(addState(ringbuffer<ppu_state_s>)),
        Qt::QueuedConnection);
connect(callback_buffer, SIGNAL(batch_memory_states(ringbuffer<cycle>)),
        memory_model, SLOT(addState(ringbuffer<cycle>)),
        Qt::QueuedConnection);

}

void MainWindow::init_callback_forwarder() {
  callback_forwarder = new NESCallbackForwarder();
  callback_forwarder->moveToThread(nes_thread);
  
  // start in single step mode as opposed to buffer mode 
  cpu_register_state_callback(&on_cpu_state_update, callback_forwarder);
  ppu_register_state_callback(&on_ppu_state_update, callback_forwarder);
  memory_register_cb(&on_memory_fetch, callback_forwarder, MEMORY_CB_FETCH);
  memory_register_cb(&on_memory_write, callback_forwarder, MEMORY_CB_WRITE);

  connect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s)), cpu_model,
          SLOT(addState(cpu_state_s)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s)), ppu_model,
          SLOT(addState(ppu_state_s)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(memory_update(cycle)), memory_model,
          SLOT(addState(cycle)), Qt::QueuedConnection);
}

void MainWindow::init_nes_controller() {
  nes_controller = new NESController();
  nes_controller->moveToThread(nes_thread);
  // controller signals 
  connect(this, SIGNAL(nes_button_pressed(int)), nes_controller,
          SLOT(nes_button_pressed(int)), Qt::QueuedConnection);
  connect(this, SIGNAL(nes_button_released(int)), nes_controller,
          SLOT(nes_button_released(int)), Qt::QueuedConnection);
}

void MainWindow::init_nes_context(const std::string &rom_filename, NESScreen *s) {
  try {
    qDebug() << "Initialising NESContext";
    nes_context = new NESContext(rom_filename, s->get_put_pixel(), s,
                                 &get_pressed_buttons, nes_controller);
  } catch (NESError &e) {
    error(e);
    throw e;
  }

  nes_context->moveToThread(nes_thread);

  connect(this, SIGNAL(pause_button_clicked()), nes_context, SLOT(nes_pause()),
          Qt::BlockingQueuedConnection);
  connect(this, SIGNAL(play_button_clicked()), nes_context, SLOT(nes_start()),
          Qt::BlockingQueuedConnection);
  
  connect(this, SIGNAL(step_button_clicked()), nes_context, SLOT(nes_step()));
  connect(nes_context, SIGNAL(nes_error(NESError)), this,
          SLOT(error(NESError)));
}
