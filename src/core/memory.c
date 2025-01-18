/* This module is responsible for memory and parsing the ines input file.
 * The ppu memory map is implemented in this module... */

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

#include "core/memory.h"
#include "core/errors.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ines_header_s {
  uint8_t prg_rom_size; /* size of PRG ROM in 16KB units */
  uint8_t chr_rom_size; /* size of CHR ROM in 8KB units */
  uint8_t mapper_n;
  /* bit 6: */
  uint8_t nt_arrangement;
  uint8_t bat_prg_ram;
  uint8_t trainer;
  uint8_t alt_nt_layout;
  /* bit 7: */
  uint8_t vs_unisys;
  uint8_t playchoice_10;
  uint8_t nes_2;
  /* bit 8: */
  uint8_t prg_ram_size;

  uint8_t tv_system;
} ines_header_s;

static int parse_ines_header(char ines_header[16], ines_header_s *header_data,
                             char *e_context);
static int mapper_0(ines_header_s *header_data, FILE *fp, char *e_context);
static void do_three_ppu_steps(uint8_t *to_nmi);

/* ======= Memory Layout =======
 * https://www.nesdev.org/wiki/CPU_memory_map
 *
 * Memory: 0x0000 - 0xFFFF
 * ------- General -------
 *
 * 0x0000 - 0x00FF : Zero page
 * 0x0100 - 0x01FF : Stack
 *
 * 0xFFFA - 0xFFFB : NMI Handler
 * 0xFFFC - 0xFFFD : Reset Vector
 * 0xFFFE - 0xFFFF : IRQ/BRK Vector
 *
 * ------- NES-specific -------
 *
 * 0x0000 - 0x07FF : 2 KB internal RAM
 * 0x0800 - 0x0FFF : Mirror of 0x0000 = 0x07FF
 * 0x1000 - 0x17FF : "                       "
 * 0x1800 - 0x1FFF : "                       "
 * 0x2000 - 0x2007 : NES PPU Registers
 * 0x2008 - 0x3FFF : Mirrors of 0x2000 - 0x2007
 * 0x4000 - 0x4017 : NES APU and I/O Registers
 * 0x4018 - 0x401F : APU and I/O Functionality (normally disabled)
 * 0x4020 - 0xFFFF : Unmapped
 *(0x6000 - 0x7FFF): Usually cartridge RAM if present
 *(0x8000 - 0xFFFF): Usually cartridge ROM and mapper registers
 *
 */
static uint8_t memory_cpu[0x10000] = {0};
static ines_header_s header_data;
/* better to do this than callback in main or
 * passing ppu to cpu to memory: */
static ppu_s *ppu;
static void (*on_fetch)(uint16_t addr, uint8_t val) = NULL;
static void (*on_write)(uint16_t addr, uint8_t val) = NULL;

void memory_register_cb(void (*memory_cb)(uint16_t, uint8_t),
                        memory_cb_e cb_type) {
  switch (cb_type) {
  case MEMORY_CB_FETCH:
    on_fetch = memory_cb;
    break;
  case MEMORY_CB_WRITE:
    on_write = memory_cb;
    break;
  }
}

int memory_init(const char *filename, ppu_s *p, char *e_context) {
  if (on_fetch == NULL || on_write == NULL) {
    return -E_NO_CALLBACK;
  }
  if (p == NULL) {
    return -E_NO_PPU;
  }
  FILE *fp;
  char ines_header_bytes[16];
  int err;

  ppu = p;

  if (filename == NULL) {
    return -E_NO_FILE;
  }
  if ((fp = fopen(filename, "rb")) == NULL) {
    strncpy(e_context, filename, LEN_E_CONTEXT - 1);
    return -E_OPEN_FILE;
  }
  if (fread(ines_header_bytes, sizeof(char), 16, fp) < 16) {
    err = -E_READ_FILE;
    goto error;
  }
  if ((err = parse_ines_header(ines_header_bytes, &header_data, e_context)) <
      0) {
    goto error;
  }
  if (header_data.trainer &&
      fread(memory_cpu + 0x7000, sizeof(char), 512, fp) < 512) {
    err = -E_READ_FILE;
    goto error;
  }

  switch (header_data.mapper_n) {
  case 0:
    err = mapper_0(&header_data, fp, e_context);
    break;
  default:
    err = -E_MAPPER_IMPLEMENTED;
    sprintf(e_context, "%d", header_data.mapper_n);
  }

  fclose(fp);
  return err;

error:
  fclose(fp);
  if (err == -E_READ_FILE) {
    strncpy(e_context, filename, LEN_E_CONTEXT - 1);
  }
  return err;
}

