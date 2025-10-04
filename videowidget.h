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
    enum AspectRatioMode {
        OriginalAspect,    // 保持原始比例
        Stretch,            // 拉伸填充
        Ratio16_9,          // 16:9
        Ratio4_3,           // 4:3
        Ratio1_1            // 1:1
    };
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    // 设置新的比例模式
    void setAspectRatioMode(int mode);
    AspectRatioMode aspectRatioMode() const { return aspectRatioMode_; }

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
    // 更新顶点坐标
    void updateVertices();

    QOpenGLShaderProgram program;
    GLuint texY, texU,texV;
    int width_ = 0,height_ = 0;
    std::shared_ptr<Yuv420PFrame> frame_;
    std::mutex mtx_;

    float aspectRatio_ = 0.0f;  // 存储视频的原始宽高比
    AspectRatioMode aspectRatioMode_ = OriginalAspect;  //存储当前选择的显示比例模式
    GLfloat vertices_[8]; // 顶点坐标
    GLfloat texCoords_[8] = {   // 纹理坐标
        0.0f, 1.0f,   // 左下
        1.0f, 1.0f,   // 右下
        0.0f, 0.0f,   // 左上
        1.0f, 0.0f    // 右上
    };
};

#endif // VIDEOWIDGET_H
