#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget{parent},texY(0),texU(0),texV(0),
    videoW(0),videoH(0)
{}

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

void VideoWidget::setFrame(uchar *yPlane, uchar *uPlane, uchar *vPlane,
                           int yPitch, int uPitch, int vPitch,
                           int width, int height) {
    videoW = width;
    videoH = height;
    yStride = yPitch;
    uStride = uPitch;
    vStride = vPitch;

    yData = QByteArray((char*)yPlane, yPitch * height);
    uData = QByteArray((char*)uPlane, uPitch * height/2);
    vData = QByteArray((char*)vPlane, vPitch * height/2);

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
    glViewport(0,0,w,h);
}

void VideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if(videoW == 0 || videoH == 0)
        return;

    program.bind();
    // 上传y
    glActiveTexture(GL_TEXTURE0);   // 激活纹理单元0
    glBindTexture(GL_TEXTURE_2D,texY);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,yStride,videoH,0,GL_RED,GL_UNSIGNED_BYTE, yData.constData());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 上传u
    glActiveTexture(GL_TEXTURE1);   // 激活纹理单元1
    glBindTexture(GL_TEXTURE_2D,texU);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,uStride,videoH/2,0,GL_RED,GL_UNSIGNED_BYTE, uData.constData());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 上传v
    glActiveTexture(GL_TEXTURE2);   // 激活纹理单元2
    glBindTexture(GL_TEXTURE_2D,texV);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,vStride,videoH/2,0,GL_RED,GL_UNSIGNED_BYTE, vData.constData());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    program.setUniformValue("texY", 0);
    program.setUniformValue("texU", 1);
    program.setUniformValue("texV", 2);

    // 绘制一个全屏矩形
    GLfloat vertices[] = {
        -1.0f,-1.0f,  1.0f,-1.0f,  -1.0f,1.0f,  1.0f,1.0f
    };
    GLfloat texCoords[] = {
        0.0f,1.0f,  1.0f,1.0f,  0.0f,0.0f,  1.0f,0.0f
    };

    program.enableAttributeArray("vertexIn");
    program.setAttributeArray("vertexIn", GL_FLOAT, vertices, 2);
    program.enableAttributeArray("textureIn");
    program.setAttributeArray("textureIn", GL_FLOAT, texCoords, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program.disableAttributeArray("vertexIn");
    program.disableAttributeArray("textureIn");
    program.release();

}
