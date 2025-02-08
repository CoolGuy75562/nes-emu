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

#ifndef PPU_H_
#define PPU_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct ppu_s ppu_s;

/* ppu state, passed to ppu_state_cb */
typedef struct ppu_state_s {
  uint16_t cycles;
  uint16_t scanline;
  /* status registers */
  uint8_t ppuctrl;
  uint8_t ppumask;
  uint8_t ppustatus;
  /* internal registers */
  uint8_t w;
  uint8_t x;
  uint16_t t;
  uint16_t v;
  /* shift registers */
  uint8_t nt_byte;
  uint8_t at_byte;
  uint8_t ptt_low;
  uint8_t ptt_high;
} ppu_state_s;

/* Do not call cpu_exec() after unregistering a callback! */

/* TODO: error propagation from ppu->memory->cpu so cpu_exec() can
 * return -E_NO_CALLBACK when this happens
 */

/* register callback for register state update */
void ppu_register_state_callback(void (*ppu_state_cb)(const ppu_state_s *, void *),
                                 void *data);
void ppu_unregister_state_callback(void);

/* register callback for error logging */
void ppu_register_error_callback(void (*log_error_cb)(const char *, ...));
void ppu_unregister_error_callback(void);


int ppu_init_no_alloc(ppu_s *ppu,
                      void (*put_pixel_cb)(int i, int j, uint8_t palette_idx,
                                           void *data),
                      void *put_pixel_data);

/* allocate memory to and initialise ppu struct, and set function to
 * be used to plot pixels */
int ppu_init(ppu_s **ppu,
             void (*put_pixel_cb)(int i, int j, uint8_t palette_idx, void *data),
             void *put_pixel_data);

/* deallocate memory allocated to ppu with ppu_init */
void ppu_destroy(ppu_s *ppu);

void ppu_draw_pattern_table(uint8_t is_right,
                            void (*put_pixel)(int, int, uint8_t, void *),
			    void *data);

/*============================================================*/
/*                      Do not touch:                         */ /*============================================================*/
typedef struct ppu_s {
  /* memory-mapped registers */
  uint8_t ppuctrl;     /* 0x2000 */
  uint8_t ppumask;     /* 0x2001 */
  uint8_t oamaddr;     /* 0x2003 */
  uint8_t ppuscroll_x; /* 0x2005 */
  uint8_t ppuscroll_y;
  uint8_t ppuaddr_high; /* 0x2006 */
  uint8_t ppuaddr_low;
  uint8_t oamdma;     /* 0x4014 */
  uint8_t ppustatus;  /* 0x2002 */
  uint8_t oamdata;    /* 0x2004 */
  uint8_t ppudata;    /* 0x2007 */
  uint8_t ppudata_rb; /* ppudata read buffer */

  /* data bus:
   * set to value written to any memory-mapped register.
   * reading a write-only register returns value on data bus.
   * unused bits in ppustatus are corresponding bits on data bus,
   * i.e., ppu_db = ABCDEFGH, ppustatus = 123xxxxx,
   *       read ppustatus -> 123DEFGH
   * (I think)
   */
  uint8_t ppu_db;

  /* internal registers */
  uint8_t w;
  uint8_t x;
  uint16_t t;
  uint16_t v;

  /* internal registers for tile data */
  uint8_t nt_byte;
  uint8_t at_byte;
  uint8_t ptt_low;
  uint8_t ptt_high;

  /* tile shift registers */
  uint16_t at_shift;
  uint16_t ptt_shift_low;
  uint16_t ptt_shift_high;

  /* other things to keep track of */
  uint16_t cycles;
  uint16_t scanline;
  uint32_t total_cycles;
  uint8_t ready_to_write;
  uint8_t frame_parity;        /* 0: even, 1: odd */
  uint8_t to_toggle_rendering; /* counts down each dot, toggles rendering when
                                  reaches 1 */
  uint8_t nmi_occurred;
} ppu_s;

#endif
