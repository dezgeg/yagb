#include <assert.h>
#include "LcdWidget.hpp"

enum Attr {
    Vertex,
    TexCoord,
};

void LcdWidget::initializeGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    vertexShader = new QGLShader(QGLShader::Vertex, this);
    const char* vsrc =
            "attribute highp vec2 vertex;\n" \
            "// attribute mediump vec4 texCoord;\n" \
            "// varying mediump vec4 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    gl_Position = vec4(vertex, 0.0, 1.0);\n" \
            "//     texc = texCoord;\n" \
            "}\n";
    vertexShader->compileSourceCode(vsrc);

    fragmentShader = new QGLShader(QGLShader::Fragment, this);
    const char* fsrc =
            "// uniform sampler2D texture;\n" \
            "// varying mediump vec4 texc;\n" \
            "void main(void)\n" \
            "{\n" \
            "    gl_FragColor = vec4(1, 0, 0, 1);// texture2D(texture, texc.st);\n" \
            "}\n";
    fragmentShader->compileSourceCode(fsrc);

    shaderProgram = new QGLShaderProgram(this);
    shaderProgram->addShader(vertexShader);
    shaderProgram->addShader(fragmentShader);
    shaderProgram->bindAttributeLocation("vertex", Attr::Vertex);
    //shaderProgram->bindAttributeLocation("texCoord", Attr::TexCoord);
    shaderProgram->link();

    shaderProgram->bind();
    shaderProgram->setUniformValue("texture", 0);
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

#define CHECK_GL() assert(glGetError() == GL_NO_ERROR)

void LcdWidget::paintGL() {
    glClearColor(0, 1, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL();

    shaderProgram->enableAttributeArray(Attr::Vertex);
    CHECK_GL();
    //shaderProgram->enableAttributeArray(Attr::TexCoord);
    shaderProgram->setAttributeArray (Attr::Vertex, &vertexes[0]);
    CHECK_GL();
    //shaderProgram->setAttributeArray (Attr::TexCoord, &texCoords[0]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_GL();
}
