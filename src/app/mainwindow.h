#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "core/cppwrapper.hpp"
#include "openglwidget.h"

void show_error(QWidget *parent, NESError &e);

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void initOpenGLWidgetNESScreen(NESScreen *s);
  
  friend void on_cpu_state_update(cpu_state_s *cpu_state,
                                  void *window_instance);
  friend void on_ppu_state_update(ppu_state_s *ppu_state,
                                  void *window_instance);

public slots:
  void done(void);
  void error(NESError &e);
  
signals:
  void pause_button_clicked();
  void play_button_clicked();
  
private:

  Ui::MainWindow *ui;
  static const QStringList cpu_labels;
  static const QStringList ppu_labels;
  cpu_state_s *cpu_state;
  ppu_state_s *ppu_state;

private slots:
  void on_pauseButton_clicked();
  void on_playButton_clicked();

  void refresh_cpu_state();
  void refresh_ppu_state();
};

#endif // MAINWINDOW_H
