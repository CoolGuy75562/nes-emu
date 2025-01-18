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

#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>

/* external information about cpu state to log etc */
typedef struct cpu_state_s {
  uint16_t pc;
  uint16_t cycles;
  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint8_t sp;
  uint8_t p;
  uint8_t opc;
  const char *curr_instruction;
  const char *curr_addr_mode;
} cpu_state_s;

/* cpu context for internal use */
typedef struct cpu_s cpu_s;

/* called with current cpu state after each instruction (for now) */
void cpu_register_state_callback(void (*cpu_state_cb)(cpu_state_s *));
/* called when error e.g. illegal opcode */
void cpu_register_error_callback(void (*log_error_cb)(const char *, ...));

/* allocate memory to opaque pointer cpu_s * and initialise cpu
*
* Return value < 0 if error
*/
int cpu_init(cpu_s **, uint8_t nestest);

/* free memory allocated from cpu_init */
void cpu_destroy(cpu_s *);

/* execute one instruction
 *
 * Return value < 0 if error
 */
int cpu_exec(cpu_s *, char *e_context);

#endif
