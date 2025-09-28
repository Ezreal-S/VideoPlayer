#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "player.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    player = new Player(ui->openGLWidget);
    player->openFile("lmtc.mp4");
    player->play();
}

MainWindow::~MainWindow()
{
    delete player;
    delete ui;
}
