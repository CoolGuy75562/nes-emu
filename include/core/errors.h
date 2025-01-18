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

#ifndef ERRORS_H_
#define ERRORS_H_

#define LEN_E_CONTEXT 256

#define ERROR_LIST                                                             \
  X(E_NO_ERROR, ""), X(E_NO_CALLBACK, "Not all callbacks registered"),         \
      X(E_NO_FILE, "File path is null"), X(E_ILLEGAL_OPC, "Illegal opcode"),   \
      X(E_READ_FILE, "Error reading file"),                                    \
      X(E_MALLOC, "Error allocating memory"),                                  \
      X(E_INES_SIGNATURE, "Invalid iNES signature"),                           \
      X(E_MAPPER_IMPLEMENTED, "Mapper number not implemented: "),              \
      X(E_CHR_ROM_SIZE, "CHR ROM size incompatible with mapper number: "),     \
      X(E_PRG_ROM_SIZE, "PRG ROM size incompatible with mapper number: "),     \
      X(E_OPEN_FILE, "Unable to open file")

#define X(error, message) error

typedef enum { ERROR_LIST } error_e;

#undef X

#endif
