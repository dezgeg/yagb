#pragma once
#include "emu/Gameboy.hpp"
#include "emu/Logger.hpp"
#include "emu/Rom.hpp"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMainWindow>
#include <QPixmap>
#include <QTimer>
#include <memory>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Logger log;
    Rom rom;
    Gameboy gb;

    QTimer* frameTimer;
    QPixmap qtFramebuffer;
    QGraphicsPixmapItem* fbSceneItem;
    std::unique_ptr<Ui::MainWindow> ui;

    void setupUi();

private slots:
    void tick();
};
