#ifndef HARTE_H_
#define HARTE_H_

#include "core/cppwrapper.hpp"
#include <memory>
#include <vector>
#include <array>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

struct harte_case {
  cpu_state_s *cpu_state;
  std::vector<uint16_t> *addrs;
  std::vector<uint8_t> *vals;
};

struct cycle {  
  cycle(uint16_t addr, uint8_t val, char type);
  uint16_t addr;
  uint8_t val;
  char type;
};

bool operator==(const cycle &cycle1, const cycle &cycle2);
std::ostream& operator<<(std::ostream &os, const cycle &cyc);

class Harte {

public:
  Harte(rapidjson::Document &document);
  ~Harte();

  bool init_harte_test(int test_id);
  bool do_next_harte_case(harte_case &hc_expected, harte_case &hc_actual,
                          std::vector<cycle> &cycles_expected, std::vector<cycle> &cycles_actual);

  const char *case_name;
  const char *filename;

private:
  bool is_valid_opcode(int test_id);
  bool parse_harte_file(int test_id);
  void init_harte_struct(const rapidjson::Value &curr_case, harte_case &harte_struct,
                         const char *initial_or_final);
  void init_cycles_expected(const rapidjson::Value &curr_case,
                            std::vector<cycle> &cycles_expected);

  std::unique_ptr<cpu_s, void (*)(cpu_s *)> cpu_ptr;
  rapidjson::Document &document;
  rapidjson::SizeType test_no;
  
};
  
void harte_init(cpu_s **cpu_ptr);

int do_harte_case(cpu_s *cpu, const harte_case &hc_initial,
                  harte_case *hc_final, std::vector<cycle> *cycles);

#endif
