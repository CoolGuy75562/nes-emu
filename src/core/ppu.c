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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/errors.h"
#include "core/ppu.h"
#include "ppup.h"

#define MASK_PPUCTRL_NAMETABLE 0x3
#define MASK_PPUCTRL_INCREMENT 0x4
#define MASK_PPUCTRL_ST_SELECT 0x8
#define MASK_PPUCTRL_BT_SELECT 0x10
#define MASK_PPUCTRL_SPRITE_HEIGHT 0x20
#define MASK_PPUCTRL_PPU_MASTER_SLAVE 0x40
#define MASK_PPUCTRL_NMI_ENABLE 0x80

#define MASK_PPUMASK_GREYSCALE 0x1
#define MASK_PPUMASK_BG_LC_ENABLE 0x2
#define MASK_PPUMASK_SPRITE_LC_ENABLE 0x4
#define MASK_PPUMASK_BG_R_ENABLE 0x8
#define MASK_PPUMASK_SPRITE_R_ENABLE 0x10
#define MASK_PPUMASK_COLOR_EMPHASIS 0xE0

#define MASK_PPUSTATUS_SPRITE_OVERFLOW 0x20
#define MASK_PPUSTATUS_SPRITE_0_HIT 0x40
#define MASK_PPUSTATUS_VBLANK 0x80
#define MASK_PPUSTATUS_ALL 0xE0

#define MASK_PPUADDR_HIGH 0x3F

#define MASK_T_V_COARSE_X 0x1F
#define MASK_T_V_COARSE_Y 0x3E0
#define MASK_T_V_NAMETABLE 0xC00
#define MASK_T_V_FINE_Y 0x7000
#define MASK_T_V_ADDR_ALL 0x3FFF   /* 0011111111111111 */
#define MASK_T_V_SCROLL_ALL 0x7FFF /* 0111111111111111 */
#define MASK_T_V_HORI 0x41F        /* ....N.....XXXXX */
#define MASK_T_V_VERT 0x7BE0       /* YYYN.YYYYY..... */
#define MASK_PPUSCROLL_FINE 0x7

#define IGNORE_REG_WRITE_CYCLES 29657

static inline void state_init(const ppu_s *ppu);
static inline void state_update(const ppu_s *ppu);

static inline void increment_ppu(ppu_s *ppu);
static inline void update_nmi(ppu_s *ppu);
static inline void background_step(ppu_s *ppu);
static inline void sprite_step(ppu_s *ppu);
static inline void tile_data_fetch(ppu_s *ppu, uint8_t offset);
static inline void render_pixel(const ppu_s *ppu);
static inline void nt_byte_fetch(ppu_s *ppu);
static inline void at_byte_fetch(ppu_s *ppu);
static inline void ptt_low_byte_fetch(ppu_s *ppu);
static inline void ptt_high_byte_fetch(ppu_s *ppu);
static inline void inc_hori_v(ppu_s *ppu);
static inline void inc_vert_v(ppu_s *ppu);

static inline uint8_t ppustatus_fetch(ppu_s *ppu);
static inline uint8_t oamdata_fetch(ppu_s *ppu);
static inline uint8_t ppudata_fetch(ppu_s *ppu);

static inline void ppuctrl_write(ppu_s *ppu, uint8_t val);
static inline void ppumask_write(ppu_s *ppu, uint8_t val);
static inline void oamaddr_write(ppu_s *ppu, uint8_t val);
static inline void oamdata_write(ppu_s *ppu, uint8_t val);
static inline void ppuscroll_write(ppu_s *ppu, uint8_t val);
static inline void ppuaddr_write(ppu_s *ppu, uint8_t val);
static inline void ppudata_write(ppu_s *ppu, uint8_t val);
static inline void oamdma_write(ppu_s *ppu, uint8_t val);

/* global state */
static ppu_state_s ppu_state;
static uint8_t memory_oam[0x100] = {0};
static uint8_t memory_secondary_oam[32] = {0};

/* callbacks */
static uint8_t default_fetch(uint16_t a) { return 0;}
static void default_write(uint16_t a, uint8_t v) {}

static void (*log_error)(const char *, ...) = NULL;
static void (*on_ppu_state_update)(const ppu_state_s *ppu_state,
                                   void *data) = NULL;
static void (*put_pixel)(int i, int j, uint8_t palette_idx, void *data) = NULL;
static uint8_t (*vram_fetch)(uint16_t addr) = &default_fetch;
static void (*vram_write)(uint16_t addr, uint8_t val) = &default_write;

/* callback data */
static void *on_ppu_state_update_data = NULL;
static void *put_pixel_data = NULL;

