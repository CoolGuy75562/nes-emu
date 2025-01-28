#ifndef OPENGLWIDGET_H_
#define OPENGLWIDGET_H_

#include <array>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include "nesscreen.h"



class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions{

  Q_OBJECT
 
public:
  OpenGLWidget(QWidget *parent);
  ~OpenGLWidget();

  /* connect pbuf_full signal to update slot, and set pbuf_ptr to pbuf in s */
  void initScreen(NESScreen *s);
  
protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  std::array<uint8_t, nes_screen_size> *pbuf_ptr;
  QOpenGLVertexArrayObject vao;
  QOpenGLBuffer vbo;
  QOpenGLShaderProgram *program;
};

#endif
