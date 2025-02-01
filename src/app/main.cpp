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

extern "C" void log_none(const char *format, ...) {}

int main(int argc, char **argv) {

  QApplication a(argc, argv);
  std::unique_ptr<MainWindow> w;
  
  ppu_register_error_callback(&log_none);
  cpu_register_error_callback(&log_none);
  try {
    w.reset(new MainWindow());
  } catch (NESError &e) {
    exit(EXIT_FAILURE);
  }
  /* for some reason I have to do this by hand or else
   the programme keeps running after I close the window */
  w->show();
  QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  return a.exec();
}
