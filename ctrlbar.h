#ifndef CTRLBAR_H
#define CTRLBAR_H

#include <QWidget>

namespace Ui {
class CtrlBar;
}

class CtrlBar : public QWidget
{
    Q_OBJECT

public:
    explicit CtrlBar(QWidget *parent = nullptr);
    ~CtrlBar();

signals:
    void openFileClicked();
    void stopClicked();
    void playClicked();
    void pauseClicked();
    void seekRequested(double pos);
    void updatePlayBtnState(bool paused);
    // 音量改变信号
    void volumeChanged(float vol);
public slots:
    // 更新进度条槽函数
    void updateProgress(double currentTime, double totalTime);
private:
    Ui::CtrlBar *ui;

    bool isDragging_ = false;
};

#endif // CTRLBAR_H
