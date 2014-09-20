#include "emu/Utils.hpp"
#include "gui/HexTextField.hpp"
#include "gui/MainWindow.hpp"
#include "gui/OneCharacterButton.hpp"
#include "ui_MainWindow.h"
#include "TimingUtils.hpp"

using namespace std;
static constexpr double FrameNsecs = (70224.0 / (1L << 22)) * 1000000000L;

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

void MainWindow::fillDynamicRegisterTables() {
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

MainWindow::MainWindow(const char* romFile, bool insnTrace, QWidget* parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        log(ui.get()),
        rom(&log, romFile),
        gb(&log, &rom),
        frameTimer(new QTimer(this)) {
    setFocusPolicy(Qt::StrongFocus);

    ui->setupUi(this);
    fillDynamicRegisterTables();

    connect(ui->lcdWidget, SIGNAL(focusChanged(bool)), this, SLOT(lcdFocusChanged(bool)));
    connect(ui->lcdWidget, SIGNAL(keyEvent(QKeyEvent * )), this, SLOT(lcdKeyEvent(QKeyEvent * )));

    ui->lcdWidget->init(gb.getGpu()->getFramebuffer(), QSize(ScreenWidth, ScreenHeight), "main.frag");
    ui->lcdWidget->setFocus();

    Gpu* gpu = gb.getGpu();
    // FIXME(maybe): vram is copied to texture memory twice
    ui->patternViewerLcdWidget->init(gb.getGpu()->getVram(), QSize(8192, 1), "patternViewer.frag");
    ui->tileMapViewerLcdWidget->init(gb.getGpu()->getVram(), QSize(8192, 1), "tilemapViewer.frag",
            [gpu](LcdWidget* tilemapViewer) {
                QGLShaderProgram* tilemapShader = tilemapViewer->getShaderProgram();
                tilemapShader->setUniformValue("bgPatternBaseSelect", (int)gpu->getRegs()->bgPatternBaseSelect);
                tilemapShader->setUniformValue("bgTileBaseSelect", (int)gpu->getRegs()->bgTileBaseSelect);
    });
    updateRegisters();

    log.insnLoggingEnabled = insnTrace;

    // Skip BootRom
    gb.getGpu()->setRenderEnabled(false);
    while (gb.getGpu()->getCurrentFrame() != 332) {
        gb.runOneInstruction();
    }
    gb.getGpu()->setRenderEnabled(true);

    connect(frameTimer, SIGNAL(timeout()), this, SLOT(timerTick()));
    nextRenderAt = TimingUtils::getNsecs();
    frameTimer->start(0);
}

void MainWindow::timerTick() {
    Gpu* gpu = gb.getGpu();
    Sound* snd = gb.getSound();

    long startTime = TimingUtils::getNsecs();
    long overtime = clamp(startTime - nextRenderAt, -FrameNsecs / 20, FrameNsecs / 20);

    long frame = gpu->getCurrentFrame();
    long sample = snd->getCurrentSampleNumber();
    // TimingUtils::log() << "Frame start, audio sample: " << snd->getCurrentSampleNumber();
    while (true) {
        gb.runOneInstruction();

        if (snd->getCurrentSampleNumber() != sample) {
            audioHandler.feedSamples(snd->getLeftSample(), snd->getRightSample());
            sample = snd->getCurrentSampleNumber();
        }

        if (gpu->getCurrentFrame() != frame) {
            break;
        }
    }
    // TimingUtils::log() << "Frame over, audio sample: " << snd->getCurrentSampleNumber() << ", available: " << audioHandler.samplesAvailable();

    ui->lcdWidget->repaint();
    ui->patternViewerLcdWidget->repaint();
    ui->tileMapViewerLcdWidget->repaint();

    if (gb.getGpu()->getCurrentFrame() % 60 == 0) {
        updateRegisters();
    }

    long endTime = TimingUtils::getNsecs();
    nextRenderAt = startTime - overtime + FrameNsecs;
    long msec = (nextRenderAt - endTime) / 1000000;
    // qDebug() << "msec: " << msec << "overtime: " << overtime;
    frameTimer->start(msec < 0 ? 0 : msec);
}

void MainWindow::lcdFocusChanged(bool in) {
    if (in) {
        frameTimer->start();
    } else {
        frameTimer->stop();
        updateRegisters();
    }
}

void MainWindow::lcdKeyEvent(QKeyEvent* e) {
    Byte keys;
    switch (e->key()) {
        case Qt::Key_Right:
            keys = Pad_Right;
            break;
        case Qt::Key_Left:
            keys = Pad_Left;
            break;
        case Qt::Key_Up:
            keys = Pad_Up;
            break;
        case Qt::Key_Down:
            keys = Pad_Down;
            break;
        case Qt::Key_Return:
            keys = Pad_Start;
            break;
        case Qt::Key_Backspace:
            keys = Pad_Select;
            break;
        case Qt::Key_Control:
            keys = Pad_A;
            break;
        case Qt::Key_Shift:
            keys = Pad_B;
            break;
        default:
            return;
    }

    if (e->type() == QEvent::KeyPress) {
        gb.getJoypad()->keysPressed(keys);
    } else {
        gb.getJoypad()->keysReleased(keys);
    }
}

void MainWindow::updateRegisters() {
    Bus* bus = gb.getBus();
    for (unsigned i = 0; i < arraySize(lcdRegs); i++) {
        HexTextField* edit = static_cast<HexTextField*>(ui->lcdRegsFormLayout->itemAt(i, QFormLayout::FieldRole)->widget());
        unsigned reg = lcdRegs[i].first;
        edit->setHex(bus->memRead8(reg, NULL));
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

MainWindow::~MainWindow() {
}

QTextStream qtStdout(stdout);

void GuiLogger::logImpl(const char* format, ...) {
    QString s;
    va_list ap;
    va_start(ap, format);
    s.vsprintf(format, ap);
    va_end(ap);

    qtStdout << s << "\n";

#if 0
    ui->logTextarea->moveCursor(QTextCursor::End);
    ui->logTextarea->insertPlainText(s);
    ui->logTextarea->moveCursor(QTextCursor::End);
    ui->logTextarea->insertPlainText("\n");
    ui->logTextarea->moveCursor(QTextCursor::End);
#endif
}