/*----------------------------------------------------------------------------*/

void ppu_draw_pattern_table(uint8_t is_right,
                            void (*put_pixel)(int, int, uint8_t, void *),
                            void *data) {
  for (uint16_t tile_y = 0; tile_y < 0x10; tile_y++) {
    for (uint16_t tile_x = 0; tile_x < 0x10; tile_x++) {
      uint16_t tile_offset =
          tile_y * 0x100 + tile_x * 0x10; // offset into next tile
      for (uint8_t row = 0; row < 8; row++) {

        uint8_t tile_low =
            vram_fetch(is_right * 0x1000 + tile_offset + row);
        uint8_t tile_high = vram_fetch(
            is_right * 0x1000 + tile_offset + row + 8);

        for (uint8_t col = 0; col < 8; col++) {

	  uint8_t palette_idx = 0x3F00 + (((tile_low & 1) << 1) | (tile_high & 1));
	  tile_low >>= 1; tile_high >>=1;

          put_pixel(tile_y * 8 + row,
                    tile_x * 8 + (7 - col),
                    palette_idx,
                    data);
	}
      }
    }
  }
}

void ppu_register_state_callback(void (*ppu_state_cb)(const ppu_state_s *,
                                                      void *),
                                 void *data) {
  on_ppu_state_update = ppu_state_cb;
  on_ppu_state_update_data = data;
}

void ppu_unregister_state_callback(void) { on_ppu_state_update = NULL; }

void ppu_register_error_callback(void (*log_error_cb)(const char *, ...)) {
  log_error = log_error_cb;
}

void ppu_unregister_error_callback(void) { log_error = NULL; }

void ppu_register_vram_fetch_callback(uint8_t (*callback)(uint16_t)) {
  vram_fetch = callback;
}

void ppu_register_vram_write_callback(void (*callback)(uint16_t, uint8_t)) {
  vram_write = callback;
}

int ppu_init_no_alloc(ppu_s *ppu, void (*put_pixel_cb)(int, int, uint8_t, void *),
             void *data) {
  put_pixel = put_pixel_cb;
  put_pixel_data = data;
  if (on_ppu_state_update == NULL || log_error == NULL) {
    return -E_NO_CALLBACK;
  }
  ppu->ppustatus = 0xA0;
  state_init(ppu);
  // on_ppu_state_update(&ppu_state, on_ppu_state_update_data);
  return E_NO_ERROR;
}

int ppu_init(ppu_s **p, void (*put_pixel_cb)(int, int, uint8_t, void *),
             void *data) {
  put_pixel = put_pixel_cb;
  put_pixel_data = data;
  if (on_ppu_state_update == NULL || log_error == NULL) {
    return -E_NO_CALLBACK;
  }
  if ((*p = calloc(1, sizeof(ppu_s))) == NULL) {
    return -E_MALLOC;
  }
  ppu_s *ppu = *p;
  ppu->ppustatus = 0xA0;
  state_init(ppu);
  // on_ppu_state_update(&ppu_state, on_ppu_state_update_data);
  return E_NO_ERROR;
}

void ppu_destroy(ppu_s *ppu) { free(ppu); }

void ppu_step(ppu_s *ppu, uint8_t *to_nmi) {
  static uint8_t is_rendering;
  
  is_rendering = ((ppu->ppumask &
                   (MASK_PPUMASK_BG_R_ENABLE | MASK_PPUMASK_SPRITE_R_ENABLE)));
  

  
  /* ppuctrl write could have set nmi */
  *to_nmi |= ppu->nmi_occurred;

  if (is_rendering) {
    background_step(ppu);
    sprite_step(ppu);
    if (ppu->scanline < 240 && ppu->cycles < 256) {
    render_pixel(ppu);
    }
  }

  // Start of vblank
  if (ppu->scanline == 241 && ppu->cycles == 1) {
    ppu->ppustatus |= MASK_PPUSTATUS_VBLANK;
    update_nmi(ppu);
    *to_nmi |= ppu->nmi_occurred;
  }

  // End of vblank
  if (ppu->scanline == 261 && ppu->cycles == 1) {
    ppu->ppustatus &= ~MASK_PPUSTATUS_ALL;
    update_nmi(ppu);
    *to_nmi |= ppu->nmi_occurred;
  }

  increment_ppu(ppu);
  /*
  ppu->total_cycles++;
  if (!ppu->ready_to_write && ppu->total_cycles > 3 * IGNORE_REG_WRITE_CYCLES) {
    ppu->ready_to_write = 1;
  }
  */
  
  /* or this cycle could have set nmi  */
  *to_nmi |= ppu->nmi_occurred;
  ppu->nmi_occurred = 0;
  state_update(ppu);
  on_ppu_state_update(&ppu_state, on_ppu_state_update_data);
}

