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
    TexCoord,
};

void LcdWidget::initializeGL() {
    glGetError();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    texture.setSize(ScreenWidth, ScreenHeight);
    texture.setFormat(QOpenGLTexture::R8_UNorm);
    texture.setMipLevels(1);
    texture.allocateStorage();
    texture.create();

    vertexShader = new QGLShader(QGLShader::Vertex, this);
    const char* vsrc =
            "attribute highp vec2 vertex;\n" \
            "attribute mediump vec2 texCoord;\n" \
            "varying mediump vec2 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    gl_Position = vec4(vertex, 0.0, 1.0);\n" \
            "    texc = (vertex + vec2(1.0, 1.0)) / 2.0;\n" \
            "}\n";
    vertexShader->compileSourceCode(vsrc);

    fragmentShader = new QGLShader(QGLShader::Fragment, this);
    const char* fsrc =
            "uniform sampler2D texture;\n" \
            "varying mediump vec2 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    vec4 colorIndex = texture2D(texture, texc);\n" \
            "    gl_FragColor = vec4(colorIndex.r/3.0, colorIndex.r/3.0, colorIndex.r/3.0, 1.0);\n" \
            "    gl_FragColor = vec4(0.0, texc.s, texc.t, 1.0);\n" \
            "}\n";
    fragmentShader->compileSourceCode(fsrc);

    shaderProgram = new QGLShaderProgram(this);
    shaderProgram->addShader(vertexShader);
    shaderProgram->addShader(fragmentShader);
    shaderProgram->bindAttributeLocation("vertex", Attr::Vertex);
    shaderProgram->bindAttributeLocation("texCoord", Attr::TexCoord);
    shaderProgram->link();
    if (!shaderProgram->isLinked()) {
        throw "Shader compiling/linking failed";
    }

    shaderProgram->bind();
    shaderProgram->setUniformValue("texture", texture.textureId());
}
void LcdWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

static const QVector2D vertexes[] = {
        QVector2D(-1, -1), // bottom left
        QVector2D(+1, -1), // bottom right
        QVector2D(-1, +1), // top left
        QVector2D(+1, +1), // top right
};
static const QVector2D texCoords[] = {
        QVector2D(0, 0), // bottom left
        QVector2D(1, 0), // bottom right
        QVector2D(0, 1), // top left
        QVector2D(1, 1), // top right
};

void LcdWidget::paintGL() {
    glClearColor(0, 1, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL();

    texture.bind();
    CHECK_GL();

    texture.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (void*)framebuffer);
    CHECK_GL();

    shaderProgram->enableAttributeArray(Attr::Vertex);
    CHECK_GL();
    shaderProgram->enableAttributeArray(Attr::TexCoord);
    shaderProgram->setAttributeArray (Attr::Vertex, &vertexes[0]);
    CHECK_GL();
    shaderProgram->setAttributeArray (Attr::TexCoord, &texCoords[0]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_GL();
}
