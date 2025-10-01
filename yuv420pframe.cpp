#include "yuv420pframe.h"
extern "C" {
#include <libavutil/frame.h>
}
Yuv420PFrame::Yuv420PFrame(AVFrame *frame)
{
    if(!frame)
        return;
    width_ = frame->width;
    height_ = frame->height;

    yData_.resize(width_ * height_);
    uData_.resize(width_ * height_ / 4);
    vData_.resize(width_ * height_ / 4);

    // copy Y
    if(frame->linesize[0] == width_){
        memcpy(yData_.data(),frame->data[0],width_*height_);
    }else{
        for(int i = 0; i < height_; ++i){
            memcpy(yData_.data() + i * width_,frame->data[0] + i *frame->linesize[0],width_);
        }
    }
    // copy U
    if(frame->linesize[1] == width_ / 2){
        memcpy(uData_.data(),frame->data[1],width_*height_ / 4);
    }else{
        for(int i = 0; i < height_/2; ++i){
            memcpy(uData_.data() + i * (width_/2),frame->data[1] + i *frame->linesize[1],width_/2);
        }
    }
    // copy V
    if(frame->linesize[2] == width_ / 2){
        memcpy(vData_.data(),frame->data[2],width_*height_ / 4);
    }else{
        for(int i = 0; i < height_/2; ++i){
            memcpy(vData_.data() + i * (width_/2),frame->data[2] + i *frame->linesize[2],width_/2);
        }
    }
}

const uint8_t *Yuv420PFrame::yPlane() const
{
    return yData_.data();
}

const uint8_t *Yuv420PFrame::uPlane() const
{
    return uData_.data();
}

const uint8_t *Yuv420PFrame::vPlane() const
{
    return vData_.data();
}

int Yuv420PFrame::getWidth() const
{
    return width_;
}

int Yuv420PFrame::getHeight() const
{
    return height_;
}
