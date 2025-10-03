#include "ctrlbar.h"
#include "ui_ctrlbar.h"
#include <QMouseEvent>
#include <QDebug>
#include <string>
#include <sstream>
#include <iomanip>

CtrlBar::CtrlBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CtrlBar)
{
    ui->setupUi(this);
    // 倍速选择器初始化
    ui->speed_cb->addItem("0.5x",0.5);
    ui->speed_cb->addItem("1.0x",1.0);
    ui->speed_cb->addItem("1.25x",1.25);
    ui->speed_cb->addItem("1.5x",1.5);
    ui->speed_cb->addItem("1.75x",1.75);
    ui->speed_cb->addItem("2.0x",2.0);
    ui->speed_cb->setCurrentIndex(1);

    // 初始化信号连接
    connect(this->ui->open_btn,&QPushButton::clicked,this,[this]{
        emit openFileClicked();
    });
    connect(this->ui->stop_btn,&QPushButton::clicked,this,[this]{
        emit stopClicked();
    });
    connect(this->ui->play_btn,&QPushButton::clicked,this,[this]{
        emit playClicked();
    });

    connect(this,&CtrlBar::updatePlayBtnState,this,[this](bool paused){
        // 为true则代表正在播放，按钮标题应为暂停
        if(paused == true)
            this->ui->play_btn->setText(tr("暂停"));
        else
            this->ui->play_btn->setText(tr("播放"));
    });

    connect(this->ui->progress_slid,&QSlider::sliderPressed,this,[this]{
        isDragging_ = true;
    });

    connect(this->ui->progress_slid,&QSlider::sliderReleased,this,[this]{
        isDragging_ = false;
        double pos = ui->progress_slid->value() / static_cast<double>(ui->progress_slid->maximum());
        emit seekRequested(pos); // 发信号告诉 Player 去 seek
    });

}

CtrlBar::~CtrlBar()
{
    delete ui;
}

void CtrlBar::updateProgress(double currentTime, double totalTime)
{
    if(totalTime <= 0.0){
        ui->now_time_lb->setText("00:00:00");
        ui->total_time_lb->setText("00:00:00");
        int value = static_cast<int>((currentTime/totalTime) * ui->progress_slid->maximum());
        ui->progress_slid->setValue(value);
        return;
    }
    static auto formatTime = [](double seconds){
        int totalSeconds = static_cast<int>(seconds + 0.5);
        int h = totalSeconds / 3600;
        int m = (totalSeconds % 3600) / 60;
        int s = totalSeconds % 60;

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << h << ":"
            << std::setfill('0') << std::setw(2) << m << ":"
            << std::setfill('0') << std::setw(2) << s;
        return oss.str();
    };
    if(!isDragging_){
        ui->now_time_lb->setText(QString::fromStdString(formatTime(currentTime)));
        ui->total_time_lb->setText(QString::fromStdString(formatTime(totalTime)));
        int value = static_cast<int>((currentTime/totalTime) * ui->progress_slid->maximum());
        ui->progress_slid->setValue(value);
    }

}


