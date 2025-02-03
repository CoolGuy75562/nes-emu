#ifndef NESSCREEN_H_
#define NESSCREEN_H_

#include <QWidget>
#include <array>

const auto nes_screen_width = 256;
const auto nes_screen_height =  240;
const auto nes_screen_size =  nes_screen_width * nes_screen_height * 3;

const auto pattern_table_width = 128;
const auto pattern_table_height = 128;
const auto pattern_table_size = pattern_table_width * pattern_table_height * 3;

extern "C" void put_pixel();
extern "C" void pt_put_pixel();

class NESScreen : public QObject {

  Q_OBJECT

public:
  NESScreen(QObject *parent = nullptr);
  std::array<uint8_t, nes_screen_size> *getPBufPtr();
  friend void put_pixel(int i, int j, uint8_t palette_idx, void *data);
  void (*get_put_pixel(void))(int, int, uint8_t, void *);
    
signals:
  void pbuf_full();
  
private:
  std::array<uint8_t, nes_screen_size> pbuf;
};

  
class PatternTableViewer : public QWidget {

  Q_OBJECT

public:
  PatternTableViewer(uint8_t is_right, QWidget *parent = nullptr);

  void draw_pattern_table(void);
  friend void pt_put_pixel(int y, int x, uint8_t palette_idx, void *pt_viewer);
protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::array<uint8_t, pattern_table_size> pbuf;
  uint8_t is_right;
};


#endif