uint16_t memory_init_cpu_pc(void) {
  uint8_t pc_low = memory_cpu[0xFFFC];
  uint8_t pc_high = memory_cpu[0xFFFD];
  return (pc_low | pc_high << 8);
}

void memory_dump(void) {
  FILE *fp;
  fp = fopen("./memory_dump.log", "w");
  size_t i, j;
  char c;
  for (i = 0; i < 0x1000; i++) {
    fprintf(fp, "%3x0: ", (unsigned)i);
    for (j = 0; j < 0x10; j++) {
      fprintf(fp, "%2x ", memory_cpu[0x10 * i + j]);
    }
    fprintf(fp, "|");
    for (j = 0; j < 0x10; j++) {
      c = memory_cpu[0x10 * i + j];
      if (c == 0 || !isprint(c)) {
        fprintf(fp, ".");
      } else {
        fprintf(fp, "%c", c);
      }
    }
    fprintf(fp, "|");
    fprintf(fp, "\n");
  }
  fclose(fp);
  printf("memory dump written to memory_dump.log\n");
}

/*
void ines_header_dump(void) {
  FILE *fp;
  fp = fopen("./header_dump.log", "w");
  fprintf(fp, "prg_rom_size: %d\n", header_data.prg_rom_size);
  fprintf(fp, "chr_rom_size: %d\n", header_data.chr_rom_size);
  fprintf(fp, "mapper_n: %d\n", header_data.mapper_n);
  fprintf(fp, "nt_arrangement: %d\n", header_data.nt_arrangement);
  fprintf(fp, "bat_prg_ram: %d\n", header_data.bat_prg_ram);
  fprintf(fp, "trainer: %d\n", header_data.trainer);
  fprintf(fp, "alt_nt_layout: %d\n", header_data.alt_nt_layout);
  fprintf(fp, "vs_unisys: %d\n", header_data.vs_unisys);
  fprintf(fp, "playchoice_10: %d\n", header_data.playchoice_10);
  fprintf(fp, "nes_2: %d\n", header_data.nes_2);
  fprintf(fp, "prg_ram_size: %d\n", header_data.nes_2);
  fprintf(fp, "tv_system: %d\n", header_data.tv_system);
  fclose(fp);
  printf("ines header information written to header_dump.log\n");
}
*/

void memory_do_oamdma(uint8_t val, uint16_t *cycles, uint8_t *to_nmi) {
  /* if odd cpu cycle need to wait another cycle for dma to read */
  if (*cycles & 1) {
    (*cycles)++;
  }
  uint16_t a_high = val << 8;
  for (int i = 0; i < 0x100; i++) {
    ppu_register_write(ppu, 4, memory_cpu[a_high + i]); /* 4: OAMDATA */
    *cycles += 2;
    do_three_ppu_steps(to_nmi);
  }
}

uint8_t memory_fetch(uint16_t addr, uint8_t *to_nmi) {
  uint8_t val;
  if (addr < 0x2000) {
    val = memory_cpu[addr % 0x800];
    on_fetch(addr % 0x800, val);
  }

  else if (addr < 0x4000) {
    val = ppu_register_fetch(ppu, (addr - 0x2000) % 8);
    on_fetch(0x2000 + (addr - 0x2000) % 8, val);
  }

  else if (addr < 0x4018) {
    val = memory_cpu[addr];
    on_fetch(addr, val);
  }

  else {
    val = memory_cpu[addr];
    on_fetch(addr, val);
  }

  do_three_ppu_steps(to_nmi);
  return val;
}

