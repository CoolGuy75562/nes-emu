#define BOOST_TEST_MODULE harte_tests

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include "harte.hpp"

#define HARTE_CHECK_CPU_MESSAGE(expected, actual, reg, name)                   \
  BOOST_CHECK_MESSAGE((expected) == (actual),                                  \
                      "Test " << name << ", register "                         \
                              << reg ": Expected: " << (uint16_t)(expected)    \
                              << " Actual: " << (uint16_t)(actual))


BOOST_AUTO_TEST_SUITE(harte_tests)

BOOST_AUTO_TEST_CASE(harte_test) {

  rapidjson::Document document;
  Harte harte(document);
  for (int i = 0; i < 0x100; i++) {
    if (!harte.init_harte_test(i)) {
      BOOST_TEST_MESSAGE("Skipping " << i);
      continue;
    }
    
    while (1) {
      
      cpu_state_s cpu_state_expected;
      std::vector<uint16_t> addrs_expected;
      std::vector<uint8_t> vals_expected;
      std::vector<cycle> cycles_expected, cycles_actual;

      harte_case hc_expected;
      hc_expected.cpu_state = &cpu_state_expected;
      hc_expected.addrs = &addrs_expected;
      hc_expected.vals = &vals_expected;

      harte_case hc_actual;
      std::vector<uint8_t> vals_actual;
      hc_actual.vals = &vals_actual;
      
      if (!harte.do_next_harte_case(hc_expected, hc_actual, cycles_expected,
                                    cycles_actual)) {
        break;
      }
      
      HARTE_CHECK_CPU_MESSAGE(hc_expected.cpu_state->pc,
                              hc_actual.cpu_state->pc, "pc",
                              harte.case_name);
      HARTE_CHECK_CPU_MESSAGE(hc_expected.cpu_state->sp,
                              hc_actual.cpu_state->sp, "s",
                              harte.case_name);
      HARTE_CHECK_CPU_MESSAGE(hc_expected.cpu_state->a,
                              hc_actual.cpu_state->a, "a",
                              harte.case_name);
      HARTE_CHECK_CPU_MESSAGE(hc_expected.cpu_state->x,
                              hc_actual.cpu_state->x, "x",
                              harte.case_name);
      HARTE_CHECK_CPU_MESSAGE(hc_expected.cpu_state->y,
                              hc_actual.cpu_state->y, "y",
                              harte.case_name);
      HARTE_CHECK_CPU_MESSAGE(hc_expected.cpu_state->p,
                              hc_actual.cpu_state->p, "p",
                              harte.case_name);

      BOOST_TEST(*hc_expected.vals == *hc_actual.vals,
		 boost::test_tools::per_element());
      BOOST_TEST(cycles_expected == cycles_actual,
                 boost::test_tools::per_element());
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()
