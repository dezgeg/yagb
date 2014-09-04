#pragma once

#include <QString>
#include <QToolButton>

class OneCharacterButton : public QToolButton {
Q_OBJECT

public:
    explicit OneCharacterButton(QWidget* parent = nullptr, QString text = "") :
            QToolButton(parent) {
        setCheckable(true);
        setText(text);
    }
};
