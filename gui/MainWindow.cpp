#include "emu/Utils.hpp"
#include "gui/HexTextField.hpp"
#include "gui/MainWindow.hpp"
#include "gui/OneCharacterButton.hpp"
#include "ui_MainWindow.h"

#include <QCheckBox>
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
    { 0xff48, QStringLiteral("OBP0") },
    { 0xff49, QStringLiteral("OBP1") },
    { 0xff4a, QStringLiteral("WY") },
    { 0xff4b, QStringLiteral("WX") },
};

static const QString irqNames[] = {
    QStringLiteral("VBlank"),
    QStringLiteral("LcdStat"),
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

    ui->irqsFormLayout->addWidget(new QLabel("Enb"), 0, 1);
    ui->irqsFormLayout->addWidget(new QLabel("Pend"), 0, 2);
    for (unsigned i = 0; i < arraySize(irqNames); i++) {
        ui->irqsFormLayout->addWidget(new QLabel(irqNames[i]), i + 1, 0);

        QCheckBox* enabledBtn = new QCheckBox();
        ui->irqsFormLayout->addWidget(enabledBtn, i + 1, 1);

        QCheckBox* pendingBtn = new QCheckBox();
        ui->irqsFormLayout->addWidget(pendingBtn, i + 1, 2);
    }
}

