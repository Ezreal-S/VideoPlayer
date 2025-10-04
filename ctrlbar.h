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
    // seek请求信号
    void seekRequested(double pos);
    // 更新播放器按钮状态信号
    void updatePlayBtnState(bool paused);
    // 音量改变信号
    void volumeChanged(float vol);
    // 修改视频显示比例信号
    void aspectRatioChanged(int value);
public slots:
    // 更新进度条槽函数
    void updateProgress(double currentTime, double totalTime);
private:
    Ui::CtrlBar *ui;

    bool isDragging_ = false;
};

#endif // CTRLBAR_H
