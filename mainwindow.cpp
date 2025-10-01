#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "player.h"
#include <QFileDialog>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 初始化信号
    connectInit();
    player = new Player(ui->openGLWidget);

}

MainWindow::~MainWindow()
{
    delete player;
    delete ui;
}

void MainWindow::connectInit()
{
    connect(ui->ctrlBar,&CtrlBar::openFileClicked,[this](){
        QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("选择媒体文件"),
            QString(),
            tr("视频文件 (*.mp4 *.avi *.mkv);;音频文件 (*.mp3 *.wav);;所有文件 (*.*)")
            );
        if(!filePath.isEmpty()){
            qDebug()<<filePath;
            player->openFile(filePath.toStdString());
            player->play();
            // 开始播放后应该更新播放/暂停键状态
            emit ui->ctrlBar->updatePlayBtnState(true);
        }

    });

    connect(ui->ctrlBar,&CtrlBar::stopClicked,this,[this]{
        player->stop();
        ui->openGLWidget->setFrame(nullptr);
        // 停止播放后应该更新播放/暂停键状态
        emit ui->ctrlBar->updatePlayBtnState(false);
    });

    connect(ui->ctrlBar,&CtrlBar::playClicked,this,[this]{
        auto state = player->getState();
        if(state == MediaState::Stop){
            if(player->play()){
                emit ui->ctrlBar->updatePlayBtnState(true);
                return;
            }
            emit ui->ctrlBar->updatePlayBtnState(false);
            return;
        }
        if(state == MediaState::Play){
            player->pause();
            emit ui->ctrlBar->updatePlayBtnState(false);
        }
        else if(state == MediaState::Pause){
            player->pause();
            emit ui->ctrlBar->updatePlayBtnState(true);
        }

    });
}
