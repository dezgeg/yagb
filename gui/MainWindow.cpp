#include "emu/Utils.hpp"
#include "gui/HexTextField.hpp"
#include "gui/MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QString>
#include <utility>
using namespace std;

static const pair<unsigned, QString> lcdRegs[] = {
    { 0xff40, QStringLiteral("LCDC") },
    { 0xff41, QStringLiteral("STAT") },
    { 0xff42, QStringLiteral("SCY") },
    { 0xff43, QStringLiteral("SCX") },
    { 0xff44, QStringLiteral("LY") },
    { 0xff45, QStringLiteral("LYC") },
    { 0xff46, QStringLiteral("DMA") },
    { 0xff47, QStringLiteral("BGP") },
    { 0xff4a, QStringLiteral("WY") },
    { 0xff4b, QStringLiteral("WX") },
};

static const QString irqNames[] = {
    QStringLiteral("V-Blank"),
    QStringLiteral("LCD STAT"),
    QStringLiteral("Timer"),
    QStringLiteral("Serial"),
    QStringLiteral("Joypad"),
};

void MainWindow::fillDynamicRegisterTables()
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);

    for (unsigned i = 0; i < arraySize(lcdRegs); i++) {
        QLabel* label = new QLabel(lcdRegs[i].second);
        ui->lcdRegsFormLayout->setWidget(i, QFormLayout::LabelRole, label);
        HexTextField* edit = new HexTextField();
        edit->setFont(font);
        ui->lcdRegsFormLayout->setWidget(i, QFormLayout::FieldRole, edit);
    }

    for (unsigned i = 0; i < arraySize(irqNames); i++) {
        ui->irqsFormLayout->addWidget(new QLabel(irqNames[i]), i, 0);

        QPushButton* enabledBtn = new QPushButton(QStringLiteral("Enabled"));
        ui->irqsFormLayout->addWidget(enabledBtn, i, 1);

        QPushButton* pendingBtn = new QPushButton(QStringLiteral("Pending"));
        ui->irqsFormLayout->addWidget(pendingBtn, i, 2);
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    log(false),
    rom(&log, "test.bin"),
    gb(&log, &rom),
    frameTimer(new QTimer(this)),
    qtFramebuffer(ScreenWidth*2, ScreenHeight*2),
    ui(new Ui::MainWindow)
{
    setFocusPolicy(Qt::StrongFocus);

    ui->setupUi(this);
    fillDynamicRegisterTables();

    connect(ui->lcdWidget, SIGNAL(focusChanged(bool)), this, SLOT(lcdFocusChanged(bool)));
    connect(ui->lcdWidget, SIGNAL(paintRequested(QPaintEvent*)), this, SLOT(lcdPaintRequested(QPaintEvent*)));

    connect(frameTimer, SIGNAL(timeout()), this, SLOT(timerTick()));
    frameTimer->start(17);

    ui->lcdWidget->setFocus();
    updateRegisters();
}

void MainWindow::timerTick()
{
    gb.runFrame();
    ui->lcdWidget->repaint();
    if (gb.getGpu()->getCurrentFrame() % 60 == 0)
        updateRegisters();
}

void MainWindow::lcdFocusChanged(bool in)
{
    if (in) {
        frameTimer->start();
    } else {
        frameTimer->stop();
        updateRegisters();
    }
}

void MainWindow::lcdPaintRequested(QPaintEvent*)
{
    static const QVector<QRgb> monochromeToRgb = {
        qRgb(255, 255, 255),
        qRgb(2*255/3, 2*255/3, 2*255/3),
        qRgb(255/3, 255/3, 255/3),
        qRgb(0, 0, 0),
    };

    QImage image((const uchar*)gb.getGpu()->getFramebuffer(),
                 ScreenWidth, ScreenHeight, QImage::Format_Indexed8);
    image.setColorTable(monochromeToRgb); // TODO: copies?

    // TODO: draw border
    QPainter painter;
    painter.begin(ui->lcdWidget);
    painter.drawImage(QRectF(QPointF(1, 1), QSizeF(ScreenWidth*2, ScreenHeight*2)), image);
    painter.end();
}

void MainWindow::updateRegisters()
{
    Bus* bus = gb.getBus();
    for (unsigned i = 0; i < arraySize(lcdRegs); i++) {
        HexTextField* edit = static_cast<HexTextField*>(ui->lcdRegsFormLayout->itemAt(i, QFormLayout::FieldRole)->widget());
        unsigned reg = lcdRegs[i].first;
        edit->setHex(bus->memRead8(reg));
    }

    Regs* regs = gb.getCpu()->getRegs();
    ui->cpuRegsAf->setHex(regs->af);
    ui->cpuRegsBc->setHex(regs->bc);
    ui->cpuRegsDe->setHex(regs->de);
    ui->cpuRegsHl->setHex(regs->hl);
    ui->cpuRegsSp->setHex(regs->sp);
    ui->cpuRegsPc->setHex(regs->pc);
}

MainWindow::~MainWindow()
{
}