static inline void update_nmi(ppu_s *ppu) {
  ppu->nmi_occurred = (ppu->ppuctrl & MASK_PPUCTRL_NMI_ENABLE) &&
                      (ppu->ppustatus & MASK_PPUSTATUS_VBLANK);
}

uint8_t ppu_register_fetch(ppu_s *ppu, uint16_t addr) {
  switch (addr) {
  case 0x2002:
    ppu->ppu_db = ppustatus_fetch(ppu);
    break;
  case 0x2004:
    ppu->ppu_db = oamdata_fetch(ppu);
    break;
  case 0x2007:
    ppu->ppu_db = ppudata_fetch(ppu);
    break;
  }
  return ppu->ppu_db;
}

void ppu_register_write(ppu_s *ppu, uint16_t addr, uint8_t val,
                        uint8_t *to_oamdma) {
  switch (addr) {
  case 0x2000:
    // if (ppu->ready_to_write) {
    ppuctrl_write(ppu, val);
    //  }
    break;
  case 0x2001:
    // if (ppu->ready_to_write) {
    ppumask_write(ppu, val);
    //   }
    break;
  case 0x2002:
    ppu->ppu_db = val;
    break;
  case 0x2003:
    oamaddr_write(ppu, val);
    break;
  case 0x2004:
    oamdata_write(ppu, val);
    break;
  case 0x2005:
    //   if (ppu->ready_to_write) {
    ppuscroll_write(ppu, val);
    //    }
    break;
  case 0x2006:
    //   if (ppu->ready_to_write) {
    ppuaddr_write(ppu, val);
    //    }
    break;
  case 0x2007:
    ppudata_write(ppu, val);
    break;
  case 0x4014:
    *to_oamdma = 1;
    oamdma_write(ppu, val);
    break;
  }
}

static void state_init(const ppu_s *ppu) { state_update(ppu); }

static void state_update(const ppu_s *ppu) {
  ppu_state.cycles = ppu->cycles;
  ppu_state.scanline = ppu->scanline;
  ppu_state.ppuctrl = ppu->ppuctrl;
  ppu_state.ppumask = ppu->ppumask;
  ppu_state.ppustatus = ppu->ppustatus;
  ppu_state.w = ppu->w;
  ppu_state.x = ppu->x;
  ppu_state.t = ppu->t;
  ppu_state.v = ppu->v;
  ppu_state.nt_byte = ppu->nt_byte;
  ppu_state.at_byte = ppu->at_byte;
  ppu_state.ptt_low = ppu->ptt_low;
  ppu_state.ptt_high = ppu->ptt_high;
}

/*------------------------------tile
 * fetching--------------------------------*/
static void nt_byte_fetch(ppu_s *ppu) {
  /* according to wiki: */
  ppu->nt_byte = vram_fetch(0x2000 | (ppu->v & 0xFFF));
}

static void at_byte_fetch(ppu_s *ppu) {
 /* according to wiki: */
  uint16_t addr = 0x23C0 | (ppu->v & MASK_T_V_NAMETABLE) |
                  ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07);
  ppu->at_byte = vram_fetch(addr);
}

static void ptt_low_byte_fetch(ppu_s *ppu) {
  uint16_t addr = (ppu->v & MASK_T_V_FINE_Y) >> 12;
  addr += (ppu->nt_byte << 4);
  addr += (ppu->ppuctrl & MASK_PPUCTRL_BT_SELECT) ? 0x1000 : 0;
  ppu->ptt_low = vram_fetch(addr);
}

static void ptt_high_byte_fetch(ppu_s *ppu) {
  uint16_t addr = ((ppu->v & MASK_T_V_FINE_Y) >> 12) + 8;
  addr += (ppu->nt_byte << 4);
  addr += (ppu->ppuctrl & MASK_PPUCTRL_BT_SELECT) ? 0x1000 : 0;
  ppu->ptt_high = vram_fetch(addr);
}

/*-------------------------memory-mapped register reads
 * -----------------------*/

static uint8_t ppustatus_fetch(ppu_s *ppu) {
  uint8_t val = (ppu->ppustatus & MASK_PPUSTATUS_ALL) |
                (ppu->ppu_db & ~MASK_PPUSTATUS_ALL);

  ppu->w = 0;
  ppu->ppustatus &= ~MASK_PPUSTATUS_VBLANK; /* clear vblank flag */
  update_nmi(ppu);
  return val;
}

