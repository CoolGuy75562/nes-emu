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

#ifndef MEMORY_H_
#define MEMORY_H_

#include "core/ppu.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum memory_cb_e { MEMORY_CB_WRITE, MEMORY_CB_FETCH } memory_cb_e;

/* register callback for memory_fetch or memory_write */
void memory_register_cb(void (*memory_cb)(uint16_t, uint8_t, void *),
			void *data,
                        memory_cb_e cb_type);
void memory_unregister_cb(memory_cb_e cb_type);

/* initialises cpu memory and mapper according to
 * contents of .nes file rom_filename. ppu is used for calling ppu functions.
 *
 * If rom_filename and ppu are both null, cpu memory is zeroed out
 * and calls to memory_fetch and memory_write will not use a ppu
 *
 * Returns <0 if .nes file is invalid, error reading contents, etc.
 */
int memory_init(const char *rom_filename, ppu_s *p, char *e_context);

/* dump ines header information to file */
/* void ines_header_dump(void); */

/* hexdump cpu memory contents to file */
int memory_dump_file(FILE *fp);

/* hexdump memory contents to string
 *
 * return <0 if error, 0 otherwise 
 */
int memory_dump_string(char *dump, size_t dump_len);

/* hexdump vram contents to string
 *
 * return <0 if error, 0 otherwise
 */
int memory_vram_dump_string(char *dump, size_t dump_len);

/* Initialises memory to addrs and vals */
void memory_init_harte_test_case(const uint16_t *addrs, const uint8_t *vals, size_t length);

/* sets list of addresses to 0 and sets vals to the values at the addresses*/
void memory_reset_harte(const uint16_t *addrs, uint8_t *final_vals, size_t length);

#endif
