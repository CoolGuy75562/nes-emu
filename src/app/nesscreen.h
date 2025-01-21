#ifndef NESSCREEN_H_
#define NESSCREEN_H_

#include <QObject>
#include <array>

const auto nes_screen_width = 256;
const auto nes_screen_height =  240;
const auto nes_screen_size =  nes_screen_width * nes_screen_height * 3;

#define PALETTE_SIZE 64

class NESScreen : public QObject {

  Q_OBJECT

public:
  NESScreen(QObject *parent = nullptr);
  std::array<uint8_t, nes_screen_size> *getPBufPtr();
  static void put_pixel(int i, int j, uint8_t palette_idx);

signals:
  void pbuf_full();
  
private:
  static const uint8_t palette[3 * PALETTE_SIZE];
  std::array<uint8_t, nes_screen_size> pbuf;
};

#endif