static uint8_t oamdata_fetch(ppu_s *ppu) { return memory_oam[ppu->oamaddr]; }

static uint8_t ppudata_fetch(ppu_s *ppu) {
  uint8_t val = ppu->ppudata_rb;
  ppu->ppudata_rb = vram_fetch(ppu->v & MASK_T_V_ADDR_ALL);

  /* Increment VRAM address by 1 or 32, depending on PPUCTRL second bit */
  ppu->v += (ppu->ppuctrl & MASK_PPUCTRL_INCREMENT) ? 32 : 1;

  return val;
}

/*=============================Register
 * Writes=================================*/

static void ppuctrl_write(ppu_s *ppu, uint8_t val) {
  ppu->ppuctrl = val;
  /* ppuctrl = ......GH -> t = ... GH ..... ..... */
  ppu->t = /* double checked, this is right */
      (ppu->t & ~MASK_T_V_NAMETABLE) | ((val & MASK_PPUCTRL_NAMETABLE) << 10);
  update_nmi(ppu);
  ppu->ppu_db = val;
}

static void ppumask_write(ppu_s *ppu, uint8_t val) {
  ppu->ppumask = val;
  ppu->ppu_db = val;
}

static void oamaddr_write(ppu_s *ppu, uint8_t val) {
  ppu->oamaddr = val;
  ppu->ppu_db = val;
}

static void oamdata_write(ppu_s *ppu, uint8_t val) {
  ppu->oamdata = val;
  memory_oam[ppu->oamaddr++] = val;
  ppu->ppu_db = val;
}

static void ppuscroll_write(ppu_s *ppu, uint8_t val) {
  if (ppu->w) {
    ppu->ppuscroll_y = val;
    /* .....FGH -> t = 0 FGH .. ..... ..... */
    ppu->t = (ppu->t & ~MASK_T_V_FINE_Y) | ((val & MASK_PPUSCROLL_FINE) << 12);
    /* ABCDE... -> t = 0 ... .. ABCDE ..... */
    ppu->t = (ppu->t & ~MASK_T_V_COARSE_Y) | ((val & 0xF8) << 2);
    ppu->w = 0;
  } else {
    ppu->ppuscroll_x = val;
    /* .....FGH -> x = FGH */
    ppu->x = val & MASK_PPUSCROLL_FINE;
    /* ABCDE... -> t = 0 ... .. ..... ABCDE */
    ppu->t = (ppu->t & ~MASK_T_V_COARSE_X) | (val >> 3);
    ppu->w = 1;
  }
  ppu->ppu_db = val;
}

static void ppuaddr_write(ppu_s *ppu, uint8_t val) {
  if (!ppu->w) {
    ppu->ppuaddr_high = val & MASK_PPUADDR_HIGH;
    ppu->t = (ppu->t & 0xFF) | ((val & MASK_PPUADDR_HIGH) << 8);
    ppu->w = 1;
  } else {
    ppu->ppuaddr_low = val;
    ppu->t = (ppu->t & 0xFF00) | val;
    ppu->w = 0;
    ppu->v = ppu->t;
  }
  ppu->ppu_db = val;
}

static void ppudata_write(ppu_s *ppu, uint8_t val) {
  if (!(ppu->ppumask & MASK_PPUMASK_BG_R_ENABLE ||
        ppu->ppumask & MASK_PPUMASK_SPRITE_R_ENABLE)) {
    /* not rendering */
    vram_write(ppu->v & MASK_T_V_ADDR_ALL, val);
  }
  ppu->v += (ppu->ppuctrl & MASK_PPUCTRL_INCREMENT) ? 32 : 1;
  ppu->v &= MASK_T_V_SCROLL_ALL;
  ppu->ppudata = val;
  ppu->ppu_db = val;
}

static void oamdma_write(ppu_s *ppu, uint8_t val) { ppu->oamdma = val; }

/*-------------------------rendering helper functions-----------------------*/

static inline void inc_hori_v(ppu_s *ppu) {
  ppu->v = (ppu->v & ~MASK_T_V_COARSE_X) | ((ppu->v + 1) & MASK_T_V_COARSE_X);
  /* if coarse x overflow */
  if (!(ppu->v & MASK_T_V_COARSE_X)) {
    ppu->v ^= 0x400; /* switch horizontal nametable */
  }

  /* load shift registers */
  ppu->at_shift = (ppu->at_byte << 8) | (ppu->at_shift >> 8);
  ppu->ptt_shift_high = (ppu->ptt_high << 8) | (ppu->ptt_shift_high >> 8);
  ppu->ptt_shift_low = (ppu->ptt_low << 8) | (ppu->ptt_shift_low >> 8);
}

