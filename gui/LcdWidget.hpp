#pragma once

#include <QWidget>

class LcdWidget : public QWidget {
Q_OBJECT

protected:
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
    explicit LcdWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = 0) : QWidget(parent, flags) {
        setAttribute(Qt::WA_OpaquePaintEvent);
        setFocusPolicy(Qt::StrongFocus);
    }

signals:
    void focusChanged(bool in);
    void paintRequested(QPaintEvent*);
    void keyEvent(QKeyEvent*);
};
