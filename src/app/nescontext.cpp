#include <QtDebug>
#include "nescontext.h"

NESContext::NESContext(const std::string &rom_filename, void (*put_pixel)(int, int, uint8_t, void *), void *put_pixel_data, QObject *parent)
    : QObject(parent), cpu_ptr(nullptr, &cpu_destroy),
      ppu_ptr(nullptr, &ppu_destroy) {

  /* init ppu_ptr */
  qDebug() << "NESContext: Initialising ppu";
  ppu_s *ppu = nullptr;
  nes_ppu_init(&ppu, put_pixel, put_pixel_data);
  ppu_ptr.reset(ppu);

  qDebug() << "NESContext: Initialising memory";
  nes_memory_init(rom_filename, ppu_ptr.get());

  /* init cpu_ptr */
  qDebug() << "NESContext: Initialising cpu";
  cpu_s *cpu = nullptr;
  nes_cpu_init(&cpu, 0);
  cpu_ptr.reset(cpu);
}

/* Slots */

void NESContext::nes_step(void) {
  try {
    if (nes_cpu_exec(cpu_ptr.get()) == 0) {
      emit nes_done();
    }
  } catch (NESError &e) {
    emit nes_error(e);
  }
}