static inline void copy_hori_v_t(ppu_s *ppu) {
  ppu->v = (ppu->v & ~MASK_T_V_HORI) | (ppu->t & MASK_T_V_HORI);
}

static inline void inc_vert_v(ppu_s *ppu) {
  /* increment fine y */
  ppu->v = (ppu->v & ~MASK_T_V_FINE_Y) | ((ppu->v + 0x1000) & MASK_T_V_FINE_Y);
  /* if fine y overflow: */
  if (!(ppu->v & MASK_T_V_FINE_Y)) {
    /* increment coarse y */
    ppu->v =
        (ppu->v & ~MASK_T_V_COARSE_Y) | ((ppu->v + 32) & MASK_T_V_COARSE_Y);
    /* if coarse y now == 30 (29 is last row of nametable): */
    if ((ppu->v & MASK_T_V_COARSE_Y) == 0x3C0) {
      ppu->v &= ~MASK_T_V_COARSE_Y;
      /* note that if coarse y = 31 then overflows we don't switch nametable
       */
      ppu->v ^= 0x800; /* switch vertical nametable */
    }
  }
}

static inline void copy_vert_v_t(ppu_s *ppu) {
  ppu->v = (ppu->v & ~MASK_T_V_VERT) | (ppu->t & MASK_T_V_HORI);
}

static inline void tile_data_fetch(ppu_s *ppu, uint8_t offset) {
  switch (offset) {
  case 1:
    nt_byte_fetch(ppu);
    break;
  case 3:
    at_byte_fetch(ppu);
    break;
  case 5:
    ptt_low_byte_fetch(ppu);
    break;
  case 7:
    ptt_high_byte_fetch(ppu);
    inc_hori_v(ppu);
    break;
  }
}

/*------------------------------the meat--------------------------------*/

static void background_step(ppu_s *ppu) {
  // If rendering scanline
  if (ppu->scanline < 240 || ppu->scanline == 261) {

    // If rendering cycle
    if (((ppu->cycles > 0) && (ppu->cycles < 257)) ||
             ((ppu->cycles > 320) && (ppu->cycles < 337))) {
      uint8_t offset = (ppu->cycles - 1) % 8;
      tile_data_fetch(ppu, offset);
      
      if (ppu->cycles == 256) {
          inc_vert_v(ppu);
        }
    }

    else if (ppu->cycles == 257) {
      copy_hori_v_t(ppu);
    }

    if (ppu->scanline == 261 && ((ppu->cycles > 279) && (ppu->cycles < 305))) {
      copy_vert_v_t(ppu);
    }
  }
}


static void sprite_step(ppu_s *ppu) {}

static void render_pixel(const ppu_s *ppu) {
  // Otherwise tiles are wrong way around
  static uint8_t i, tile_x, tile_y, tile_x_quad_select, tile_y_quad_select,
    quad_id, at_color_idx, ptt_color_idx, color_idx, palette_idx;
  
  i = 7 - (ppu->cycles & 7);

  tile_x = ppu->v & MASK_T_V_COARSE_X;
  tile_y = (ppu->v & MASK_T_V_COARSE_Y) >> 5;

  tile_x_quad_select = tile_x & 2;
  tile_y_quad_select = tile_y & 2;
  
  // make a 2 bit index into 4 colour subpallete using tile_x_quad_select
  // as high bit and tile_y_quad_select as low bit
  quad_id = tile_x_quad_select | (tile_y_quad_select >> 1);

  // ???
  at_color_idx = (ppu->at_shift & (0x3 << (quad_id * 2))) >> (quad_id * 2);

  // Select pixel from tile
  ptt_color_idx =
    (((ppu->ptt_shift_high >> i) & 1) << 1) | ((ppu->ptt_shift_low >> i) & 1);

  // AAPP
  // AA : attribute bits
  // PP : pattern bits
  color_idx = (at_color_idx << 2) | ptt_color_idx;

  // Since we are background rendering for now, start at 3F00
  palette_idx = vram_fetch(0x3F00 + color_idx);
  put_pixel(ppu->scanline, ppu->cycles, palette_idx, put_pixel_data);
}

static void increment_ppu(ppu_s *ppu) {
  if (ppu->cycles > 339) {
    ppu->cycles = 0;
    if (ppu->scanline > 260) {
      ppu->scanline = 0;
      ppu->frame_parity = ~ppu->frame_parity;
    } else {
      ppu->scanline++;
    }
  } else {
    ppu->cycles++;
  }
}
