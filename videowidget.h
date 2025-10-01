#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <memory>
#include <mutex>
#include "yuv420pframe.h"


class VideoWidget : public QOpenGLWidget ,protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    // void setFrame(uchar *yPlane, uchar *uPlane, uchar *vPlane,
    //                            int yPitch, int uPitch, int vPitch,
    //                            int width, int height);
signals:
    void setFrame(std::shared_ptr<Yuv420PFrame> frame);
private slots:
void slotSetFrame(std::shared_ptr<Yuv420PFrame> frame);
    // QOpenGLWidget interface
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
private:
    QOpenGLShaderProgram program;
    GLuint texY, texU,texV;
    int width_ = 0,height_ = 0;
    std::shared_ptr<Yuv420PFrame> frame_;
    std::mutex mtx_;

};

#endif // VIDEOWIDGET_H
