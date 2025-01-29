#include <QMessageBox>
#include <QtDebug>
#include <QTimer>
#include <QPlainTextEdit>
#include <QBoxLayout>
#include <iostream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "nesscreen.h"


void show_error(QWidget *parent, NESError &e) {
  std::cout << e.what() << std::endl;
  //memory_dump_file(stdout);
  QMessageBox::critical(parent, "Error", e.what());
}

void on_cpu_state_update(cpu_state_s *state, void *window) {
  /* there is only one main window so only need to set this once */
  static MainWindow *win = static_cast<MainWindow *>(window);
  win->cpu_state = state;
}

void on_ppu_state_update(ppu_state_s *state, void *window) {
  static MainWindow *win = static_cast<MainWindow *>(window);
  win->ppu_state = state;
}

const QStringList MainWindow::ppu_labels = {"Cycles:",
                                            "Scanline:",
                                            "PPUCTRL ($2000):",
                                            "PPUMASK ($2001):",
                                            "PPUSTATUS ($2002):",
                                            "w:",
                                            "x:",
                                            "t:",
                                            "v:",
                                            "Nametable byte:",
                                            "Attribute table byte:",
                                            "Pattern table low byte:",
                                            "Pattern table high byte:"};

const QStringList MainWindow::cpu_labels = {
    "PC:", "CYC:", "A:",      "X:",           "Y:",
    "SP:", "P:",   "Opcode:", "Instruction:", "Addressing Mode:"};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  cpu_state = nullptr;
  ppu_state = nullptr;
  qDebug() << "MainWindow: Setting up ui";
  ui->setupUi(this);
  
  cpu_register_state_callback(&on_cpu_state_update, this);
  ppu_register_state_callback(&on_ppu_state_update, this);

  init_table_widget("ppu");
  init_table_widget("cpu");

  QTimer *timer = new QTimer(this);
  timer->setInterval(1000);

  /* play/pause button behaviour */
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_cpu_state()));
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_ppu_state()));

  connect(this, SIGNAL(play_button_clicked()), timer, SLOT(start()));
  connect(this, SIGNAL(pause_button_clicked()), timer, SLOT(stop()));

  /* step button behaviour */
  connect(this, SIGNAL(step_button_clicked()), timer, SLOT(stop()));
  connect(this, SIGNAL(step_button_clicked()), this, SLOT(refresh_cpu_state()));
  connect(this, SIGNAL(step_button_clicked()), this, SLOT(refresh_ppu_state()));
}

MainWindow::~MainWindow() { delete ui; }


void MainWindow::init_table_widget(std::string which_widget) {
  int rows, cols;
  const QStringList *labels;
  QTableWidget *widget;
  if (which_widget.compare("cpu")) {
    widget = ui->ppuStateTableWidget;
    rows = 13;
    cols = 1;
    labels = &ppu_labels;
  } else {
    widget = ui->cpuStateTableWidget;
    rows = 10;
    cols = 1;
    labels = &cpu_labels;
  }
  
  widget->setRowCount(rows);
  widget->setColumnCount(cols);
  widget->setVerticalHeaderLabels(*labels);
  widget->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  widget->horizontalHeader()->hide();
}


void MainWindow::initOpenGLWidgetNESScreen(NESScreen *s) {
  ui->openGLWidget->initScreen(s);
}

/* until I can get headers to work with QTableView this
   * will have to do...
   */
