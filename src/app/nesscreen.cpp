#include "nesscreen.h"
#include <QtDebug>

void put_pixel(int i, int j, uint8_t palette_idx, void *screen) {
  static NESScreen *s = static_cast<NESScreen *>(screen);
  int index = (nes_screen_width * i + j) * 3;
  palette_idx %= 64;
  palette_idx *= 3;
  
  s->pbuf.at(index) = NESScreen::palette[palette_idx];
  s->pbuf.at(index + 1) = NESScreen::palette[palette_idx + 1];
  s->pbuf.at(index + 2) = NESScreen::palette[palette_idx + 2];
  if (index + 2 == nes_screen_size - 1) {
    s->emit pbuf_full();
  }
}

const uint8_t NESScreen::palette[3 * PALETTE_SIZE] = {
    0x62, 0x62, 0x62, 0x01, 0x20, 0x90, 0x24, 0x0b, 0xa0, 0x47, 0x00, 0x90,
    0x60, 0x00, 0x62, 0x6a, 0x00, 0x24, 0x60, 0x11, 0x00, 0x47, 0x27, 0x00,
    0x24, 0x3c, 0x00, 0x01, 0x4a, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x47, 0x24,
    0x00, 0x36, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xab, 0xab, 0xab, 0x1f, 0x56, 0xe1, 0x4d, 0x39, 0xff, 0x7e, 0x23, 0xef,
    0xa3, 0x1b, 0xb7, 0xb4, 0x22, 0x64, 0xac, 0x37, 0x0e, 0x8c, 0x55, 0x00,
    0x5e, 0x72, 0x00, 0x2d, 0x88, 0x00, 0x07, 0x90, 0x00, 0x00, 0x89, 0x47,
    0x00, 0x73, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x67, 0xac, 0xff, 0x95, 0x8d, 0xff, 0xc8, 0x75, 0xff,
    0xf2, 0x6a, 0xff, 0xff, 0x6f, 0xc5, 0xff, 0x83, 0x6a, 0xe6, 0xa0, 0x1f,
    0xb8, 0xbf, 0x00, 0x85, 0xd8, 0x01, 0x5b, 0xe3, 0x35, 0x45, 0xde, 0x88,
    0x49, 0xca, 0xe3, 0x4e, 0x4e, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xbf, 0xe0, 0xff, 0xd1, 0xd3, 0xff, 0xe6, 0xc9, 0xff,
    0xf7, 0xc3, 0xff, 0xff, 0xc4, 0xee, 0xff, 0xcb, 0xc9, 0xf7, 0xd7, 0xa9,
    0xe6, 0xe3, 0x97, 0xd1, 0xee, 0x97, 0xbf, 0xf3, 0xa9, 0xb5, 0xf2, 0xc9,
    0xb5, 0xeb, 0xee, 0xb8, 0xb8, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

NESScreen::NESScreen(QObject *parent) : QObject(parent) {
  pbuf.fill(0);
}

void (*NESScreen::get_put_pixel(void))(int, int, uint8_t, void *) {
  return &put_pixel;
}

std::array<uint8_t, nes_screen_size> *NESScreen::getPBufPtr() {
  return &pbuf;
}
