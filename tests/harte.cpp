#include <cstring>
#include <iostream>
#include <memory>

#include "harte.hpp"

static void write_cycle(uint16_t addr, uint8_t val, void *data);
static void fetch_cycle(uint16_t addr, uint8_t val, void *data);
static void update_cpu_state(cpu_state_s *state, void* data);
static void error_none(const char *, ...) {}
static void cpu_cb_none(cpu_state_s *, void *) {}
static void memory_cb_none(uint16_t addr, uint8_t val, void* data) {}

static char e_context[LEN_E_CONTEXT];

/* This seems a bit silly */
cycle::cycle(uint16_t addr, uint8_t val, char type)
    : addr(addr), val(val), type(type) {}

/* So that we can compare cycle structs for boost test */
bool operator==(const cycle &cycle1, const cycle &cycle2) {
  return (cycle1.addr == cycle2.addr && cycle1.val == cycle2.val &&
          cycle1.type == cycle2.type);
}

std::ostream &operator<<(std::ostream &os, const cycle &cyc) {
  os << (uint16_t)cyc.addr << ", " << (uint16_t)cyc.val << ", "
     << ((cyc.type == 'r') ? "read" : "write");
  return os;
}


Harte::Harte(rapidjson::Document &document) : cpu_ptr(nullptr, &cpu_destroy), document(document) {

  memory_register_cb(&memory_cb_none, NULL, MEMORY_CB_WRITE);
  memory_register_cb(&memory_cb_none, NULL, MEMORY_CB_FETCH);
  cpu_register_state_callback(&cpu_cb_none, NULL);
  cpu_register_error_callback(&error_none);

  memory_init(NULL, NULL, e_context);

  cpu_s *cpu = nullptr;
  cpu_init(&cpu, 1);
  cpu_ptr.reset(cpu);
}

Harte::~Harte() {}

bool Harte::init_harte_test(int test_id) {
  test_no = 0;
  
  /* This prevents the document's MemoryPoolAllocator from
   * eating up all the memory
   */
  rapidjson::Document d;
  document.Swap(d);

  return is_valid_opcode(test_id) && parse_harte_file(test_id);
}

/* Check that test_id is a valid opcode.
 * Bit of a hacky way to do it but alternative is
 * to have a list of valid opcodes somewhere.
 */
bool Harte::is_valid_opcode(int test_id) {
  cpu_register_state_callback(&cpu_cb_none, NULL);
  memory_register_cb(&memory_cb_none, NULL, MEMORY_CB_FETCH);
  memory_register_cb(&memory_cb_none, NULL, MEMORY_CB_WRITE);
  cpu_state_s state = {.pc = 0,
                       .cycles = 0,
                       .a = 0,
                       .x = 0,
                       .y = 0,
                       .sp = 0,
                       .p = 0,
                       .opc = 0,
                       .curr_instruction = "",
                       .curr_addr_mode = ""};
  uint16_t addr[] = {0};
  uint8_t opc[] = {(uint8_t)test_id};
  memory_init_harte_test_case(addr, opc, 1);
  cpu_init_harte_test_case(cpu_ptr.get(), &state);
  
  /* now pc is pointing to test_id and will be read as next opcode */
  return (cpu_exec(cpu_ptr.get(), e_context) == 1);
}

/* TODO: Check valid json etc, though rapidjson should do that
 * to some degree
 */
bool Harte::parse_harte_file(int test_id) {
  /* Now time to open harte_tests_dir/${test_id}.json */
  char test_filename[64];
  sprintf(test_filename, "harte_tests_dir/%02x.json", test_id);
  FILE *fp;
  if ((fp = fopen(test_filename, "r")) == NULL) {
    return false; /* file doesn't exist etc. */
  }

  static char buf[65536] = {0};
  memset(buf, 0, sizeof(buf));

  rapidjson::FileReadStream harte_stream(fp, buf, sizeof(buf));
  document.ParseStream(harte_stream);
  fclose(fp);

  return true;
}

