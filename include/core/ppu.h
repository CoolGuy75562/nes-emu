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


/* allocate memory to and initialise ppu struct, and set function to
 * be used to plot pixels */
int ppu_init(ppu_s **ppu,
             void (*put_pixel_cb)(int i, int j, uint8_t palette_idx, void *data),
             void *put_pixel_data);

/* deallocate memory allocated to ppu with ppu_init */
void ppu_destroy(ppu_s *ppu);


/*============================================================*/
/* These functions are only used in memory.c so probably deserve
 * their own header. */
  
uint8_t ppu_register_fetch(ppu_s *ppu, uint16_t addr);
void ppu_register_write(ppu_s *ppu, uint16_t addr, uint8_t val, uint8_t *to_oamdma);

/* Give the ppu functions to write and read to vram, which depend on mapper and
 * nametable arrangement */
void ppu_register_vram_write_callback(void (*vram_write)(uint16_t, uint8_t, void *),
                                      void *data);
void ppu_register_vram_fetch_callback(uint8_t (*vram_fetch)(uint16_t, void *),
                                      void *data);

/* does one ppu cycle */
void ppu_step(ppu_s *ppu, uint8_t *to_nmi);

#endif
