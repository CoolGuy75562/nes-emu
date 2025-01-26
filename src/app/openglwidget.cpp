#include <QTimer>
#include <QtDebug>

#include "openglwidget.h"

static const GLfloat vertices[] = {-1.0, -1.0, 0.0, 0.0, 0.0, 1.0, -1.0, 0.0, 1.0, 0.0,
                                   -1.0, 1.0,  0.0, 1.0, 1.0, 1.0, 1.0,  0.0, 0.0, 1.0};

OpenGLWidget::OpenGLWidget(QWidget *parent) : QOpenGLWidget(parent) {
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(draw_pbuf()));
  timer->start(32); // should be ~30fps
  pbuf_ptr = nullptr;
}

OpenGLWidget::~OpenGLWidget() {
  makeCurrent();

  vao.destroy();
  vbo.destroy();

  doneCurrent();
}

void OpenGLWidget::initScreen(NESScreen *s) {
  pbuf_ptr = s->getPBufPtr();
  Q_ASSERT(pbuf_ptr != nullptr);
  //connect(s, SIGNAL(pbuf_full()), this, SLOT(draw_pbuf()));
}

void OpenGLWidget::draw_pbuf() {
  paintGL();
}

void OpenGLWidget::initializeGL() {
  initializeOpenGLFunctions();
  
  vao.create();
  vbo.create();

  vao.bind();
  vbo.bind();
  vbo.allocate(vertices, sizeof(vertices));
}

void OpenGLWidget::paintGL() {
  
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  Q_ASSERT(pbuf_ptr != nullptr);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nes_screen_width, nes_screen_height,
               0, GL_RGB, GL_UNSIGNED_BYTE, pbuf_ptr->data());
  glGenerateMipmap(GL_TEXTURE_2D);
  
  glEnable(GL_TEXTURE_2D);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisable(GL_TEXTURE_2D);
  
  glBindTexture(GL_TEXTURE_2D, 0);

}
