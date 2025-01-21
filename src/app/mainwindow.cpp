#include <QMessageBox>
#include <QtDebug>
#include <QTimer>
#include <iostream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "nesscreen.h"

static MainWindow *w; // not ideal

void show_error(QWidget *parent, NESError &e) {
  std::cout << e.what() << std::endl;
  QMessageBox::critical(parent, "Error", e.what());
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
  /* until I can get headers to work with QTableView this
   * will have to do...
   */
  ui->cpuStateTableWidget->setRowCount(10);
  ui->cpuStateTableWidget->setColumnCount(1);
  ui->cpuStateTableWidget->setVerticalHeaderLabels(cpu_labels);
  ui->cpuStateTableWidget->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  ui->cpuStateTableWidget->horizontalHeader()->hide();
  
  ui->ppuStateTableWidget->setRowCount(13);
  ui->ppuStateTableWidget->setColumnCount(1);
  ui->ppuStateTableWidget->setVerticalHeaderLabels(ppu_labels);
  ui->ppuStateTableWidget->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  ui->ppuStateTableWidget->horizontalHeader()->hide();

  w = this;

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_cpu_state()));
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_ppu_state()));
  timer->start(32);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initOpenGLWidgetNESScreen(NESScreen *s) {
  ui->openGLWidget->initScreen(s);
}
/* this is a hack to pass a member function as a C-style callback */
void MainWindow::static_on_cpu_state_update(cpu_state_s *cpu_state) {
  // w->on_cpu_state_update(cpu_state);
  w->cpu_state = cpu_state;
}

void MainWindow::static_on_ppu_state_update(ppu_state_s *ppu_state) {
  //  w->on_ppu_state_update(ppu_state);
  w->ppu_state = ppu_state;
}

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
  show_error(this, e);
  QApplication::quit();
}

void MainWindow::on_pauseButton_clicked() {
  emit pause_button_clicked();
}

void MainWindow::on_playButton_clicked() {
  emit play_button_clicked();
}
