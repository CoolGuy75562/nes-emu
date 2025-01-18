/* Copyright (C) 2024  Angus McLean
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

//#include "mainwindow.h"

#include <cstdio>
#include <iostream>
#include <memory>

#include "core/cppwrapper.hpp"

extern "C" {
#include <unistd.h>
}

static void log_cpu_nestest(cpu_state_s *cpu_state);
static void log_ppu_none(ppu_state_s *ppu_state);
static void log_memory_none(uint16_t addr, uint8_t val);
static void log_none(const char *, ...);

static void put_pixel(int i, int j, uint8_t pallete_idx);

int main(int argc, char **argv) {



  int opt, exec_status, nestest = 1;
  cpu_s *cpu = nullptr;
  ppu_s *ppu = nullptr;
  const char *rom_filename = "../tests/nestest.nes";

  cpu_register_state_callback(&log_cpu_nestest);

  /* just for now we don't care about the other callbacks */
  cpu_register_error_callback(&log_none);
  ppu_register_state_callback(&log_ppu_none);
  ppu_register_error_callback(&log_none);
  memory_register_cb(&log_memory_none, MEMORY_CB_FETCH);
  memory_register_cb(&log_memory_none, MEMORY_CB_WRITE);

  try {
    nes_ppu_init(&ppu, &put_pixel);
    std::unique_ptr<ppu_s, void (*)(ppu_s *)> ppu_ptr(ppu, &ppu_destroy);

    nes_memory_init(rom_filename, ppu_ptr.get());

    nes_cpu_init(&cpu, nestest);
    std::unique_ptr<cpu_s, void (*)(cpu_s *)> cpu_ptr(cpu, &cpu_destroy);

    /* begin the main loop */
    nes_cpu_exec(cpu_ptr.get());

  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static void log_cpu_nestest(cpu_state_s *cpu_state) {
  static int line_num = 1;
  std::printf("%d %04x %02x %s %02x %02x %02x %02x %02x %d\n", line_num++,
              cpu_state->pc, cpu_state->opc, cpu_state->curr_instruction,
              cpu_state->a, cpu_state->x, cpu_state->y, cpu_state->p,
              cpu_state->sp, cpu_state->cycles);
}

static void log_ppu_none(ppu_state_s *ppu_state) {}
static void log_memory_none(uint16_t addr, uint8_t val) {}
static void log_none(const char *, ...) {}

static void put_pixel(int i, int j, uint8_t palette_idx) {}
