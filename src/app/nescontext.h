#ifndef NESCONTEXT_H_
#define NESCONTEXT_H_

#include <QObject>
#include <QTimer>

#include <memory>
#include <string>

#include "core/cppwrapper.hpp"

class NESContext : public QObject {
  Q_OBJECT

public:
  
  NESContext(const std::string &rom_filename,
	     void (*put_pixel)(int, int, uint8_t, void *),
	     void *put_pixel_data,
	     uint8_t (*get_pressed_buttons)(void *),
	     void *get_pressed_buttons_data,
	     QObject *parent = nullptr);

public slots:
  void nes_step(void);
  void nes_start(void);
  void nes_pause(void);
  
signals:
  void nes_error(NESError e);
  void nes_done(void);
  void nes_paused(void);

private:
  QTimer *nes_timer;
  cpu_s cpu;
  ppu_s ppu;

private slots:
  void nes_tick(void);
};

#endif
