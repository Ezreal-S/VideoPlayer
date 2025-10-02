#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H


#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstdint>

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}

class AudioRingBuffer{
public:
    AudioRingBuffer(size_t capacity = 1 << 20) // 默认1MB
        : capacity_(capacity),stop_(false){}
    void push(const uint8_t* data, size_t len);
    size_t pop(uint8_t* dst,size_t len);
    void stop();
    size_t size()const;
    void clear();
    bool isStopped() const;

private:
    size_t capacity_;
    std::deque<uint8_t> buffer_;
    mutable std::mutex mutex_;
    std::condition_variable cvNotEmpty_;
    std::condition_variable cvNotFull_;
    bool stop_ = false;



};


class AudioPlayer
{
public:
    AudioPlayer(int sampleRate = 44100,int channels = 2,AVSampleFormat fmt = AV_SAMPLE_FMT_S16);
    ~AudioPlayer();
    void enqueue(const uint8_t* data, size_t len);
    void play();
    void pause(bool paused);
    void stop();
    double getAudioClock() const;


    void setEof(bool b) {
        eof_ = b;
    }

    bool isEof() const {
        return eof_;
    }
private:
    AudioRingBuffer buffer_;
    int outRate_;
    int outChannels_;
    int bytesPerSample_;
    double audioClock_;
    static void audioCallbackWrapper(void* userdata, uint8_t* stream, int len);
    bool eof_ = false;
    void audioCallback(uint8_t* stream, int len);
};

#endif // AUDIOPLAYER_H
