/* wrappers to replace error codes and messages with exceptions */
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

#ifndef CPPWRAPPER_H_
#define CPPWRAPPER_H_

#include <stdexcept>

extern "C" {
#include "errors.h"
#include "memory.h"
#include "ppu.h"
#include "cpu.h"
#include "controller.h"
}

extern std::string error_names[];
extern std::string error_messages[];

class NESError : public std::runtime_error {
public:
  NESError(int err, const std::string &info);
  NESError(int err);
  NESError(const NESError &other);
  NESError();

private:
  std::string errorMessage(int err, const std::string &info = "");
};

void nes_memory_init(const std::string &rom_filename, ppu_s *ppu);
void nes_ppu_init(ppu_s **ppu, void (*put_pixel)(int, int, uint8_t, void *),
                  void *);
void nes_ppu_init_no_alloc(ppu_s *ppu,
                           void (*put_pixel)(int, int, uint8_t, void *),
			   void *);
void nes_cpu_init(cpu_s **cpu, int nestest);
void nes_cpu_init_no_alloc(cpu_s *cpu, int nestest);
void nes_cpu_exec(cpu_s *cpu);

#endif
