/* Copyright (C) 2024, 2025  Angus McLean
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QtDebug>

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "core/cppwrapper.hpp"
#include "mainwindow.h"
#include "nescontext.h"
#include "nesscreen.h"

static void log_memory_fetch(uint16_t addr, uint8_t val);
static void log_memory_write(uint16_t addr, uint8_t val);
static void log_cpu_nestest(cpu_state_s *cpu_state);
static void log_cpu(cpu_state_s *cpu_state);
static void log_ppu(ppu_state_s *ppu_state);

static void log_none(const char *format, ...) {}
static void log_memory_none(uint16_t addr, uint8_t val) {}
static void log_cpu_none(cpu_state_s *cpu_state) {}
static void log_ppu_none(ppu_state_s *ppu_state) {}

static void init(MainWindow &w);


int main(int argc, char **argv) {

  QApplication a(argc, argv);
  MainWindow w;
  /* for some reason I have to do this by hand or else
   the programme keeps running after I close the window */
  QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  
  init(w);
  return a.exec();
}


static void init(MainWindow &w) {
  qDebug() << "Initialising MainWindow";
  NESContext *nes_context;
  w.show();

  const std::string rom_filename =
      QFileDialog::getOpenFileName(&w, "Open File", "/home", "NES ROM (*.nes)")
          .toStdString();

  /* for now */
  cpu_register_error_callback(&log_none);
  ppu_register_error_callback(&log_none);
  memory_register_cb(&log_memory_none, MEMORY_CB_FETCH);
  memory_register_cb(&log_memory_none, MEMORY_CB_WRITE);

  /* MainWindow shouldn't really be dealing with this,
     hopefully I can get QTableView to work at some point */

  cpu_register_state_callback(&MainWindow::static_on_cpu_state_update);
  ppu_register_state_callback(&MainWindow::static_on_ppu_state_update);
  
  /* ew ? */
  NESScreen *s = new NESScreen(&w);
  qDebug() << "setOpenGLWidgetNESScreen(s)";
  w.initOpenGLWidgetNESScreen(s);
  
  try {
    qDebug() << "Initialising NESContext";
    nes_context = new NESContext(rom_filename, &NESScreen::put_pixel, &w);
  } catch (NESError &e) {
    show_error(&w, e);
    exit(EXIT_FAILURE);
  }

  
  
  /* when nes execution is done */
  QObject::connect(nes_context, SIGNAL(nes_done()), &w, SLOT(done()));

  QObject::connect(nes_context, SIGNAL(nes_error(NESError &)), &w,
                   SLOT(error(NESError &)));

  QTimer *timer = new QTimer(&w);
  QObject::connect(timer, SIGNAL(timeout()), nes_context, SLOT(nes_step()));
  QObject::connect(&w, SIGNAL(play_button_clicked()), timer, SLOT(start()));
  QObject::connect(&w, SIGNAL(pause_button_clicked()), timer, SLOT(stop()));

  qDebug() << "Init done";
}


static void log_memory_fetch(uint16_t addr, uint8_t val) {
  std::printf("[MEM] Fetched val %02x from addr %04x\n", val, addr);
}


static void log_memory_write(uint16_t addr, uint8_t val) {
  std::printf("[MEM] Wrote val %02x to addr %04x\n", val, addr);
}


static void log_cpu_nestest(cpu_state_s *cpu_state) {
  static int line_num = 1;
  std::printf("%d %04x %02x %s %02x %02x %02x %02x %02x %d\n", line_num++,
              cpu_state->pc, cpu_state->opc, cpu_state->curr_instruction,
              cpu_state->a, cpu_state->x, cpu_state->y, cpu_state->p,
              cpu_state->sp, cpu_state->cycles);
}


static void log_cpu(cpu_state_s *cpu_state) {
  std::printf("[CPU] PC=%04x OPC=%02x %s (%s) A=%02x X=%02x Y=%02x P=%02x "
              "SP=%02x CYC=%d\n",
              cpu_state->pc, cpu_state->opc, cpu_state->curr_instruction,
              cpu_state->curr_addr_mode, cpu_state->a, cpu_state->x,
              cpu_state->y, cpu_state->p, cpu_state->sp, cpu_state->cycles);
}


static void log_ppu(ppu_state_s *ppu_state) {
  std::printf("[PPU] CYC=%d SCL=%d v=%04x t=%04x x=%02x w=%d\n",
              ppu_state->cycles, ppu_state->scanline, ppu_state->v,
              ppu_state->t, ppu_state->x, ppu_state->w);
}
