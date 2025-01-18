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

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <fstream>
#include "nestest.hpp"

BOOST_AUTO_TEST_SUITE(core_tests)

BOOST_AUTO_TEST_CASE( nestest_test )
{
  std::ifstream neslog("nestest.log");
  std::vector<std::string> neslog_lines;
  std::string line;
  while(getline(neslog, line)) {
    neslog_lines.push_back(line);
  }

  std::vector<std::string> output_lines = nestest();
  
  BOOST_TEST( output_lines == neslog_lines, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