void memory_write(uint16_t addr, uint8_t val, uint8_t *to_oamdma,
                  uint8_t *to_nmi) {

  if (addr < 0x2000) {
    // memory_cpu[addr] = val;
    memory_cpu[addr % 0x800] = val; /* mirroring */
    on_write(addr % 0x800, val);
  }

  else if (addr < 0x4000) {
    ppu_register_write(ppu, (addr - 0x2000) % 8, val);
    on_write(0x2000 + (addr - 0x2000) % 8, val);
  }

  else if (addr < 0x4018) {
    if (addr == 0x4014) { /* OAMDMA */
      *to_oamdma = 1;
    }
    memory_cpu[addr] = val;
    on_write(addr, val);
  }

  else if (addr < 0x4020) {
    memory_cpu[addr] = val;
    on_write(addr, val);
  }

  else {
    memory_cpu[addr] = val;
    on_write(addr, val);
  }

  do_three_ppu_steps(to_nmi);
}

static int parse_ines_header(char ines_header[16], ines_header_s *header_data,
                             char *e_context) {

  if (strncmp(ines_header, "NES", 3) != 0 || ines_header[3] != 0x1A) {
    strncpy(e_context, ines_header, 4);
    e_context[4] = '\0';
    return -E_INES_SIGNATURE;
  }

  header_data->prg_rom_size = ines_header[4];
  header_data->chr_rom_size = ines_header[5];

  uint8_t flags_6 = ines_header[6];
  header_data->nt_arrangement = flags_6 & 1;
  header_data->bat_prg_ram = flags_6 & 1 << 1;
  header_data->trainer = flags_6 & 1 << 2;
  header_data->alt_nt_layout = flags_6 & 1 << 3;
  uint8_t mapper_n_low_nib = (flags_6 & 0xF0) >> 4;

  uint8_t flags_7 = ines_header[7];
  header_data->vs_unisys = flags_7 & 1;
  header_data->playchoice_10 = flags_7 & 1 << 1;
  header_data->nes_2 = (flags_7 & 0xC) == 8;
  uint8_t mapper_n_high_nib = flags_7 & 0xF0;

  header_data->mapper_n = mapper_n_high_nib | mapper_n_low_nib;

  header_data->prg_ram_size = ines_header[8];

  header_data->tv_system = ines_header[9] & 1;

  return E_NO_ERROR;
}

static int mapper_0(ines_header_s *header_data, FILE *fp, char *e_context) {

  if (header_data->chr_rom_size != 1) {
    sprintf(e_context, "%d", header_data->chr_rom_size);
    return -E_CHR_ROM_SIZE;
  }
  if (header_data->prg_rom_size == 0 || header_data->prg_rom_size > 2) {
    sprintf(e_context, "%d", header_data->prg_rom_size);
    return -E_PRG_ROM_SIZE;
  }
  if (fread(memory_cpu + 0x8000, sizeof(char),
            0x4000 * header_data->prg_rom_size,
            fp) < 0x4000 * header_data->prg_rom_size) {
    return -E_READ_FILE;
  }

  if (header_data->prg_rom_size == 1) {
    /* mirror first 16KB */
    memcpy(memory_cpu + 0xC000, memory_cpu + 0x8000, 0x4000);
  }

  return E_NO_ERROR;
}

static void do_three_ppu_steps(uint8_t *to_nmi) {
  /* three ppu cycles per one cpu cycle */
  for (int i = 0; i < 3; i++) {
    ppu_step(ppu);
    if (!(*to_nmi)) {
      *to_nmi = ppu_check_nmi(ppu);
    }
  }
}
