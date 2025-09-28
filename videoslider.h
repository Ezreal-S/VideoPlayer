#ifndef VIDEOSLIDER_H
#define VIDEOSLIDER_H

#include <QSlider>
#include <QMouseEvent>

class VideoSlider : public QSlider
{
    Q_OBJECT
public:
    explicit VideoSlider(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event)override;

};

#endif // VIDEOSLIDER_H
