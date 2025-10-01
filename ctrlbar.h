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

    void updatePlayBtnState(bool paused);
private:
    Ui::CtrlBar *ui;

};

#endif // CTRLBAR_H
