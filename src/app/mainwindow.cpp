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
#include <qobjectdefs.h>

/* Callbacks */
extern "C" {
void on_cpu_state_update();
void on_ppu_state_update();
void on_memory_fetch();
void on_memory_write();
}

Q_DECLARE_METATYPE(NESError)

/* Model constants */
const QStringList CPUTableModel::header_labels = {
    "PC", "CYC", "A",      "X",           "Y",
    "SP", "P",   "Opcode", "Instruction", "Addressing Mode"};
const int CPUTableModel::rows = 25;
const int CPUTableModel::cols = CPUTableModel::header_labels.size();

const QStringList PPUTableModel::header_labels = {
    "Cycles", "Scanline", "PPUCTRL", "PPUMASK", "PPUSTATUS", "w",         "x",
    "t",      "v",        "NT",      "AT",      "PTT (low)", "PTT (high)"};
const int PPUTableModel::rows = 75;
const int PPUTableModel::cols = PPUTableModel::header_labels.size();

const QStringList MemoryTableModel::header_labels = {"R/W", "Address", "Value"};
const int MemoryTableModel::rows = 50;
const int MemoryTableModel::cols = MemoryTableModel::header_labels.size();

void show_error(QWidget *parent, NESError &e) {
  std::cout << e.what() << std::endl;
  // memory_dump_file(stdout);
  QMessageBox::critical(parent, "Error", e.what());
}

void on_cpu_state_update(cpu_state_s *state, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  /* Not ideal but makes threads work */
  emit cbf->cpu_state_update(new cpu_state_s(*state));
}

void on_ppu_state_update(ppu_state_s *state, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  /* Not ideal but makes threads work */
  emit cbf->ppu_state_update(new ppu_state_s(*state));
}

void on_memory_fetch(uint16_t addr, uint8_t val, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->memory_update(addr, val, 'r');
}

void on_memory_write(uint16_t addr, uint8_t val, void *cb_forwarder) {
  static NESCallbackForwarder *cbf =
      static_cast<NESCallbackForwarder *>(cb_forwarder);
  emit cbf->memory_update(addr, val, 'w');
}

NESTableModel::NESTableModel(int rows, int cols, QStringList header_labels,
                             QWidget *parent)
    : QAbstractTableModel(parent), rows(rows), cols(cols),
      header_labels(header_labels) {
  nes_running = false;
  last_model_update = 0;
  QStringList init_strings;
  for (int i = 0; i < cols; i++) {
    init_strings.push_back(QString("N/A"));
  }
  for (int i = 0; i < rows; i++) {
    table_data.push_back(init_strings);
  }
}

int NESTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return rows;
}

int NESTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return cols;
}

QVariant NESTableModel::data(const QModelIndex &index, int role) const {
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
    return table_data[index.row()][index.column()];
  }
  return QVariant();
}

