#pragma once

#include <QDebug>
#include <QTextLayout>
#include <QLineEdit>

class HexTextField : public QLineEdit {
Q_OBJECT

    int numChars;

public:
    explicit HexTextField(QWidget* parent = nullptr, int numChars = 2) :
            QLineEdit(parent),
            numChars(numChars) {
        int borderSize = sizeHint().width() - 17 * fontMetrics().width('x'); // MAJOR HACK!

        QFont newFont("Monospace");
        newFont.setStyleHint(QFont::TypeWriter);
        setFont(newFont);

        QSize sz = sizeHint();
        setFixedSize(1 + borderSize + fontMetrics().width(QString(numChars, 'F')), sz.height());
        setMaxLength(numChars);
    }

    void setHex(unsigned val) {
        setText(QString().sprintf("%0*X", numChars, val));
    }
};

class HexTextField4 : public HexTextField {
Q_OBJECT

public:
    HexTextField4(QWidget* parent = nullptr) : HexTextField(parent, 4) {
    }
};
