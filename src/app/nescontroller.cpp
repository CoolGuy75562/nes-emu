#include <QDebug>

#include "nescontroller.h"
extern "C" {
#include "core/controller.h"
}

uint8_t get_pressed_buttons(void *controller) {
  static NESController *nes_controller =
      static_cast<NESController *>(controller);
  qDebug() << QStringLiteral("%1").arg(nes_controller->buttons_pressed, 8, 2, QLatin1Char('0'));
  return nes_controller->buttons_pressed;
}

void NESController::nes_button_pressed(int key) {
  switch (key) {
  case Qt::Key_W:
    buttons_pressed |= CTRLR_BUTTON_UP;
    break;
  case Qt::Key_A:
    buttons_pressed |= CTRLR_BUTTON_LEFT;
    break;
  case Qt::Key_S:
    buttons_pressed |= CTRLR_BUTTON_DOWN;
    break;
  case Qt::Key_D:
    buttons_pressed |= CTRLR_BUTTON_RIGHT;
    break;
  case Qt::Key_M:
    buttons_pressed |= CTRLR_BUTTON_A;
    break;
  case Qt::Key_Comma:
    buttons_pressed |= CTRLR_BUTTON_B;
    break;
  case Qt::Key_Period: // Like fullstop??
    buttons_pressed |= CTRLR_BUTTON_SELECT;
    break;
  case Qt::Key_Slash:
    buttons_pressed |= CTRLR_BUTTON_START;
    break;
  default:
    break;
  }
}

void NESController::nes_button_released(int key) {
  switch (key) {
  case Qt::Key_W:
    buttons_pressed &= ~CTRLR_BUTTON_UP;
    break;
  case Qt::Key_A:
    buttons_pressed &= ~CTRLR_BUTTON_LEFT;
    break;
  case Qt::Key_S:
    buttons_pressed &= ~CTRLR_BUTTON_DOWN;
    break;
  case Qt::Key_D:
    buttons_pressed &= ~CTRLR_BUTTON_RIGHT;
    break;
  case Qt::Key_M:
    buttons_pressed &= ~CTRLR_BUTTON_A;
    break;
  case Qt::Key_Comma:
    buttons_pressed &= ~CTRLR_BUTTON_B;
    break;
  case Qt::Key_Period: // Like fullstop??
    buttons_pressed &= ~CTRLR_BUTTON_SELECT;
    break;
  case Qt::Key_Slash:
    buttons_pressed &= ~CTRLR_BUTTON_START;
    break;
  default:
    break;
  } 
}