void MainWindow::refresh_cpu_state() {
  if (cpu_state == nullptr) {
    return;
  }
  ui->cpuStateTableWidget->setItem(0,
				   0, new QTableWidgetItem(QString::number(cpu_state->pc, 16)));
  ui->cpuStateTableWidget->setItem(0,
      1, new QTableWidgetItem(QString::number(cpu_state->cycles)));
  ui->cpuStateTableWidget->setItem(0,
				   2, new QTableWidgetItem(QString::number(cpu_state->a, 16)));
  ui->cpuStateTableWidget->setItem(0,
				   3, new QTableWidgetItem(QString::number(cpu_state->x, 16)));
  ui->cpuStateTableWidget->setItem(0,
				   4, new QTableWidgetItem(QString::number(cpu_state->y, 16)));
  ui->cpuStateTableWidget->setItem(0,
				   5, new QTableWidgetItem(QString::number(cpu_state->sp, 16)));
  ui->cpuStateTableWidget->setItem(0,
				   6, new QTableWidgetItem(QString::number(cpu_state->p, 2)));
  ui->cpuStateTableWidget->setItem(0,
				   7, new QTableWidgetItem(QString::number(cpu_state->opc, 16)));
  ui->cpuStateTableWidget->setItem(0,
      8, new QTableWidgetItem(QString(cpu_state->curr_instruction)));
  ui->cpuStateTableWidget->setItem(0,
      9, new QTableWidgetItem(QString(cpu_state->curr_addr_mode)));
}

void MainWindow::refresh_ppu_state() {
  if (ppu_state == nullptr) {
    return;
  }
  ui->ppuStateTableWidget->setItem(0,
      0, new QTableWidgetItem(QString::number(ppu_state->cycles)));
  ui->ppuStateTableWidget->setItem(0,
      1, new QTableWidgetItem(QString::number(ppu_state->scanline)));
  ui->ppuStateTableWidget->setItem(0,
				   2, new QTableWidgetItem(QString::number(ppu_state->ppuctrl, 2)));
  ui->ppuStateTableWidget->setItem(0,
				   3, new QTableWidgetItem(QString::number(ppu_state->ppumask, 2)));
  ui->ppuStateTableWidget->setItem(0,
				   4, new QTableWidgetItem(QString::number(ppu_state->ppustatus, 2)));
  ui->ppuStateTableWidget->setItem(0,
      5, new QTableWidgetItem(QString::number(ppu_state->w)));
  ui->ppuStateTableWidget->setItem(0,
				   6, new QTableWidgetItem(QString::number(ppu_state->x, 16)));
  ui->ppuStateTableWidget->setItem(0,
				   7, new QTableWidgetItem(QString::number(ppu_state->t, 16)));
  ui->ppuStateTableWidget->setItem(0,
				   8, new QTableWidgetItem(QString::number(ppu_state->v, 16)));
  ui->ppuStateTableWidget->setItem(0,
				   9, new QTableWidgetItem(QString::number(ppu_state->nt_byte, 16)));
  ui->ppuStateTableWidget->setItem(0,
				   10, new QTableWidgetItem(QString::number(ppu_state->at_byte, 16)));
  ui->ppuStateTableWidget->setItem(0,
				   11, new QTableWidgetItem(QString::number(ppu_state->ptt_low, 16)));
  ui->ppuStateTableWidget->setItem(0,
				   12, new QTableWidgetItem(QString::number(ppu_state->ptt_high, 16)));
}

/* ========== Slots ========== */

void MainWindow::done(void) { QApplication::quitOnLastWindowClosed(); }

void MainWindow::error(NESError &e) {
  emit pause_button_clicked();
  refresh_cpu_state();
  refresh_ppu_state();
  show_error(this, e);
  //QApplication::quit();
}

void MainWindow::on_pauseButton_clicked() {
  emit pause_button_clicked();
}

void MainWindow::on_playButton_clicked() {
  emit play_button_clicked();
}

void MainWindow::on_stepButton_clicked() {
  emit step_button_clicked();
}

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
    //memory_dump_text_view->setFixedWidth(memory_dump_text_view->document()->size().width() + 10);
  }
  
  QBoxLayout memory_dump_dialog_layout(QBoxLayout::TopToBottom);
  memory_dump_dialog_layout.addWidget(memory_dump_text_view);

  memory_dump_dialog.setLayout(&memory_dump_dialog_layout);
  memory_dump_dialog.exec();
}
