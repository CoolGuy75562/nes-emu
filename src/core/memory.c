/* This module is responsible for memory and parsing the ines input file.
 * The ppu memory map is implemented in this module... */

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
static int init_mapper_0(ines_header_s *header_data, FILE *fp, char *e_context);
static inline void do_three_ppu_steps(uint8_t *to_nmi);

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

static ppu_s *ppu;
static void (*on_fetch)(uint16_t addr, uint8_t val, void *) = NULL;
static void *on_fetch_data = NULL;
static void (*on_write)(uint16_t addr, uint8_t val, void *) = NULL;
static void *on_write_data = NULL;

void memory_register_cb(void (*memory_cb)(uint16_t, uint8_t, void *),
                        void *data, memory_cb_e cb_type) {
  switch (cb_type) {
  case MEMORY_CB_FETCH:
    on_fetch = memory_cb;
    on_fetch_data = data;
    break;
  case MEMORY_CB_WRITE:
    on_write = memory_cb;
    on_write_data = data;
    break;
  }
}

void memory_unregister_cb(memory_cb_e cb_type) {
  switch (cb_type) {
  case MEMORY_CB_FETCH:
    on_fetch = NULL;
    on_fetch_data = NULL;
    break;
  case MEMORY_CB_WRITE:
    on_write = NULL;
    on_write_data = NULL;
    break;
  }
}

