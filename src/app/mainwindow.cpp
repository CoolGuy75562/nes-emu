#include "mainwindow.h"
#include "nesscreen.h"
#include "ui_mainwindow.h"

#include <QBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QThread>
#include <QTimer>
#include <QtDebug>
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
  emit cbf->memory_update(cycle(addr, val, 'r'));
}

void on_memory_write(uint16_t addr, uint8_t val, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->memory_update(cycle(addr, val, 'w'));
}

/*============================================================*/

CPUTableModel::CPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {
  table_data.reserve(rows);
  cpu_state_s s = {
    .pc = 0,
    .cycles = 0,
    .a = 0,
    .x = 0,
    .y = 0,
    .sp = 0,
    .p = 0,
    .opc = 0,
    .curr_instruction = "N/A",
    .curr_addr_mode = "N/A"
  };
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
    return QStringLiteral("$%1").arg(the_cycle->addr, 4, 16,
                                     QLatin1Char('0'));
  case 2:
    return QStringLiteral("$%1").arg(the_cycle->val, 2, 16,
                                     QLatin1Char('0'));
  default:
    return QVariant();
  }
}

/* Hack because slots don't work with template classes */
void MemoryTableModel::nes_started() { NESTableModel<cycle>::nes_started(); }
void MemoryTableModel::nes_stopped() { NESTableModel<cycle>::nes_stopped(); }
void MemoryTableModel::addState(cycle c) { NESTableModel<cycle>::addState(c); }

void CPUTableModel::nes_started() { NESTableModel<cpu_state_s>::nes_started(); }
void CPUTableModel::nes_stopped() { NESTableModel<cpu_state_s>::nes_stopped(); }
void CPUTableModel::addState(cpu_state_s s) {NESTableModel<cpu_state_s>::addState(s); }

void PPUTableModel::nes_started() { NESTableModel<ppu_state_s>::nes_started(); }
void PPUTableModel::nes_stopped() { NESTableModel<ppu_state_s>::nes_stopped(); }
void PPUTableModel::addState(ppu_state_s s) {NESTableModel<ppu_state_s>::addState(s); }

