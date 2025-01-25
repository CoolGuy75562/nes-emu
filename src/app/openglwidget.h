#ifndef OPENGLWIDGET_H_
#define OPENGLWIDGET_H_

#include <array>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#include "nesscreen.h"



class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions{

  Q_OBJECT
 
public:
  OpenGLWidget(QWidget *parent);
  ~OpenGLWidget();
  
  void initScreen(NESScreen *s);
			      
public slots:
  void draw_pbuf();
  
protected:
  void initializeGL() override;
  void paintGL() override;
  NESScreen *screen;

private:
  std::array<uint8_t, nes_screen_size> *pbuf_ptr;
  QOpenGLVertexArrayObject vao;
  QOpenGLBuffer vbo;
};

#endif
