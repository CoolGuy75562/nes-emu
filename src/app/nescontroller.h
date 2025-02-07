#ifndef NESCONTROLLER_H_
#define NESCONTROLLER_H_

#include <QObject>

extern "C" uint8_t get_pressed_buttons(void *controller);

class NESController : public QObject {

  Q_OBJECT

public:
  explicit NESController(QObject *parent = nullptr) : QObject(parent) {}
  ~NESController() {}
  friend uint8_t get_pressed_buttons(void *controller);
  
public slots:
  void nes_button_pressed(int key);
  void nes_button_released(int key);

private:
  uint8_t buttons_pressed;
};

#endif
