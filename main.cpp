#include "mainwindow.h"
#include <QApplication>
#include "yuv420pframe.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 注册 shared_ptr<Yuv420PFrame>
    qRegisterMetaType<std::shared_ptr<Yuv420PFrame>>("std::shared_ptr<Yuv420PFrame>");
    MainWindow w;
    w.show();
    return a.exec();
}
