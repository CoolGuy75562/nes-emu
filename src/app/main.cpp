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

#include <cstdio>
#include <iostream>
#include <memory>

#include "core/cppwrapper.hpp"

extern "C" {
#include <unistd.h>
}

static void print_usage(void);

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

  int opt, nestest = 0, ignore_cpu = 0, ignore_ppu = 0, ignore_memory = 0;
  cpu_s *cpu = nullptr;
  ppu_s *ppu = nullptr;
  const char *rom_filename = NULL;

  while ((opt = getopt(argc, argv, ":r:ncpm")) != -1) {
    switch (opt) {
    case 'r':
      rom_filename = optarg;
      break;
    case 'n':
      nestest = 1;
      break;
    case 'c':
      ignore_cpu = 1;
      break;
    case 'p':
      ignore_ppu = 1;
      break;
    case 'm':
      ignore_memory = 1;
      break;
    case ':':
      fprintf(stderr, "Option -%c requires an operand\n", optopt);
      print_usage();
      return EXIT_FAILURE;
    case '?':
      fprintf(stderr, "Unrecognised option -%c\n", optopt);
      print_usage();
      return EXIT_FAILURE;
    }
  }

  if (ignore_cpu) {
    cpu_register_state_callback(&log_cpu_none);
  } else if (nestest) {
    cpu_register_state_callback(&log_cpu_nestest);
  } else {
    cpu_register_state_callback(&log_cpu);
  }

  if (nestest || ignore_ppu) {
    ppu_register_state_callback(&log_ppu_none);
  } else {
    ppu_register_state_callback(&log_ppu);
  }

  if (nestest || ignore_memory) {
    memory_register_cb(&log_memory_none, MEMORY_CB_FETCH);
    memory_register_cb(&log_memory_none, MEMORY_CB_WRITE);
  } else {
    memory_register_cb(&log_memory_fetch, MEMORY_CB_FETCH);
    memory_register_cb(&log_memory_write, MEMORY_CB_WRITE);
  }

  /* Error callbacks don't do much right now */
  cpu_register_error_callback(&log_none);
  ppu_register_error_callback(&log_none);
  
  try {
    nes_ppu_init(&ppu, &put_pixel);
    std::unique_ptr<ppu_s, void (*)(ppu_s *)> ppu_ptr(ppu, &ppu_destroy);

    nes_memory_init(rom_filename, ppu_ptr.get());

    nes_cpu_init(&cpu, nestest);
    std::unique_ptr<cpu_s, void (*)(cpu_s *)> cpu_ptr(cpu, &cpu_destroy);

    /* begin the main loop */
    nes_cpu_exec(cpu_ptr.get());

  } catch (NESError &e) {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static void print_usage(void) { std::cout << "usage" << std::endl; }

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
  
