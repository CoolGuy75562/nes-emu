/* Copyright (C) 2024, 2025  Angus McLean
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QtDebug>

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "core/cppwrapper.hpp"
#include "mainwindow.h"
#include "nescontext.h"
#include "nesscreen.h"

extern "C" void log_none(const char *format, ...) {}
extern "C" void log_memory_none(uint16_t addr, uint8_t val, void *data) {}

static void init(MainWindow &w);

int main(int argc, char **argv) {

  QApplication a(argc, argv);
  MainWindow w;
  /* for some reason I have to do this by hand or else
   the programme keeps running after I close the window */
  QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  
  init(w);
  return a.exec();
}


static void init(MainWindow &w) {
  qDebug() << "Initialising MainWindow";
  NESContext *nes_context;
  w.show();

  const std::string rom_filename =
      QFileDialog::getOpenFileName(&w, "Open File", "/home", "NES ROM (*.nes)")
          .toStdString();

  /* for now */
  cpu_register_error_callback(&log_none);
  ppu_register_error_callback(&log_none);
  memory_register_cb(&log_memory_none, NULL, MEMORY_CB_FETCH);
  memory_register_cb(&log_memory_none, NULL, MEMORY_CB_WRITE);

  qDebug() << "Initialising NESScreen";
  NESScreen *s = new NESScreen(&w);
  w.initOpenGLWidgetNESScreen(s);
  
  try {
    qDebug() << "Initialising NESContext";
    /* here s is just callback data, NESContext doesn't care about NESScreen */
    nes_context = new NESContext(rom_filename, s->get_put_pixel(), s, &w);
  } catch (NESError &e) {
    show_error(&w, e);
    exit(EXIT_FAILURE);
  }

  /*========================Signals and Slots===========================*/
  
  /* when nes execution is done */
  QObject::connect(nes_context, SIGNAL(nes_done()), &w, SLOT(done()));

  QObject::connect(nes_context, SIGNAL(nes_error(NESError &)), &w,
                   SLOT(error(NESError &)));

  /* cpu steps when 0ms timer times out, and stop/start buttons
   * control whether timer is going or not.
   */
  QTimer *timer = new QTimer(&w);
  QObject::connect(timer, SIGNAL(timeout()), nes_context, SLOT(nes_step()));
  QObject::connect(&w, SIGNAL(play_button_clicked()), timer, SLOT(start()));
  QObject::connect(&w, SIGNAL(pause_button_clicked()), timer, SLOT(stop()));

  QObject::connect(&w, SIGNAL(step_button_clicked()), timer, SLOT(stop()));
  QObject::connect(&w, SIGNAL(step_button_clicked()), nes_context, SLOT(nes_step()));
  qDebug() << "Init done";
}
