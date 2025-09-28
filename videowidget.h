#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

class VideoWidget : public QOpenGLWidget ,protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void setFrame(uchar *yPlane, uchar *uPlane, uchar *vPlane,
                               int yPitch, int uPitch, int vPitch,
                               int width, int height);
signals:

    // QOpenGLWidget interface
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
private:
    QOpenGLShaderProgram program;
    GLuint texY, texU,texV;
    int videoW,videoH;
    QByteArray yData,uData,vData;
    int yStride,uStride,vStride;
};

#endif // VIDEOWIDGET_H
