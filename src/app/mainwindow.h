#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "core/cppwrapper.hpp"
#include "openglwidget.h"

void show_error(QWidget *parent, NESError &e);

extern "C" {
void on_cpu_state_update();
void on_ppu_state_update();
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void initOpenGLWidgetNESScreen(NESScreen *s);
  friend void on_cpu_state_update(cpu_state_s *state, void *window);
  friend void on_ppu_state_update(ppu_state_s *state, void *window);

public slots:
  void done(void);
  void error(NESError &e);
  
signals:
  void pause_button_clicked();
  void play_button_clicked();
  void step_button_clicked();
  
private:
  void init_table_widget(std::string which_widget);
  
  Ui::MainWindow *ui;
  static const QStringList cpu_labels;
  static const QStringList ppu_labels;
  cpu_state_s *cpu_state;
  ppu_state_s *ppu_state;

private slots:
  void on_pauseButton_clicked();
  void on_playButton_clicked();
  void on_stepButton_clicked();
  void on_memoryDumpButton_clicked();
  
  void refresh_cpu_state();
  void refresh_ppu_state();
};

#endif // MAINWINDOW_H
