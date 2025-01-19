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
#include <QString>
#include <QTimer>
#include <QMessageBox>

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

extern "C" {
#include <unistd.h>
}

#include "core/cppwrapper.hpp"
#include "mainwindow.h"

static inline void show_error(QWidget *parent, NESError &e);
static void nes_init(std::unique_ptr<cpu_s, void (*)(cpu_s *)> &cpu_ptr,
                     std::unique_ptr<ppu_s, void (*)(ppu_s *)> &ppu_ptr,
                     std::string &rom_filename);
static void step_cpu(QApplication *a);

static void log_memory_fetch(uint16_t addr, uint8_t val);
static void log_memory_write(uint16_t addr, uint8_t val);
static void log_cpu_nestest(cpu_state_s *cpu_state);
static void log_cpu(cpu_state_s *cpu_state);
static void log_ppu(ppu_state_s *ppu_state);

static void put_pixel(int i, int j, uint8_t pallete_idx) {}
static void log_none(const char *format, ...) {}
static void log_memory_none(uint16_t addr, uint8_t val) {}
static void log_cpu_none(cpu_state_s *cpu_state) {}
static void log_ppu_none(ppu_state_s *ppu_state) {}

int main(int argc, char **argv) {

  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  std::string rom_filename =
      QFileDialog::getOpenFileName(&w, "Open File", "/home", "NES ROM (*.nes)")
          .toStdString();

  std::unique_ptr<cpu_s, void (*)(cpu_s *)> cpu_ptr(nullptr, &cpu_destroy);
  std::unique_ptr<ppu_s, void (*)(ppu_s *)> ppu_ptr(nullptr, &ppu_destroy);

  try {
    nes_init(cpu_ptr, ppu_ptr, rom_filename);
  } catch (NESError &e) {
    show_error(&w, e);
    exit(EXIT_FAILURE);
  }

  QTimer *timer = new QTimer(&w);
  QObject::connect(timer, &QTimer::timeout, [&cpu_ptr, &w] {
    try {
      if (nes_cpu_exec(cpu_ptr.get()) == 0) {
	QCoreApplication::quit();
      }
    } catch (NESError &e) {
      show_error(&w, e);
      QCoreApplication::quit();
    }
  });
  timer->start();
  
  return a.exec();
}

static void nes_init(std::unique_ptr<cpu_s, void (*)(cpu_s *)> &cpu_ptr,
                     std::unique_ptr<ppu_s, void (*)(ppu_s *)> &ppu_ptr,
                     std::string &rom_filename) {

  cpu_s *cpu = nullptr;
  ppu_s *ppu = nullptr;

  cpu_register_error_callback(&log_none);
  cpu_register_state_callback(&log_cpu_none);
  ppu_register_error_callback(&log_none);
  ppu_register_state_callback(&log_ppu_none);
  memory_register_cb(&log_memory_none, MEMORY_CB_FETCH);
  memory_register_cb(&log_memory_none, MEMORY_CB_WRITE);
  
  nes_ppu_init(&ppu, &put_pixel);
  ppu_ptr.reset(ppu);

  nes_memory_init(rom_filename, ppu);

  nes_cpu_init(&cpu, 1);
  cpu_ptr.reset(cpu);
}

static inline void show_error(QWidget *parent, NESError &e) {
  std::cout << e.what() << std::endl;
  QMessageBox::critical(parent, "Error", e.what());
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
