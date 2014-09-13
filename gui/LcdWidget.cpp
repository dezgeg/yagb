#include <assert.h>
#include <emu/Gpu.hpp>
#include "LcdWidget.hpp"

#define CHECK_GL() [](){ \
        GLint err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            qDebug() << __FILE__ << ":" << __LINE__ << " - glGetError() == " << err; \
            exit(1); \
        } \
    }()


enum Attr {
    Vertex,
};

static const QVector2D vertexes[] = {
        QVector2D(-1, -1), // bottom left
        QVector2D(+1, -1), // bottom right
        QVector2D(-1, +1), // top left
        QVector2D(+1, +1), // top right
};

void LcdWidget::initializeGL() {
    glGetError();
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    texture.setSize(ScreenWidth, ScreenHeight);
    texture.setFormat(QOpenGLTexture::R8_UNorm);
    texture.setMipLevels(1);
    texture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    texture.allocateStorage();
    texture.create();

    vertexShader = new QGLShader(QGLShader::Vertex, this);
    const char* vsrc =
            "attribute highp vec2 vertex;\n" \
            "varying mediump vec2 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    gl_Position = vec4(vertex, 0.0, 1.0);\n" \
            "    texc = vec2(1.0, -1.0) * (vertex + vec2(1.0, 1.0)) / 2.0;\n" \
            "}\n";
    vertexShader->compileSourceCode(vsrc);

    fragmentShader = new QGLShader(QGLShader::Fragment, this);
    const char* fsrc =
            "uniform sampler2D texture;\n" \
            "varying mediump vec2 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    float index = texture2D(texture, texc).r * 255.0;\n" \
            "    float grayScale = 1.0 - index/3.0;\n" \
            "    gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);\n" \
            "}\n";
    fragmentShader->compileSourceCode(fsrc);

    shaderProgram = new QGLShaderProgram(this);
    shaderProgram->addShader(vertexShader);
    shaderProgram->addShader(fragmentShader);
    shaderProgram->bindAttributeLocation("vertex", Attr::Vertex);
    shaderProgram->link();
    if (!shaderProgram->isLinked()) {
        throw "Shader compiling/linking failed";
    }

    shaderProgram->bind();
    shaderProgram->setUniformValue("texture", 0);

    shaderProgram->enableAttributeArray(Attr::Vertex);
    CHECK_GL();
    shaderProgram->setAttributeArray (Attr::Vertex, &vertexes[0]);
    CHECK_GL();

    texture.bind();
    CHECK_GL();

}
void LcdWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void LcdWidget::paintGL() {
    texture.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (void*)framebuffer);
    CHECK_GL();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_GL();
}