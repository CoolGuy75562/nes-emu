#ifndef NESCONTEXT_H_
#define NESCONTEXT_H_

#include <QObject>

#include <memory>
#include <string>

#include "core/cppwrapper.hpp"

class NESContext : public QObject {
  Q_OBJECT

public:
  
  NESContext(const std::string &rom_filename, void (*put_pixel)(int, int, uint8_t), QObject *parent = nullptr);
  
public slots:
  void nes_step(void);
  
signals:
  void nes_error(NESError &e);
  void nes_done(void);

private:
  std::unique_ptr<cpu_s, void (*)(cpu_s *)> cpu_ptr;
  std::unique_ptr<ppu_s, void (*)(ppu_s *)> ppu_ptr;
};

#endif
