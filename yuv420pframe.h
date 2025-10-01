#ifndef YUV420PFRAME_H
#define YUV420PFRAME_H

#include <vector>
#include <cstdint>
#include <memory>
#include <QMetaType>

struct AVFrame;

class Yuv420PFrame {
public:
    explicit Yuv420PFrame(AVFrame* frame);
    const uint8_t* yPlane()const;
    const uint8_t* uPlane()const;
    const uint8_t* vPlane()const;
    int getWidth() const;
    int getHeight() const;
private:
    std::vector<uint8_t> yData_;
    std::vector<uint8_t> uData_;
    std::vector<uint8_t> vData_;

    int width_ = 0, height_ = 0;
};

Q_DECLARE_METATYPE(std::shared_ptr<Yuv420PFrame>)

#endif // YUV420PFRAME_H