int memory_init(const char *filename, ppu_s *p, char *e_context) {
  if (on_fetch == NULL || on_write == NULL) {
    return -E_NO_CALLBACK;
  }

  if (filename == NULL && p == NULL) { /* no ppu mode for testing cpu */
    ppu = p;
    // memset(memory_cpu, 0, sizeof(memory_cpu));
    return E_NO_ERROR;
  } else if (p == NULL) {
    return -E_NO_PPU;
  } else if (filename == NULL) {
    return -E_NO_FILE;
  }

  FILE *fp;
  char ines_header_bytes[16];
  int err;

  ppu = p;

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
  if (header_data.trainer) {
    if (fread(memory_cpu + 0x7000, sizeof(char), 512, fp) < 512) {
      err = -E_READ_FILE;
      goto error;
    }
  }

  switch (header_data.mapper_n) {
  case 0:
    err = init_mapper_0(&header_data, fp, e_context);
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

#define FPRINTF_CHECK_ERROR(fprintf_statement)                                 \
  do {                                                                         \
    if ((fprintf_statement) < 0) {                                             \
      return -E_WRITE_FILE;                                                    \
    }                                                                          \
  } while (0)

int memory_dump_file(FILE *fp) {
  if (fp == NULL) {
    return -E_NO_FILE;
  }
  size_t i, j;
  char c;
  for (i = 0; i < 0x1000; i++) {
    FPRINTF_CHECK_ERROR(fprintf(fp, "%3x0: ", (unsigned)i));
    for (j = 0; j < 0x10; j++) {
      FPRINTF_CHECK_ERROR(fprintf(fp, "%2x ", memory_cpu[0x10 * i + j]));
    }
    fprintf(fp, "|");
    for (j = 0; j < 0x10; j++) {
      c = memory_cpu[0x10 * i + j];
      if (c == 0 || !isprint(c)) {
        FPRINTF_CHECK_ERROR(fprintf(fp, "."));
      } else {
        FPRINTF_CHECK_ERROR(fprintf(fp, "%c", c));
      }
    }
    FPRINTF_CHECK_ERROR(fprintf(fp, "|"));
    FPRINTF_CHECK_ERROR(fprintf(fp, "\n"));
  }
  return E_NO_ERROR;
}
#undef FPRINTF_CHECK_ERROR

int memory_dump_string(char *dump, size_t dump_len) {
  if (dump == NULL) {
    return -E_NO_STRING;
  }
  char buf[2048];
  size_t offset, read = 0;
  size_t i, j;
  char c;

  /* What if sprintf returns negative? */
  for (i = 0; i < 0x1000; i++) {
    offset = 0;
    offset += sprintf(buf + offset, "%3x0: ", (unsigned)i);
    for (j = 0; j < 0x10; j++) {
      offset += sprintf(buf + offset, "%2x ", memory_cpu[0x10 * i + j]);
    }
    offset += sprintf(buf + offset, "|");
    for (j = 0; j < 0x10; j++) {
      c = memory_cpu[0x10 * i + j];
      if (c == 0 || !isprint(c)) {
        offset += sprintf(buf + offset, ".");
      } else {
        offset += sprintf(buf + offset, "%c", c);
      }
    }
    offset += sprintf(buf + offset, "|\n");

    read += offset;
    if (read > dump_len) {
      return -E_BUF_SIZE;
    } else {
      strncpy(dump, buf, offset);
      dump += offset;
    }
  }
  *dump = '\0';
  return E_NO_ERROR;
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
    /* this should be done in cpu.c so we can do dummy fetch */
    (*cycles)++;
  }
  static uint8_t does_nothing;
  uint16_t a_high = val << 8;
  for (int i = 0; i < 0x100; i++) {
    ppu_register_write(ppu, 4, memory_cpu[a_high + i],
                       &does_nothing); /* 4: OAMDATA */
    on_write(0x2004, memory_cpu[a_high + i], on_write_data);
    *cycles += 2;
    do_three_ppu_steps(to_nmi);
    do_three_ppu_steps(to_nmi);
  }
}

uint8_t memory_fetch(uint16_t addr, uint8_t *to_nmi) {
  uint8_t val;
  uint16_t effective_addr;
  if (ppu == NULL) { /* no ppu mode */
    val = memory_cpu[addr];
    effective_addr = addr;

  } else {
    if (addr < 0x2000) {
      val = memory_cpu[addr % 0x800];
      effective_addr = addr % 0x800;
    }

    else if (addr < 0x4000) {
      val = ppu_register_fetch(ppu, addr % 8);
      effective_addr = 0x2000 + (addr % 8);
    }

    else if (addr == 0x4014) {
      val = ppu_register_fetch(ppu, 0x14);
      effective_addr = addr;
    }

    else if (addr < 0x4018) {
      val = memory_cpu[addr];
      effective_addr = addr;
    }

    else if (addr < 0x8000) {
      val = memory_cpu[addr];
      effective_addr = addr;
    }

    else if (addr < 0xC000 && header_data.prg_rom_size == 1) {
      val = memory_cpu[addr + 0x4000];
      effective_addr = addr + 0x4000;
    }

    else {
      val = memory_cpu[addr];
      effective_addr = val;
    }
    do_three_ppu_steps(to_nmi);
  }
  on_fetch(effective_addr, val, on_fetch_data);
  return val;
}

void memory_write(uint16_t addr, uint8_t val, uint8_t *to_oamdma,
                  uint8_t *to_nmi) {

  uint16_t effective_addr;
  if (ppu == NULL) { /* no ppu mode */
    memory_cpu[addr] = val;
    effective_addr = addr;

  } else {
    if (addr < 0x2000) {
      // memory_cpu[addr] = val;
      memory_cpu[addr % 0x800] = val; /* mirroring */
      effective_addr = addr % 0x800;
    }

    else if (addr < 0x4000) {
      ppu_register_write(ppu, addr % 8, val, to_oamdma);
      effective_addr = 0x2000 + (addr % 8);
    }

    else if (addr == 0x4014) {
      ppu_register_write(ppu, 0x14, val, to_oamdma);
      effective_addr = addr;
    }

    else if (addr < 0x4020) {
      memory_cpu[addr] = val;
      effective_addr = addr;
    }

    else if (addr < 0x8000) {
      memory_cpu[addr] = val;
      effective_addr = addr;
    }

    else if (addr < 0xC000 && header_data.prg_rom_size == 1) {
      memory_cpu[addr + 0x4000] = val;
      effective_addr = addr + 0x4000;
    }
    
    else {
      memory_cpu[addr] = val;
      effective_addr = addr;
    }

    do_three_ppu_steps(to_nmi);
  }
  on_write(effective_addr, val, on_write_data);
}

void memory_init_harte_test_case(const uint16_t *addrs, const uint8_t *vals,
                                 size_t length) {
  memset(memory_cpu, 0, sizeof(memory_cpu));
  for (size_t i = 0; i < length; i++) {
    memory_cpu[addrs[i]] = vals[i];
  }
}

void memory_reset_harte(const uint16_t *addrs, uint8_t *final_vals,
                        size_t length) {
  for (size_t i = 0; i < length; i++) {
    final_vals[i] = memory_cpu[addrs[i]];
  }
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

static int init_mapper_0(ines_header_s *header_data, FILE *fp,
                         char *e_context) {

  /* technically should be != 1 but I've come across some mapper 0
   * ROMS with no chr rom
   */
  if (header_data->chr_rom_size > 1) {
    sprintf(e_context, "%d", header_data->chr_rom_size);
    return -E_CHR_ROM_SIZE;
  }
  if (header_data->prg_rom_size == 0 || header_data->prg_rom_size > 2) {
    sprintf(e_context, "%d", header_data->prg_rom_size);
    return -E_PRG_ROM_SIZE;
  }
  size_t prg_rom_bytes = 0x4000 * header_data->prg_rom_size;
  if (fread(memory_cpu + 0x8000, 1, prg_rom_bytes, fp) != prg_rom_bytes) {
    return -E_READ_FILE;
  }

  /* mirror first 16kb */
  if (header_data->prg_rom_size == 1) {
    memcpy(memory_cpu + 0xC000, memory_cpu + 0x8000, 0x4000);
  }

  if (ppu != NULL) {

    uint8_t chr_rom[0x2000] = {0};

    if (fread(chr_rom, sizeof(uint8_t), 0x2000 * header_data->chr_rom_size,
              fp) < 0x2000 * header_data->chr_rom_size) {
      return -E_READ_FILE;
    }

    ppu_init_chr_rom(chr_rom, 0x2000 * header_data->chr_rom_size);
  }
  return E_NO_ERROR;
}

static inline void do_three_ppu_steps(uint8_t *to_nmi) {
  /* three ppu cycles per one cpu cycle */
  for (int i = 0; i < 3; i++) {
    ppu_step(ppu, to_nmi);
  }
}
