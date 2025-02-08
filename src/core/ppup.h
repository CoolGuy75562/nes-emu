#ifndef PPUP_H_
#define PPUP_H_

#include <stdint.h>

typedef struct ppu_s ppu_s;

uint8_t ppu_register_fetch(ppu_s *ppu, uint16_t addr);
void ppu_register_write(ppu_s *ppu, uint16_t addr, uint8_t val, uint8_t *to_oamdma);

/* Give the ppu functions to write and read to vram, which depend on mapper and
 * nametable arrangement */
void ppu_register_vram_write_callback(void (*vram_write)(uint16_t, uint8_t));
void ppu_register_vram_fetch_callback(uint8_t (*vram_fetch)(uint16_t));

/* does one ppu cycle */
void ppu_step(ppu_s *ppu, uint8_t *to_nmi);

#endif
