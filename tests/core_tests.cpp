/* tests for the core emulator functionality */
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

#define BOOST_TEST_MODULE core_tests

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include "core/cppwrapper.hpp"
#include "nestest.hpp"
#include <fstream>

static void put_pixel(int x, int y, uint8_t z) {}
static void cb_error_none(const char *format, ...) {}
static void cb_ppu_none(ppu_state_s *ppu_state) {}
static void cb_memory_none(uint16_t addr, uint8_t val) {}

BOOST_AUTO_TEST_SUITE(core_tests)

BOOST_AUTO_TEST_CASE(ppu_test) {

  ppu_s *ppu = nullptr;
  /* required callbacks not registered yet */
  BOOST_CHECK(ppu_init(&ppu, &put_pixel) == -E_NO_CALLBACK);

  /* ----------------------------------------------------------------------- */
  ppu_register_error_callback(&cb_error_none);
  ppu_register_state_callback(&cb_ppu_none);
  /* callbacks now registered so should be fine */
  BOOST_CHECK(ppu_init(&ppu, &put_pixel) == E_NO_ERROR);
  
  ppu_destroy(ppu);
}

BOOST_AUTO_TEST_CASE(memory_test) {

  /* TODO: check e_context contents and and check for buffer overflow
     and check that mappers and memory map etc. behave properly */
  
  char e_context[LEN_E_CONTEXT];
  *e_context = '\0';
  ppu_s *ppu = nullptr;
  nes_ppu_init(&ppu, &put_pixel);
  BOOST_CHECK(memory_init("nestest.nes", ppu, e_context) == -E_NO_CALLBACK);

  memory_register_cb(&cb_memory_none, MEMORY_CB_FETCH);
  memory_register_cb(&cb_memory_none, MEMORY_CB_WRITE);

  /* null ppu */
  BOOST_CHECK(memory_init("nestest.nes", NULL, e_context) == -E_NO_PPU);

  /* null file name */
  BOOST_CHECK(memory_init(NULL, ppu, e_context) == -E_NO_FILE);

  /* file that doesn't exist */
  BOOST_CHECK(memory_init("does_not_exist", ppu, e_context) == -E_OPEN_FILE);

  /* .nes file with mapper not yet implemented */
  BOOST_CHECK(memory_init("mapper_6.nes", ppu, e_context) ==
              -E_MAPPER_IMPLEMENTED);

  /* not a .nes file */
  BOOST_CHECK(memory_init("nestest.log", ppu, e_context) == -E_INES_SIGNATURE);

  /* this should work now */
  BOOST_CHECK(memory_init("nestest.nes", ppu, e_context) == E_NO_ERROR);

  ppu_destroy(ppu);
}

/* Runs nestest.nes and compares output with correct log nestest.log */
BOOST_AUTO_TEST_CASE(nestest_test) {
  std::ifstream neslog("nestest.log");
  std::vector<std::string> neslog_lines;
  std::string line;
  while (getline(neslog, line)) {
    neslog_lines.push_back(line);
  }

  std::vector<std::string> output_lines = nestest();

  BOOST_TEST(output_lines == neslog_lines, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
