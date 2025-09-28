#include "videoslider.h"
#include <QDebug>
#include <QStyle>

VideoSlider::VideoSlider(QWidget *parent):
    QSlider(Qt::Horizontal,parent)
{

}

void VideoSlider::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        //获取滑块的宽度
        int handleWidth = style()->pixelMetric(QStyle::PM_SliderLength, nullptr, this);
        // 计算新的值
        int newVal = QStyle::sliderValueFromPosition(minimum(), maximum(),
                                                     event->pos().x() - handleWidth/2,
                                                     width() - handleWidth,
                                                     false);
        setValue(newVal);

        emit sliderMoved(newVal);
        emit sliderReleased();
    }
    QSlider::mousePressEvent(event);
}
