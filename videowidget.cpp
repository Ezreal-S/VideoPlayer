#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget{parent},texY(0),texU(0),texV(0)
{
    connect(this,&VideoWidget::setFrame,this,&VideoWidget::slotSetFrame,Qt::QueuedConnection);
}

static const char* vShaderSrc =
    "attribute vec4 vertexIn;      \n"
    "attribute vec2 textureIn;     \n"
    "varying vec2 texCoord;        \n"
    "void main(void) {             \n"
    "   gl_Position = vertexIn;    \n"
    "   texCoord = textureIn;      \n"
    "}";
static const char *fShaderSrc =
    "varying vec2 texCoord;        \n"
    "uniform sampler2D texY;       \n"
    "uniform sampler2D texU;       \n"
    "uniform sampler2D texV;       \n"
    "void main(void) {             \n"
    "   float y = texture2D(texY, texCoord).r; \n"
    "   float u = texture2D(texU, texCoord).r - 0.5; \n"
    "   float v = texture2D(texV, texCoord).r - 0.5; \n"
    "   float r = y + 1.402 * v;   \n"
    "   float g = y - 0.344 * u - 0.714 * v; \n"
    "   float b = y + 1.772 * u;   \n"
    "   gl_FragColor = vec4(r,g,b,1.0);\n"
    "}";

VideoWidget::~VideoWidget()
{
    makeCurrent();
    glDeleteTextures(1,&texY);
    glDeleteTextures(1,&texU);
    glDeleteTextures(1,&texV);
    doneCurrent();
}

void VideoWidget::setAspectRatioMode(int mode)
{
    // 设置新的比例模式
    aspectRatioMode_ = static_cast<AspectRatioMode>(mode);
    // 调用 updateVertices() 根据新比例更新顶点坐标
    updateVertices();
    // 调用 update() 触发重绘
    update();
}

void VideoWidget::slotSetFrame(std::shared_ptr<Yuv420PFrame> frame) {
    if (frame == nullptr) {
        // 实现stop时设置opengl界面为黑色
        width_ = height_ = 0;
        aspectRatio_ = 0.0f;
        update();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mtx_);
        frame_ = frame;
    }

    width_ = frame_->getWidth();
    height_ = frame_->getHeight();

    if (height_ > 0) {
        aspectRatio_ = static_cast<float>(width_) / height_;
    } else {
        aspectRatio_ = 0.0f;
    }

    updateVertices();
    update(); // 触发重绘
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,vShaderSrc);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,fShaderSrc);
    program.link();

    glGenTextures(1,&texY);
    glGenTextures(1,&texU);
    glGenTextures(1,&texV);
}

void VideoWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    updateVertices();
    //glViewport(0,0,w,h);
}

void VideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if(width_ == 0 || height_ == 0)
        return;

    program.bind();

    std::lock_guard<std::mutex> lock(mtx_);
    // 上传y
    glActiveTexture(GL_TEXTURE0);   // 激活纹理单元0
    glBindTexture(GL_TEXTURE_2D,texY);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width_,height_,0,GL_RED,GL_UNSIGNED_BYTE, frame_->yPlane());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 上传u
    glActiveTexture(GL_TEXTURE1);   // 激活纹理单元1
    glBindTexture(GL_TEXTURE_2D,texU);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width_/2,height_/2,0,GL_RED,GL_UNSIGNED_BYTE, frame_->uPlane());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 上传v
    glActiveTexture(GL_TEXTURE2);   // 激活纹理单元2
    glBindTexture(GL_TEXTURE_2D,texV);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width_/2,height_/2,0,GL_RED,GL_UNSIGNED_BYTE, frame_->vPlane());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    program.setUniformValue("texY", 0);
    program.setUniformValue("texU", 1);
    program.setUniformValue("texV", 2);

    // 使用计算好的顶点坐标
    program.enableAttributeArray("vertexIn");
    program.setAttributeArray("vertexIn", GL_FLOAT, vertices_, 2);
    program.enableAttributeArray("textureIn");
    program.setAttributeArray("textureIn", GL_FLOAT, texCoords_, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program.disableAttributeArray("vertexIn");
    program.disableAttributeArray("textureIn");
    program.release();

}

void VideoWidget::updateVertices()
{
    if (width_ == 0 || height_ == 0) {
        return;
    }

    float targetAspect = aspectRatio_;

    switch (aspectRatioMode_) {
    case Stretch:
        // 拉伸填充整个窗口
        vertices_[0] = -1.0f; vertices_[1] = -1.0f; // 左下
        vertices_[2] =  1.0f; vertices_[3] = -1.0f; // 右下
        vertices_[4] = -1.0f; vertices_[5] =  1.0f; // 左上
        vertices_[6] =  1.0f; vertices_[7] =  1.0f; // 右上
        return;

    case Ratio16_9:
        targetAspect = 16.0f / 9.0f;
        break;

    case Ratio4_3:
        targetAspect = 4.0f / 3.0f;
        break;

    case Ratio1_1:
        targetAspect = 1.0f;
        break;

    case OriginalAspect:
    default:
        // 使用原始比例
        break;
    }

    // 计算窗口宽高比
    float widgetAspect = static_cast<float>(width()) / height();

    // 计算保持目标比例的正确显示区域
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;

    if (widgetAspect > targetAspect) {
        // 窗口比视频宽，左右留黑边
        w = targetAspect / widgetAspect;
        x = (1.0f - w) / 2.0f;
    } else {
        // 窗口比视频高，上下留黑边
        h = widgetAspect / targetAspect;
        y = (1.0f - h) / 2.0f;
    }

    // 转换为OpenGL坐标
    vertices_[0] = -1.0f + 2*x; vertices_[1] = -1.0f + 2*y; // 左下
    vertices_[2] =  1.0f - 2*x; vertices_[3] = -1.0f + 2*y; // 右下
    vertices_[4] = -1.0f + 2*x; vertices_[5] =  1.0f - 2*y; // 左上
    vertices_[6] =  1.0f - 2*x; vertices_[7] =  1.0f - 2*y; // 右上
}
