#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
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
    void keybindInit();

private slots:
    void onChangePlayState();
private:
    Ui::MainWindow *ui;
    Player* player;
    QTimer* hideTimer;

    // 全屏切换
    void toggleFullScreen();


    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event);
};
#endif // MAINWINDOW_H
