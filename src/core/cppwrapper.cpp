#include "core/cppwrapper.hpp"

#define X(error, message) #error

std::string error_names[] = {ERROR_LIST};

#undef X

#define X(error, message) message

std::string error_messages[] = {ERROR_LIST};

#undef X

NESError::NESError(error_e err, const std::string &info)
  : std::runtime_error(errorMessage(err, info)) {}

NESError::NESError(error_e err)
: std::runtime_error(errorMessage(err)) {}

std::string NESError::errorMessage(error_e err, const std::string &info) {
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
    throw NESError((error_e)-err, e_context);
  }
}
		    
void nes_ppu_init(ppu_s **ppu, void (*put_pixel)(int, int, uint8_t)) {
  int err;
  if ((err = ppu_init(ppu, put_pixel)) < 0) {
    throw NESError((error_e)-err);
  }
}

void nes_cpu_init(cpu_s **cpu, int nestest) {
  int err;
  if ((err = cpu_init(cpu, nestest)) < 0) {
    throw NESError((error_e)-err);
  }
}

void nes_cpu_exec(cpu_s *cpu) {
  char e_context[LEN_E_CONTEXT];
  *e_context = '\0';
  int exec_status = 0;
  while ((exec_status = cpu_exec(cpu, e_context)) == 1)
    ;
  if (exec_status < 0) {
    throw NESError((error_e)-exec_status, e_context);
  }
}
