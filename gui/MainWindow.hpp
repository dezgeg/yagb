#pragma once

#include "emu/Gameboy.hpp"
#include "emu/Logger.hpp"
#include "emu/Rom.hpp"
#include "AudioHandler.hpp"

#include <QMainWindow>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QTimer>
#include <memory>

namespace Ui { class MainWindow; }

class GuiLogger : public Logger {
    Ui::MainWindow* ui;

public:
    GuiLogger(Ui::MainWindow* ui) :
            ui(ui) {
    }

    virtual void logImpl(const char* format, ...) override;
};

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(const char* romFile, bool gbc, bool insnTrace, QWidget* parent = 0);
    ~MainWindow();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    AudioHandler audioHandler;

    GuiLogger log;
    Rom rom;
    Gameboy gb;

    long nextRenderAt;
    QTimer* frameTimer;

    void fillDynamicRegisterTables();
    void updateRegisters();

private slots:
    void timerTick();

    void lcdFocusChanged(bool);
    void lcdKeyEvent(QKeyEvent*);
    void saveGameState();
    void loadGameState();
};
