#include "ctrlbar.h"
#include "ui_ctrlbar.h"
#include <QMouseEvent>

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
        emit playerClicked();
    });
}

CtrlBar::~CtrlBar()
{
    delete ui;
}