/*============================================================*/

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {

  /*----------------------Set up gui thread------------------------------*/
  cpu_model = new CPUTableModel(this);
  ppu_model = new PPUTableModel(this);
  memory_model = new MemoryTableModel(this);

  qDebug() << "MainWindow: Setting up ui";
  ui->setupUi(this);

  auto init_view = [this](QTableView *view, QAbstractTableModel *model,
                          enum QHeaderView::ResizeMode resize_mode) {
    view->setModel(model);
    view->verticalHeader()->hide();
    // view->horizontalHeader()->setSectionResizeMode(resize_mode);
    connect(this, SIGNAL(play_button_clicked()), model, SLOT(nes_started()));
    connect(this, SIGNAL(pause_button_clicked()), model, SLOT(nes_stopped()));
    connect(this, SIGNAL(step_button_clicked()), model, SLOT(nes_stopped()));
  };

  init_view(ui->cpuStateTableView, cpu_model, QHeaderView::ResizeToContents);
  init_view(ui->ppuStateTableView, ppu_model, QHeaderView::ResizeToContents);
  init_view(ui->memoryTableView, memory_model, QHeaderView::Stretch);

  NESScreen *s = new NESScreen(this);
  ui->openGLWidget->initScreen(s);

  /*----------------------Set up emulator thread-------------------------*/
  const std::string rom_filename =
      QFileDialog::getOpenFileName(this, "Open File", "/home",
                                   "NES ROM (*.nes)")
          .toStdString();

  callback_forwarder = new NESCallbackForwarder();
  cpu_register_state_callback(&on_cpu_state_update, callback_forwarder);
  ppu_register_state_callback(&on_ppu_state_update, callback_forwarder);
  memory_register_cb(&on_memory_fetch, callback_forwarder, MEMORY_CB_FETCH);
  memory_register_cb(&on_memory_write, callback_forwarder, MEMORY_CB_WRITE);

  qRegisterMetaType<cycle>();
  qRegisterMetaType<cpu_state_s>();
  qRegisterMetaType<ppu_state_s>();
  qRegisterMetaType<NESError>();
  
  
  
  try {
    qDebug() << "Initialising NESContext";
    nes_context = new NESContext(rom_filename, s->get_put_pixel(), s);
  } catch (NESError &e) {
    error(e);
    throw e;
  }
  nes_thread = new QThread();
  nes_context->moveToThread(nes_thread);
  callback_forwarder->moveToThread(nes_thread);

  /* TODO: Move model addState logic to other thread because
   * gui thread getting bogged down by all the updates
   */
  connect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s)),
          cpu_model, SLOT(addState(cpu_state_s)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s)),
          ppu_model, SLOT(addState(ppu_state_s)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(memory_update(cycle)), memory_model,
          SLOT(addState(cycle)), Qt::QueuedConnection);
  
  connect(nes_context, SIGNAL(nes_error(NESError)), this,
          SLOT(error(NESError)));

  // TODO: sync emulator to frames
  QTimer *timer = new QTimer();
  timer->setInterval(0);
  timer->moveToThread(nes_thread);

  connect(timer, SIGNAL(timeout()), nes_context, SLOT(nes_step()));
  connect(this, SIGNAL(pause_button_clicked()), timer, SLOT(stop()));
  connect(this, SIGNAL(play_button_clicked()), timer, SLOT(start()));
  connect(this, SIGNAL(step_button_clicked()), timer, SLOT(stop()));
  connect(this, SIGNAL(step_button_clicked()), nes_context, SLOT(nes_step()));

  /* I think this should delete everything properly when exiting programme */

  /*Nope it segfaults when closing window when nes context running */
  connect(nes_context, SIGNAL(nes_done()), timer, SLOT(stop()));
  connect(nes_context, SIGNAL(nes_done()), nes_thread, SLOT(quit()));
  connect(nes_context, SIGNAL(nes_done()), nes_context, SLOT(deleteLater()));
  connect(nes_context, SIGNAL(nes_done()), callback_forwarder,
          SLOT(deleteLater()));

  connect(nes_thread, SIGNAL(finished()), nes_thread, SLOT(deleteLater()));
  connect(nes_thread, SIGNAL(finished()), this, SLOT(done()));
  qDebug() << "Starting nes thread";
  nes_thread->start();
}

MainWindow::~MainWindow() { delete ui; }

/*============================================================*/

void MainWindow::done(void) { QApplication::quitOnLastWindowClosed(); }

void MainWindow::error(NESError e) {
  emit pause_button_clicked();
  /*
  refresh_cpu_state();
  refresh_ppu_state();
  */
  show_error(this, e);
  // QApplication::quit();
}

void MainWindow::on_pauseButton_clicked() { emit pause_button_clicked(); }

void MainWindow::on_playButton_clicked() { emit play_button_clicked(); }

void MainWindow::on_stepButton_clicked() { emit step_button_clicked(); }

/* Double check this is ok with threads */
void MainWindow::on_memoryDumpButton_clicked() {
  emit pause_button_clicked();
  char dump_data[1 << 19];
  size_t dump_len = 1 << 19;
  if (memory_dump_string(dump_data, dump_len) < 0) {
    qDebug() << "Memory dump didn't work";
    return;
  }

  show_hexdump_dialog(dump_data);
}

void MainWindow::on_VRAMDumpButton_clicked() {
  emit pause_button_clicked();
  char dump_data[1 << 18];
  size_t dump_len = 1 << 18;
  if (memory_vram_dump_string(dump_data, dump_len) < 0) {
    qDebug() << "VRAM dump didn't work";
    return;
  }
  
  show_hexdump_dialog(dump_data);
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
  
