#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "player.h"
#include <QFileDialog>
#include <QDebug>
#include <QShortcut>
#include <QMouseEvent>
#include <QComboBox>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qApp->installEventFilter(this);
    player = new Player(ui->openGLWidget);

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    // 如果超时，则隐藏控制栏
    connect(hideTimer,&QTimer::timeout,this,[=](){
        if(isFullScreen()){
            ui->ctrlBar->hide();
            ui->listWidget->hide();
        }
    });
    // 初始化信号
    connectInit();
    // 初始化热键绑定
    keybindInit();
}

MainWindow::~MainWindow()
{
    delete player;
    delete ui;
}

void MainWindow::connectInit()
{
    connect(ui->ctrlBar,&CtrlBar::openFileClicked,this,[this](){
        QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("选择媒体文件"),
            QString(),
            tr("视频文件 (*.mp4 *.avi *.mkv);;音频文件 (*.mp3 *.wav);;所有文件 (*.*)")
            );
        if(!filePath.isEmpty()){
            qDebug()<<filePath;
            ui->listWidget->loadFromFile(filePath);
            player->openFile(filePath.toStdString());
            player->play();
            // 开始播放后应该更新播放/暂停键状态
            emit ui->ctrlBar->updatePlayBtnState(true);
        }

    });

    // 连接播放列表双击播放事件
    connect(ui->listWidget,&PlaylistWidget::playRequested,this,[this](const QString& filePath){
        player->openFile(filePath.toStdString());
        player->play();
        // 开始播放后应该更新播放/暂停键状态
        emit ui->ctrlBar->updatePlayBtnState(true);
    });

    // 连接播放上一个和播放下一个事件
    connect(ui->ctrlBar,&CtrlBar::preClicked,this,[this](){
        ui->listWidget->playPrevious();
    });
    connect(ui->ctrlBar,&CtrlBar::nextClicked,this,[this](){
        ui->listWidget->playNext();
    });

    // 改变播放模式
    connect(ui->ctrlBar,&CtrlBar::changeModel,this,[this](int mode){
        ui->listWidget->setPlayMode((PlaylistWidget::PlayMode)mode);
    });

    // 连接进度条更新
    connect(this->player,&Player::playbackProgress,ui->ctrlBar,&CtrlBar::updateProgress,Qt::QueuedConnection);

    // 连接停止按钮事件
    connect(ui->ctrlBar,&CtrlBar::stopClicked,this,[this]{
        player->stop();
        emit ui->openGLWidget->setFrame(nullptr);
        // 停止播放后应该更新播放/暂停键状态
        emit ui->ctrlBar->updatePlayBtnState(false);
        // 更新进度条信息
        emit this->player->playbackProgress(0.0,0.0);
    });

    // 连接播放/暂停按钮的点击事件
    connect(ui->ctrlBar,&CtrlBar::playClicked,this,[this]{
        onChangePlayState();
    });

    // 连接进度条移动seek
    connect(ui->ctrlBar, &CtrlBar::seekRequested, player, [this](double pos){
        player->seek(pos);
    });

    //连接播放完成信号
    connect(this->player,&Player::playFinish,this,[this]{
        player->stop();
        emit ui->openGLWidget->setFrame(nullptr);
        // 停止播放后应该更新播放/暂停键状态
        emit ui->ctrlBar->updatePlayBtnState(false);
        // 更新进度条信息
        emit this->player->playbackProgress(0.0,0.0);
        // 根据播放模式判断是否继续播放
        auto mode = ui->listWidget->getPlayMode();
        if(mode == PlaylistWidget::PlayMode::Once){
            return;
        }
        ui->listWidget->playNext();
    });
    // 修改音量
    connect(this->ui->ctrlBar,&CtrlBar::volumeChanged,this,[this](float vol){
        this->player->setVolume(vol);
    });

    // 修改播放比例
    connect(this->ui->ctrlBar,&CtrlBar::aspectRatioChanged,this,[this](int val){
        ui->openGLWidget->setAspectRatioMode(val);
    });


}

void MainWindow::keybindInit()
{
    // 空格切换播放/暂停
    new QShortcut(QKeySequence(Qt::Key_Space),this,[=](){
        onChangePlayState();
    });
    // F键切换全屏
    new QShortcut(QKeySequence(Qt::Key_F),this,[=](){
        toggleFullScreen();
    });
    // ESC键退出全屏
    new QShortcut(QKeySequence(Qt::Key_Escape),this,[=](){
        if(isFullScreen())
            toggleFullScreen();
    });
}

void MainWindow::onChangePlayState()
{
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
}

void MainWindow::toggleFullScreen() {
    if (isFullScreen()) {
        showNormal();   // 退出全屏
        // 恢复 UI 控件
        ui->listWidget->show();
        ui->ctrlBar->show();
        hideTimer->stop();
    } else {
        showFullScreen(); // 进入全屏
        // 隐藏 UI 控件，只保留视频
        ui->listWidget->hide();
        ui->ctrlBar->hide();
    }
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(isFullScreen() && event->type() == QEvent::MouseMove){
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        // 显示控制栏和播放列表
        ui->ctrlBar->show();
        ui->listWidget->show();
        // 重置定时器
        // 3秒后隐藏
        hideTimer->start(3000);
    }

    return QMainWindow::eventFilter(obj,event);
}
