#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Player;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void connectInit();
private:
    Ui::MainWindow *ui;
    Player* player;
};
#endif // MAINWINDOW_H
