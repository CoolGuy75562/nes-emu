#include "mainwindow.h"
#include "nesscreen.h"
#include "ui_mainwindow.h"
#include <QBoxLayout>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTimer>
#include <QtDebug>
#include <iostream>

extern "C" {
void on_cpu_state_update();
void on_ppu_state_update();
void on_memory_fetch();
void on_memory_write();
}

const QStringList CPUTableModel::header_labels = {
    "PC", "CYC", "A",      "X",           "Y",
    "SP", "P",   "Opcode", "Instruction", "Addressing Mode"};
const int CPUTableModel::rows = 10;
const int CPUTableModel::cols = CPUTableModel::header_labels.size();

const QStringList PPUTableModel::header_labels = {
    "Cycles", "Scanline", "PPUCTRL", "PPUMASK", "PPUSTATUS", "w",         "x",
    "t",      "v",        "NT",      "AT",      "PTT (low)", "PTT (high)"};
const int PPUTableModel::rows = 10;
const int PPUTableModel::cols = PPUTableModel::header_labels.size();

const QStringList MemoryTableModel::header_labels = {"R/W", "Address", "Value"};
const int MemoryTableModel::rows = 100;
const int MemoryTableModel::cols = MemoryTableModel::header_labels.size();

void show_error(QWidget *parent, NESError &e) {
  std::cout << e.what() << std::endl;
  // memory_dump_file(stdout);
  QMessageBox::critical(parent, "Error", e.what());
}

void on_cpu_state_update(cpu_state_s *state, void *cpu_model) {
  static CPUTableModel *model = static_cast<CPUTableModel *>(cpu_model);
  model->addState(state);
}

void on_ppu_state_update(ppu_state_s *state, void *ppu_model) {
  static PPUTableModel *model = static_cast<PPUTableModel *>(ppu_model);
  model->addState(state);
}

void on_memory_fetch(uint16_t addr, uint8_t val, void *memory_model) {
  static MemoryTableModel *model =
      static_cast<MemoryTableModel *>(memory_model);
  model->addState(addr, val, 'r');
}

void on_memory_write(uint16_t addr, uint8_t val, void *memory_model) {
  static MemoryTableModel *model =
      static_cast<MemoryTableModel *>(memory_model);
  model->addState(addr, val, 'w');
}

NESTableModel::NESTableModel(int rows, int cols, QStringList header_labels,
                             QWidget *parent)
    : QAbstractTableModel(parent), rows(rows), cols(cols),
      header_labels(header_labels) {
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
  beginResetModel();
  table_data.push_front(state_strings);
  if (table_data.size() > rows) {
    table_data.pop_back();
  }
  endResetModel();
}

CPUTableModel::CPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {}

PPUTableModel::PPUTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {}

MemoryTableModel::MemoryTableModel(QWidget *parent)
    : NESTableModel(rows, cols, header_labels, parent) {}

void CPUTableModel::addState(cpu_state_s *state) {
  QStringList state_strings = {
      QStringLiteral("$%1").arg(state->pc, 4, 16, QLatin1Char('0')),
      QStringLiteral("%1").arg(state->cycles, 6, 10, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->a, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->x, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->y, 2, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->sp, 2, 16, QLatin1Char('0')),
      QStringLiteral("%1b").arg(state->p, 8, 2, QLatin1Char('0')),
      QStringLiteral("$%1").arg(state->opc, 2, 16, QLatin1Char('0')),
      QString(state->curr_instruction),
      QString(state->curr_addr_mode)};
  add_state_strings(state_strings);
}

/* And here */
void PPUTableModel::addState(ppu_state_s *state) {
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
  add_state_strings(state_strings);
}

void MemoryTableModel::addState(uint16_t addr, uint8_t val, char r_or_w) {
  QStringList state_strings = {
    (r_or_w == 'r') ? QStringLiteral("Read") : QStringLiteral("Write"),
      QStringLiteral("$%1").arg(addr, 4, 16, QLatin1Char('0')),
      QStringLiteral("$%1").arg(val, 2, 16, QLatin1Char('0'))};
  add_state_strings(state_strings);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
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

  cpu_register_state_callback(&on_cpu_state_update, cpu_model);
  ppu_register_state_callback(&on_ppu_state_update, ppu_model);
  memory_register_cb(&on_memory_fetch, memory_model, MEMORY_CB_FETCH);
  memory_register_cb(&on_memory_write, memory_model, MEMORY_CB_WRITE);
  
  QTimer *timer = new QTimer(this);
  timer->setInterval(1000);

  /* play/pause button behaviour */
  /*
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_cpu_state()));
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_ppu_state()));
  */
  connect(this, SIGNAL(play_button_clicked()), timer, SLOT(start()));
  connect(this, SIGNAL(pause_button_clicked()), timer, SLOT(stop()));

  /* step button behaviour */
  connect(this, SIGNAL(step_button_clicked()), timer, SLOT(stop()));
  /*
  connect(this, SIGNAL(step_button_clicked()), this, SLOT(refresh_cpu_state()));
  connect(this, SIGNAL(step_button_clicked()), this, SLOT(refresh_ppu_state()));
  */
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initOpenGLWidgetNESScreen(NESScreen *s) {
  ui->openGLWidget->initScreen(s);
}

/* ========== Slots ========== */

void MainWindow::done(void) { QApplication::quitOnLastWindowClosed(); }

void MainWindow::error(NESError &e) {
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

void MainWindow::on_memoryDumpButton_clicked() {
  emit pause_button_clicked();
  /*
  refresh_cpu_state();
  refresh_ppu_state();
  */
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
