#pragma once

#include "emu/Platform.hpp"
#include <QWidget>
#include <QGLWidget>
#include <QtOpenGL>

class LcdWidget : public QGLWidget {
Q_OBJECT
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QOpenGLTexture texture;

    Byte* textureData;
    QSize textureSize;
    char const* fragmentShaderSource;

    static QGLFormat& createGLFormat() {
        static QGLFormat f = QGLFormat::defaultFormat();
        f.setSwapInterval(0);
        return f;
    }

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void focusInEvent(QFocusEvent*) override {
        emit focusChanged(true);
    }
    virtual void focusOutEvent(QFocusEvent*) override {
        emit focusChanged(false);
    }
    virtual void paintEvent(QPaintEvent* e) override {
        emit paintRequested(e);
    }
    virtual void keyPressEvent(QKeyEvent* e) override {
        emit keyEvent(e);
    }
    virtual void keyReleaseEvent(QKeyEvent* e) override {
        emit keyEvent(e);
    }

public:

    explicit LcdWidget(QWidget* parent) :
            QGLWidget(createGLFormat(), parent),
            texture(QOpenGLTexture::Target2D) {
        setFocusPolicy(Qt::StrongFocus);
    }

    void init(Byte* textureData, QSize size, char const* shader) {
        this->textureData = textureData;
        this->textureSize = size;
        this->fragmentShaderSource = shader;
    }
signals:
    void focusChanged(bool in);
    void paintRequested(QPaintEvent*);
    void keyEvent(QKeyEvent*);
};
