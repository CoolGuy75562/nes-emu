/* Copyright (C) 2024  Angus McLean
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

#include "core/cppwrapper.hpp"

#define X(error, message) #error

std::string error_names[] = {ERROR_LIST};

#undef X

#define X(error, message) message

std::string error_messages[] = {ERROR_LIST};

#undef X

NESError::NESError(int err, const std::string &info)
    : std::runtime_error(errorMessage(err, info)) {}

NESError::NESError(int err)
: std::runtime_error(errorMessage(err)) {}

std::string NESError::errorMessage(int err, const std::string &info) {
    std::string e_mesg =
        error_names[err].append(" ").append(error_messages[err]);
    if (!info.empty()) {
      e_mesg.append(" ").append(info);
    }
    return e_mesg;
}


void nes_memory_init(const std::string &rom_filename, ppu_s *ppu) {
  int err;
  char e_context[LEN_E_CONTEXT];
  *e_context = '\0';
  if ((err = memory_init(rom_filename.c_str(), ppu, e_context)) < 0) {
    throw NESError(-err, std::string(e_context));
  }
}
		    
void nes_ppu_init(ppu_s **ppu, void (*put_pixel)(int, int, uint8_t)) {
  int err;
  if ((err = ppu_init(ppu, put_pixel)) < 0) {
    throw NESError(-err);
  }
}

void nes_cpu_init(cpu_s **cpu, int nestest) {
  int err;
  if ((err = cpu_init(cpu, nestest)) < 0) {
    throw NESError(-err);
  }
}

void nes_cpu_exec(cpu_s *cpu) {
  char e_context[LEN_E_CONTEXT];
  *e_context = '\0';
  int exec_status = 0;
  while ((exec_status = cpu_exec(cpu, e_context)) == 1)
    ;
  if (exec_status < 0) {
    throw NESError(-exec_status, std::string(e_context));
  }
}
