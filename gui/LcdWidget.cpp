#include <emu/Gpu.hpp>
#include "LcdWidget.hpp"

#include <algorithm>

#define CHECK_GL() [](){ \
        GLint err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            qDebug() << __FILE__ << ":" << __LINE__ << " - glGetError() == " << err; \
            exit(1); \
        } \
    }()

static const QVector2D vertexes[] = {
        QVector2D(-1, -1), // bottom left
        QVector2D(+1, -1), // bottom right
        QVector2D(-1, +1), // top left
        QVector2D(+1, +1), // top right
};

QGLFormat& LcdWidget::createGLFormat() {
    static QGLFormat f = QGLFormat::defaultFormat();
    f.setSwapInterval(0);
    return f;
}

void LcdWidget::setupTexture(QOpenGLTexture& glTexture) {
    glTexture.setFormat(QOpenGLTexture::R8_UNorm);
    glTexture.setMipLevels(1);
    glTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    glTexture.allocateStorage();
    glTexture.create();
    glTexture.bind();
}

void LcdWidget::makeGridTexture(QOpenGLTexture& gridTexture, int size) {
    gridTexture.setSize(size);
    setupTexture(gridTexture);
    unsigned char buf[8192];
    for (int i = 0; i < size; ++i) {
        if (i % 17 == 16) {
            buf[i] = 0xff;
        } else {
            buf[i] = std::min(int(round(0xff * (double((i % 17) / 2) / 7.0))), 0xfe);
        }
    }
    gridTexture.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (void*)buf);
}

void LcdWidget::initializeGL() {
    if (textureSize.isEmpty()) {
        return;
    }

    glGetError();
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    texture.setSize(textureSize.width(), textureSize.height());
    setupTexture(texture);

    glActiveTexture(GL_TEXTURE1);
    makeGridTexture(xAxisGridTexture, size().width());
    glActiveTexture(GL_TEXTURE2);
    makeGridTexture(yAxisGridTexture, size().height());

    vertexShader = new QGLShader(QGLShader::Vertex, this);
    const char* vsrc =
            "attribute highp vec2 vertex;\n" \
            "varying highp vec2 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    gl_Position = vec4(vertex, 0.0, 1.0);\n" \
            "    texc = vec2(0.0, 1.0) + vec2(1.0, -1.0) * (vertex + vec2(1.0, 1.0)) / 2.0;\n" \
            "}\n";
    vertexShader->compileSourceCode(vsrc);

    fragmentShader = new QGLShader(QGLShader::Fragment, this);
    QString fileName = QCoreApplication::instance()->applicationDirPath() +
            QString("/shaders/") + fragmentShaderFile;
    fragmentShader->compileSourceFile(fileName);

    shaderProgram = new QGLShaderProgram(this);
    shaderProgram->addShader(vertexShader);
    shaderProgram->addShader(fragmentShader);
    shaderProgram->bindAttributeLocation("vertex", 0);

    // XXX: huge hack
    shaderProgram->bindAttributeLocation("bgPatternBaseSelect", 1);
    shaderProgram->bindAttributeLocation("bgTileBaseSelect", 2);

    shaderProgram->bindAttributeLocation("textureHeight", 3);
    shaderProgram->bindAttributeLocation("textureWidth", 4);

    shaderProgram->link();
    if (!shaderProgram->isLinked()) {
        throw "Shader compiling/linking failed";
    }

    shaderProgram->bind();
    shaderProgram->setUniformValue("texture", 0);
    shaderProgram->setUniformValue("xAxisGrid", 1);
    shaderProgram->setUniformValue("yAxisGrid", 2);

    shaderProgram->enableAttributeArray(0);
    CHECK_GL();
    shaderProgram->setAttributeArray(0, &vertexes[0]);
    CHECK_GL();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    shaderProgram->setUniformValue("textureWidth", size().width());
    shaderProgram->setUniformValue("textureHeight", size().height());
}

void LcdWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void LcdWidget::paintGL() {
    if (drawCallback) {
        drawCallback(this);
    }
    texture.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (void*)textureData);
    CHECK_GL();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_GL();
}