QVariant NESTableModel::headerData(int section, Qt::Orientation orientation,
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

void NESTableModel::add_state_strings(QStringList &state_strings) {
  table_data.push_front(state_strings);
  if (table_data.size() > rows) {
    table_data.pop_back();
  }
  emit dataChanged(index(0, 0), index(rows - 1, cols - 1));
}

void NESTableModel::add_state_strings_buffered(QStringList &state_strings) {
  table_data.push_front(state_strings);
  if (table_data.size() > rows) {
    table_data.pop_back();
  }
  if (last_model_update++ >= 100*rows) {
    last_model_update = 0;
    emit dataChanged(index(0, 0), index(rows - 1, cols - 1));
  }

}

void NESTableModel::nes_started() {
  nes_running = true;
  last_model_update = 0;
}

void NESTableModel::nes_stopped() {
  nes_running = false;
  emit dataChanged(index(0, 0), index(rows - 1, cols - 1));
}

CPUTableModel::CPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {}

PPUTableModel::PPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {}

MemoryTableModel::MemoryTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {}

/* Note that state is heap allocated */
void CPUTableModel::addState(cpu_state_s *state) {
  std::unique_ptr<cpu_state_s> state_ptr(state);
  QStringList state_strings = {
      QStringLiteral("$%1").arg(state->pc, 4, 16, QLatin1Char('0')),
      QStringLiteral("%1").arg(state->cycles, 5, 10, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->a, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->x, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->y, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->sp, 2, 16, QLatin1Char('0')),
      QStringLiteral("%1b").arg(state->p, 8, 2, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->opc, 2, 16, QLatin1Char('0')),
      QString(state->curr_instruction),
      QString(state->curr_addr_mode)};
  if (nes_running) {
    add_state_strings_buffered(state_strings);
  } else {
    add_state_strings(state_strings);
  }
}

/* Note that state is heap allocated */
void PPUTableModel::addState(ppu_state_s *state) {
  std::unique_ptr<ppu_state_s> state_ptr(state);
  QStringList state_strings = {
      QStringLiteral("%1").arg(state->cycles, 3, 10, QLatin1Char('0')),
      QStringLiteral("%1").arg(state->scanline, 3, 10, QLatin1Char('0')),
      QStringLiteral("%1b").arg(state->ppuctrl, 8, 2, QLatin1Char('0')),
      QStringLiteral("%1b").arg(state->ppumask, 8, 2, QLatin1Char('0')),
      QStringLiteral("%1b").arg(state->ppustatus, 8, 2, QLatin1Char('0')),
      QStringLiteral("%1").arg(state->w, 1, 2, QLatin1Char('0')),
      QStringLiteral("%1").arg(state->x, 1, 10, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->t, 4, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->v, 4, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->nt_byte, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->at_byte, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->ptt_low, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->ptt_high, 2, 16, QLatin1Char('0'))};
  if (nes_running) {
    add_state_strings_buffered(state_strings);
  } else {
    add_state_strings(state_strings);
  }
}

void MemoryTableModel::addState(uint16_t addr, uint8_t val, char r_or_w) {
  QStringList state_strings = {
      (r_or_w == 'r') ? QStringLiteral("Read") : QStringLiteral("Write"),
      QStringLiteral("$%1").arg(addr, 4, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(val, 2, 16, QLatin1Char('0'))};
  if (nes_running) {
    add_state_strings_buffered(state_strings);
  } else {
    add_state_strings(state_strings);
  }
}

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
    view->horizontalHeader()->setSectionResizeMode(resize_mode);
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

  qRegisterMetaType<uint16_t>("uint16_t");
  qRegisterMetaType<uint8_t>("uint8_t");
  qRegisterMetaType<NESError>();

  connect(callback_forwarder, SIGNAL(cpu_state_update(cpu_state_s *)),
          cpu_model, SLOT(addState(cpu_state_s *)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(ppu_state_update(ppu_state_s *)),
          ppu_model, SLOT(addState(ppu_state_s *)), Qt::QueuedConnection);
  connect(callback_forwarder, SIGNAL(memory_update(uint16_t, uint8_t, char)),
          memory_model, SLOT(addState(uint16_t, uint8_t, char)),
          Qt::QueuedConnection);

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

  connect(nes_context, SIGNAL(nes_error(NESError)), this,
          SLOT(error(NESError)));

  QTimer *timer = new QTimer();
  timer->setInterval(1); // temporary until I can get gui thread to catch up with emu thread 
  timer->moveToThread(nes_thread);

  connect(timer, SIGNAL(timeout()), nes_context, SLOT(nes_step()));
  connect(this, SIGNAL(pause_button_clicked()), timer, SLOT(stop()));
  connect(this, SIGNAL(play_button_clicked()), timer, SLOT(start()));
  connect(this, SIGNAL(step_button_clicked()), timer, SLOT(stop()));
  connect(this, SIGNAL(step_button_clicked()), nes_context, SLOT(nes_step()));

  /* I think this should delete everything properly when exiting programme */
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

/* ========== Slots ========== */

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
  QDialog memory_dump_dialog(this);

  QPlainTextEdit *memory_dump_text_view =
      new QPlainTextEdit(&memory_dump_dialog);
  memory_dump_text_view->setReadOnly(true);
  char dump_data[1 << 19];
  size_t dump_len = 1 << 19;
  if (memory_dump_string(dump_data, dump_len) < 0) {
    qDebug() << "Memory dump didn't work";
  } else {
    memory_dump_text_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    memory_dump_text_view->setPlainText(dump_data);
    memory_dump_text_view->setFont(QFont("Monospace"));
    // memory_dump_text_view->setFixedWidth(memory_dump_text_view->document()->size().width()
    // + 10);
  }

  QBoxLayout memory_dump_dialog_layout(QBoxLayout::TopToBottom);
  memory_dump_dialog_layout.addWidget(memory_dump_text_view);

  memory_dump_dialog.setLayout(&memory_dump_dialog_layout);
  memory_dump_dialog.exec();
}
