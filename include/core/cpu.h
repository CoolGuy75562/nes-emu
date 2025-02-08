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

/* cpu context, for internal use only. definition is included below
 * so it can be stack allocated then initialised using cpu_init_no_alloc,
 * but nothing outside of cpu.c should touch it. Just treat it as an
 * opaque pointer. */
typedef struct cpu_s cpu_s;

/* called with current cpu state after each instruction (for now) */
void cpu_register_state_callback(void (*cpu_state_cb)(const cpu_state_s *, void *),
                                 void *cpu_cb_data);

void cpu_unregister_state_callback(void);

/* called when error e.g. illegal opcode */
void cpu_register_error_callback(void (*log_error_cb)(const char *, ...));
void cpu_unregister_error_callback(void);

/* initialise cpu
 *
 * Return value < 0 if error
 */
int cpu_init_no_alloc(cpu_s *cpu, uint8_t nestest);

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
int cpu_exec(cpu_s *);

/* Resets cpu and sets cpu values to values in cpu_state
 *
 * Used for each harte test case
 */
void cpu_init_harte_test_case(cpu_s *cpu, cpu_state_s *cpu_state);


/*============================================================*/
/*                      Do not touch:                         */ /*============================================================*/
typedef struct cpu_s {
  uint8_t a; // Accumulator register
  uint8_t x; // X, Y registers, used for indexing
  uint8_t y;
  uint16_t pc; // Programme counter
  uint8_t sp;  // Stack pointer
  uint8_t flags;
  uint16_t to_update_flags; /* 0: No, 1: Yes, 2: After next instruction */
  uint8_t new_int_disable_flag;
  uint8_t to_oamdma;
  uint16_t cycles;
  uint8_t to_nmi;
  uint8_t in_nmi;
  uint8_t to_irq;
} cpu_s;

#endif
