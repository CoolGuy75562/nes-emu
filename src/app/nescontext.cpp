#include <QtDebug>
#include "nescontext.h"

NESContext::NESContext(const std::string &rom_filename,
		       void (*put_pixel)(int, int, uint8_t, void *),
		       void *put_pixel_data,
		       uint8_t (*get_pressed_buttons_cb)(void *),
		       void *get_pressed_buttons_data,
		       QObject *parent)
    : QObject(parent) {

  nes_timer = new QTimer(this);
  nes_timer->setInterval(0);
  connect(nes_timer, SIGNAL(timeout()), this, SLOT(nes_tick()));
  
  /* init ppu_ptr */
  qDebug() << "NESContext: Initialising ppu";
  nes_ppu_init_no_alloc(&ppu, put_pixel, put_pixel_data);

  qDebug() << "NESContext: Initialising controller";
  controller_init(get_pressed_buttons_cb, get_pressed_buttons_data);
  
  qDebug() << "NESContext: Initialising memory";
  nes_memory_init(rom_filename, &ppu);

  /* init cpu_ptr */
  qDebug() << "NESContext: Initialising cpu";
  nes_cpu_init_no_alloc(&cpu, 0);
  qDebug() << "NESContext: Init done";
}

/* Slots */

void NESContext::nes_tick(void) {
  try {
    nes_cpu_exec(&cpu);
  } catch (NESError &e) {
    nes_timer->stop();
    emit nes_error(e);
    emit nes_done();
  }
}

void NESContext::nes_step() {
  nes_timer->stop();
  nes_tick();
}

void NESContext::nes_start() {
  nes_timer->start();
}

void NESContext::nes_pause() {
  nes_timer->stop();
  emit nes_paused();
}

