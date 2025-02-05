/* tests for the core emulator functionality */
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

#define BOOST_TEST_MODULE core_tests

#include <fstream>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include "core/cppwrapper.hpp"

#include "nestest.hpp"

static void put_pixel(int, int, uint8_t, void *) {}
static void cb_error_none(const char *format, ...) {}
static void cb_ppu_none(const ppu_state_s *ppu_state, void *data) {}
static void cb_cpu_none(const cpu_state_s *cpu_state, void *data) {}
static void cb_memory_none(uint16_t addr, uint8_t val, void *data) {}

BOOST_AUTO_TEST_SUITE(core_tests)

BOOST_AUTO_TEST_CASE(ppu_test) {

  ppu_s *ppu = nullptr;
  /* No callbacks registered */
  BOOST_CHECK(ppu_init(&ppu, &put_pixel, NULL) == -E_NO_CALLBACK);
  
  /* only error callback registered */
  ppu_register_error_callback(&cb_error_none);
  BOOST_CHECK(ppu_init(&ppu, &put_pixel, NULL) == -E_NO_CALLBACK);
  ppu_unregister_error_callback();

  /* only state callback registered */
  ppu_register_state_callback(&cb_ppu_none, NULL);
  BOOST_CHECK(ppu_init(&ppu, &put_pixel, NULL) == -E_NO_CALLBACK);
  ppu_unregister_state_callback();

  /* both callbacks now registered */
  ppu_register_state_callback(&cb_ppu_none, NULL);
  ppu_register_error_callback(&cb_error_none);
  BOOST_CHECK(ppu_init(&ppu, &put_pixel, NULL) == E_NO_ERROR);

  ppu_destroy(ppu);
  ppu_unregister_state_callback();
  ppu_unregister_error_callback();
}

BOOST_AUTO_TEST_CASE(memory_test) {

  /* TODO: check e_context contents and and check for buffer overflow
     and check that mappers and memory map etc. behave properly */

  char e_context[LEN_E_CONTEXT];
  *e_context = '\0';
  ppu_s *ppu = nullptr;

  ppu_register_state_callback(&cb_ppu_none, NULL);
  ppu_register_error_callback(&cb_error_none);
  ppu_init(&ppu, &put_pixel, NULL);

  /* -------------------------------------------------- */

  /* neither callback registered */
  BOOST_CHECK(memory_init("nestest.nes", ppu, e_context) == -E_NO_CALLBACK);

  /* only fetch callback registered */
  memory_register_cb(&cb_memory_none, NULL, MEMORY_CB_FETCH);
  BOOST_CHECK(memory_init("nestest.nes", ppu, e_context) == -E_NO_CALLBACK);
  memory_unregister_cb(MEMORY_CB_FETCH);

  /* only write callback registered */
  memory_register_cb(&cb_memory_none, NULL, MEMORY_CB_WRITE);
  BOOST_CHECK(memory_init("nestest.nes", ppu, e_context) == -E_NO_CALLBACK);
  memory_unregister_cb(MEMORY_CB_WRITE);

  memory_register_cb(&cb_memory_none, NULL, MEMORY_CB_WRITE);
  memory_register_cb(&cb_memory_none, NULL, MEMORY_CB_FETCH);

  /* null ppu */
  BOOST_CHECK(memory_init("nestest.nes", NULL, e_context) == -E_NO_PPU);

  /* null file name */
  BOOST_CHECK(memory_init(NULL, ppu, e_context) == -E_NO_FILE);

  /* file that doesn't exist */
  BOOST_CHECK(memory_init("does_not_exist", ppu, e_context) == -E_OPEN_FILE);

  /* .nes file with mapper not yet implemented */
  BOOST_CHECK(memory_init("mapper_3.nes", ppu, e_context) ==
              -E_MAPPER_IMPLEMENTED);

  /* not a .nes file */
  BOOST_CHECK(memory_init("nestest.log", ppu, e_context) == -E_INES_SIGNATURE);

  /* this should work now */
  BOOST_CHECK(memory_init("nestest.nes", ppu, e_context) == E_NO_ERROR);

  ppu_destroy(ppu);
  memory_unregister_cb(MEMORY_CB_WRITE);
  memory_unregister_cb(MEMORY_CB_FETCH);
  ppu_unregister_error_callback();
  ppu_unregister_state_callback();
}

BOOST_AUTO_TEST_CASE(cpu_test) {

  char e_context[LEN_E_CONTEXT];
  *e_context = '\0';

  ppu_register_state_callback(&cb_ppu_none, NULL);
  ppu_register_error_callback(&cb_error_none);
  memory_register_cb(&cb_memory_none, NULL, MEMORY_CB_WRITE);
  memory_register_cb(&cb_memory_none, NULL, MEMORY_CB_FETCH);
  ppu_s *ppu = nullptr;
  ppu_init(&ppu, &put_pixel, NULL);
  memory_init("nestest.nes", ppu, e_context);

  /* -------------------------------------------------- */
  cpu_s *cpu = nullptr;

  /* == cpu_init() == */

  /* neither callback registered */
  BOOST_CHECK(cpu_init(&cpu, 0) == -E_NO_CALLBACK);
  BOOST_CHECK(cpu_init(&cpu, 1) == -E_NO_CALLBACK);

  /* only cpu state callback registered */
  cpu_register_state_callback(&cb_cpu_none, NULL);

  BOOST_CHECK(cpu_init(&cpu, 0) == -E_NO_CALLBACK);
  BOOST_CHECK(cpu_init(&cpu, 1) == -E_NO_CALLBACK);

  cpu_unregister_state_callback();

  /* only error callback registered */
  cpu_register_error_callback(&cb_error_none);

  BOOST_CHECK(cpu_init(&cpu, 0) == -E_NO_CALLBACK);
  BOOST_CHECK(cpu_init(&cpu, 1) == -E_NO_CALLBACK);

  cpu_unregister_error_callback();

  /* both callbacks registered */
  cpu_register_state_callback(&cb_cpu_none, NULL);
  cpu_register_error_callback(&cb_error_none);

  BOOST_CHECK(cpu_init(&cpu, 0) == E_NO_ERROR);
  cpu_destroy(cpu);

  cpu = nullptr;
  BOOST_CHECK(cpu_init(&cpu, 1) == E_NO_ERROR);

  /* == cpu_exec() == */

  cpu_unregister_error_callback();
  cpu_unregister_state_callback();

  BOOST_CHECK(cpu_exec(cpu, e_context) == -E_NO_CALLBACK);

  cpu_register_error_callback(&cb_error_none);
  BOOST_CHECK(cpu_exec(cpu, e_context) == -E_NO_CALLBACK);
  cpu_unregister_error_callback();

  cpu_register_state_callback(&cb_cpu_none, NULL);
  BOOST_CHECK(cpu_exec(cpu, e_context) == -E_NO_CALLBACK);
  cpu_unregister_state_callback();

  ppu_unregister_state_callback();
  ppu_unregister_error_callback();
  memory_unregister_cb(MEMORY_CB_WRITE);
  memory_unregister_cb(MEMORY_CB_FETCH);
  ppu_destroy(ppu);
  cpu_destroy(cpu);
}


BOOST_AUTO_TEST_CASE(nestest_test) {
  BOOST_TEST(nestest_actual() == nestest_log(), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