bool Harte::do_next_harte_case(harte_case &hc_expected, harte_case &hc_actual,
                               std::vector<cycle> &cycles_expected,
                               std::vector<cycle> &cycles_actual) {
  if (test_no > document.Size() - 1) {
    return false;
  }

  assert(document[test_no].IsObject());
  const rapidjson::Value &curr_case = document[test_no++].GetObject();

  assert(curr_case["name"].IsString());
  case_name = curr_case["name"].GetString();

  /* init initial */
  harte_case hc_initial;
  cpu_state_s cpu_state_initial;
  hc_initial.cpu_state = &cpu_state_initial;
  std::vector<uint16_t> addrs_initial;
  hc_initial.addrs = &addrs_initial;
  std::vector<uint8_t> vals_initial;
  hc_initial.vals = &vals_initial;
  
  init_harte_struct(curr_case, hc_initial, "initial");
  
  /* init expected */
  init_harte_struct(curr_case, hc_expected, "final");

  /* init actual */
  hc_actual.addrs = hc_expected.addrs;
  hc_actual.vals->resize(hc_actual.addrs->size());

  /* init cycles */
  init_cycles_expected(curr_case, cycles_expected);
  
  memory_register_cb(&fetch_cycle, &cycles_actual, MEMORY_CB_FETCH);
  memory_register_cb(&write_cycle, &cycles_actual, MEMORY_CB_WRITE);
  cpu_register_state_callback(&update_cpu_state, &hc_actual);
  
  /* now ready to do the case */
  memory_init_harte_test_case(hc_initial.addrs->data(), hc_initial.vals->data(),
                              hc_initial.vals->size());
  cpu_init_harte_test_case(cpu_ptr.get(), hc_initial.cpu_state);

  cpu_exec(cpu_ptr.get(), e_context);

  memory_reset_harte(hc_actual.addrs->data(), hc_actual.vals->data(),
                     hc_actual.vals->size());
  cpu_unregister_state_callback();
  memory_unregister_cb(MEMORY_CB_FETCH);
  memory_unregister_cb(MEMORY_CB_WRITE);
  return true;
}

void Harte::init_cycles_expected(const rapidjson::Value &curr_case,
                                 std::vector<cycle> &cycles_expected) {

  assert(curr_case["cycles"].IsArray());
  const auto &the_cycles = curr_case["cycles"];

  for (rapidjson::SizeType i = 0; i < the_cycles.Size(); i++) {
    std::string type = the_cycles[i][2].GetString();
    cycles_expected.push_back(cycle(the_cycles[i][0].GetUint(),
                                    the_cycles[i][1].GetUint(),
                                    (type.compare("read") ? 'w' : 'r')));
  }
}

void Harte::init_harte_struct(const rapidjson::Value &curr_case,
                              harte_case &harte_struct,
                              const char *initial_or_final) {

  const auto &object = curr_case[initial_or_final];
  assert(object.IsObject());

  harte_struct.cpu_state->pc = object["pc"].GetUint();
  harte_struct.cpu_state->cycles = 0;
  harte_struct.cpu_state->a = object["a"].GetUint();
  harte_struct.cpu_state->x = object["x"].GetUint();
  harte_struct.cpu_state->y = object["y"].GetUint();
  harte_struct.cpu_state->sp = object["s"].GetUint();
  harte_struct.cpu_state->p = object["p"].GetUint();

  const auto &ram = object["ram"];
  assert(ram.IsArray());
  for (rapidjson::SizeType i = 0; i < ram.Size(); i++) {
    harte_struct.addrs->push_back(static_cast<uint16_t>(ram[i][0].GetUint()));
    harte_struct.vals->push_back(static_cast<uint8_t>(ram[i][1].GetUint()));
  }
}

static void write_cycle(uint16_t addr, uint8_t val, void *data) {
  static std::vector<cycle> *cycles;
  cycles = static_cast<std::vector<cycle> *>(data);
  cycles->push_back(cycle(addr, val, 'w'));
}

static void fetch_cycle(uint16_t addr, uint8_t val, void *data) {
  static std::vector<cycle> *cycles;
  cycles = static_cast<std::vector<cycle> *>(data);
  cycles->push_back(cycle(addr, val, 'r'));
}

static void update_cpu_state(cpu_state_s *state, void *data) {
  harte_case *hc_actual = static_cast<harte_case *>(data);
  hc_actual->cpu_state = state;
}
