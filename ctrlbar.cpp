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


}

CtrlBar::~CtrlBar()
{
    delete ui;
}