MainWindow::MainWindow(const char* romFile, bool insnTrace, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    log(ui.get()),
    rom(&log, romFile),
    gb(&log, &rom),
    frameTimer(new QTimer(this)),
    qtFramebuffer(ScreenWidth*2, ScreenHeight*2)
{
    setFocusPolicy(Qt::StrongFocus);

    ui->setupUi(this);
    fillDynamicRegisterTables();

    connect(ui->lcdWidget, SIGNAL(focusChanged(bool)), this, SLOT(lcdFocusChanged(bool)));
    connect(ui->lcdWidget, SIGNAL(paintRequested(QPaintEvent*)), this, SLOT(lcdPaintRequested(QPaintEvent*)));
    connect(ui->lcdWidget, SIGNAL(keyEvent(QKeyEvent*)), this, SLOT(lcdKeyEvent(QKeyEvent*)));

    connect(ui->patternViewerLcdWidget, SIGNAL(paintRequested(QPaintEvent*)),
            this, SLOT(patternViewerPaintRequested(QPaintEvent*)));
    connect(ui->tileMapViewerLcdWidget, SIGNAL(paintRequested(QPaintEvent*)),
            this, SLOT(tileMapViewerPaintRequested(QPaintEvent*)));

    connect(frameTimer, SIGNAL(timeout()), this, SLOT(timerTick()));
    frameTimer->start(4);

    ui->lcdWidget->setFocus();
    updateRegisters();

    log.insnLoggingEnabled = insnTrace;
}

void MainWindow::timerTick()
{
    gb.runFrame();
    ui->lcdWidget->repaint();
    ui->patternViewerLcdWidget->repaint();
    ui->tileMapViewerLcdWidget->repaint();
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

void MainWindow::lcdKeyEvent(QKeyEvent* e)
{
    Byte keys;
    switch (e->key()) {
        case Qt::Key_Right:     keys = Pad_Right;   break;
        case Qt::Key_Left:      keys = Pad_Left;    break;
        case Qt::Key_Up:        keys = Pad_Up;      break;
        case Qt::Key_Down:      keys = Pad_Down;    break;
        case Qt::Key_Return:    keys = Pad_Start;   break;
        case Qt::Key_Backspace: keys = Pad_Select;  break;
        case Qt::Key_Control:   keys = Pad_A;       break;
        case Qt::Key_Shift:     keys = Pad_B;       break;
        default: return;
    }

    if (e->type() == QEvent::KeyPress)
        gb.getJoypad()->keysPressed(keys);
    else
        gb.getJoypad()->keysReleased(keys);
}

static const QVector<QRgb> monochromeToRgb = {
    qRgb(255, 255, 255),
    qRgb(2*255/3, 2*255/3, 2*255/3),
    qRgb(255/3, 255/3, 255/3),
    qRgb(0, 0, 0),
};

void MainWindow::lcdPaintRequested(QPaintEvent*)
{
    QImage image((const uchar*)gb.getGpu()->getFramebuffer(),
                 ScreenWidth, ScreenHeight, QImage::Format_Indexed8);
    image.setColorTable(monochromeToRgb); // TODO: copies?

    // TODO: draw border
    QPainter painter;
    painter.begin(ui->lcdWidget);
    painter.drawImage(QRectF(QPointF(1, 1), QSizeF(ScreenWidth*2, ScreenHeight*2)), image);
    painter.end();
}

static void drawTile(int i, int j, Byte* tile, QPainter* painter)
{
    unsigned char tmpBuf[8][8];
    QImage image((uchar*)tmpBuf, 8, 8, QImage::Format_Indexed8);
    image.setColorTable(monochromeToRgb); // TODO: copies?

    for (unsigned x = 0; x < 8; x++)
        for (unsigned y = 0; y < 8; y++)
            tmpBuf[y][x] = Gpu::drawTilePixel(tile, x, y, 0xe4);

    painter->drawImage(QRectF(QPointF(17 * i, 17 * j), QSizeF(16, 16)), image,
                       QRectF(QPointF(0, 0), QSize(8, 8)));
}

void MainWindow::patternViewerPaintRequested(QPaintEvent*)
{
    Byte* vram = gb.getGpu()->getVram();
    QPainter painter;
    painter.begin(ui->patternViewerLcdWidget);
    for (unsigned i = 0; i < 16; i++) {
        for (unsigned j = 0; j < 24; j++) {
            drawTile(i, j, &vram[16 * (16 * j + i)], &painter);
        }
    }
    painter.end();
}

void MainWindow::tileMapViewerPaintRequested(QPaintEvent*)
{
    Byte* vram = gb.getGpu()->getVram();
    Byte* tiles = vram + 0x1800;
    unsigned char tmpBuf[8][8];
    QImage image((uchar*)tmpBuf, 8, 8, QImage::Format_Indexed8);
    image.setColorTable(monochromeToRgb); // TODO: copies?

    QPainter painter;
    painter.begin(ui->tileMapViewerLcdWidget);
    for (unsigned i = 0; i < 32; i++) {
        for (unsigned j = 0; j < 32; j++) {
            drawTile(i, j, &vram[16 * tiles[32 * j + i]], &painter);
        }
    }
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

    Byte irqsEnabled = bus->getEnabledIrqs();
    Byte irqsPending = bus->getPendingIrqs();
    for (unsigned i = 0; i < Irq_Max; i++) {
        unsigned mask = (1 << i);
        QAbstractButton* enb = (QAbstractButton*)ui->irqsFormLayout->itemAtPosition(i + 1, 1)->widget();
        QAbstractButton* pend = (QAbstractButton*)ui->irqsFormLayout->itemAtPosition(i + 1, 2)->widget();

        enb->setChecked(irqsEnabled & mask);
        pend->setEnabled(irqsEnabled & mask);
        pend->setChecked(irqsPending & mask);
    }

    Regs* regs = gb.getCpu()->getRegs();
    ui->cpuRegsAf->setHex(regs->af);
    ui->cpuRegsBc->setHex(regs->bc);
    ui->cpuRegsDe->setHex(regs->de);
    ui->cpuRegsHl->setHex(regs->hl);
    ui->cpuRegsSp->setHex(regs->sp);
    ui->cpuRegsPc->setHex(regs->pc);
    ui->cpuFlagZ->setChecked(regs->flags.z);
    ui->cpuFlagN->setChecked(regs->flags.n);
    ui->cpuFlagH->setChecked(regs->flags.h);
    ui->cpuFlagC->setChecked(regs->flags.c);
    ui->cpuFlagIrqs->setChecked(regs->irqsEnabled);
}

MainWindow::~MainWindow()
{
}

void GuiLogger::logImpl(const char* format, ...)
{
    QString s;
    va_list ap;
    va_start(ap, format);
    s.vsprintf(format, ap);
    va_end(ap);

    ui->logTextarea->moveCursor(QTextCursor::End);
    ui->logTextarea->insertPlainText(s);
    ui->logTextarea->moveCursor(QTextCursor::End);
    ui->logTextarea->insertPlainText("\n");
    ui->logTextarea->moveCursor(QTextCursor::End);
}
