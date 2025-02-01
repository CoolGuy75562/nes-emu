/* WIP */

#include <QTimer>
#include <QtDebug>

#include "openglwidget.h"

/* vec3 vertex co-ordinates then vec2 texture co-ordinates */
static const GLfloat vertices[] = {
    -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,
     1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
     1.0f,  1.0f,  0.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  0.0f,  0.0f,  1.0f
};
  
OpenGLWidget::OpenGLWidget(QWidget *parent) : QOpenGLWidget(parent) {
  program = new QOpenGLShaderProgram(this);
  pbuf_ptr = nullptr;
}

OpenGLWidget::~OpenGLWidget() {
  makeCurrent();

  vbo.release();
  vao.release();
  if (program != nullptr) {
    program->removeAllShaders();
  }
  vao.destroy();
  vbo.destroy();
  
  doneCurrent();
}

void OpenGLWidget::initScreen(NESScreen *s) {
  pbuf_ptr = s->getPBufPtr();
  connect(s, SIGNAL(pbuf_full()), this, SLOT(update()));
}

void OpenGLWidget::resizeGL(int w, int h) {}

void OpenGLWidget::initializeGL() {
  initializeOpenGLFunctions();

  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                        "#version 330\n"
                                        "layout (location = 0) in vec3 vPos;\n"
                                        "layout (location = 1) in vec2 vTex;\n"
                                        "out vec2 texCoords;\n"
                                        "void main(void)\n"
                                        "{\n"
                                        "    gl_Position = vec4(vPos, 1.0);\n"
                                        "    texCoords = vTex;\n"
                                        "}")) {
    qDebug() << program->log();
  }

  if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                        "#version 330\n"
                                        "in vec2 texCoords;\n"
                                        "out vec4 color;\n"
                                        "uniform sampler2D tex;\n"
                                        "void main(void)\n"
                                        "{\n"
                                        "    color = texture(tex, texCoords);\n"
                                        "}")) {
    qDebug() << program->log();
  }

  program->link();
  program->bind();
  
  vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  vbo.create();
  vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  vbo.bind();
  vbo.allocate(vertices, sizeof(vertices));
  
  vao.create();
  vao.bind();

  /* vertex coordinates */
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                        (void *)0);

  /* texture coordinates */
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                        (void *)(3 * sizeof(GLfloat)));
}


void OpenGLWidget::paintGL() {

  glViewport(0, 0, width(), height());
  glClear(GL_COLOR_BUFFER_BIT);
 
  GLuint texture_id;
  glGenTextures(1, &texture_id);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  program->setUniformValue("tex", 0);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nes_screen_width, nes_screen_height,
  0, GL_RGB, GL_UNSIGNED_BYTE, pbuf_ptr->data());
  glEnable(GL_TEXTURE_2D);
  glGenerateMipmap(GL_TEXTURE_2D);
  
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glDisable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}
