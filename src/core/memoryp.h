#ifndef MEMORYP_H_
#define MEMORYP_H_

#include  <stdint.h>
/* return value from addr of cpu memory, or result of reading
 * ppu memory-mapped register if addr corresponds to one.
 *
 * The cpu either fetches or writes every cycle, and the ppu
 * steps three ppu cycles for each cpu cycle, so this function
 * steps the ppu three cycles
 *
 * if to_nmi is 0, and nmi is triggered in the ppu steps, to_nmi
 * is set to 1
 *
 * fetch callback is called with addr and return value before return
 *
 */
uint8_t memory_fetch(uint16_t addr, uint8_t *to_nmi);

/* write value val to cpu memory at address addr. if addr corresponds to
 * a ppu memory-mapped register, ppu does stuff. else val is written to
 * cpu memory, accounting for mirroring etc defined by the mapper.
 *
 * *oamdma is set to 1 if the write is to address of ppu register OAMDMA
 *
 * The cpu either fetches or writes every cycle, and the ppu
 * steps three ppu cycles for each cpu cycle, so this function
 * steps the ppu three cycles
 *
 * write callback is called with addr and val before return
 */
void memory_write(uint16_t addr, uint8_t val, uint8_t *oamdma, uint8_t *to_nmi);

/* does nothing right now but will do oamdma in the future */
void memory_do_oamdma(uint8_t val, uint16_t *cycles, uint8_t *to_nmi);

#endif
