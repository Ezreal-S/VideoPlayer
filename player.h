#ifndef PLAYER_H
#define PLAYER_H



#include "videowidget.h"
#include "audioplayer.h"


extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>

}

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <memory>
#include <QObject>

enum class MediaState{
    Play,
    Pause,
    Stop
};

class PacketQueue{
public:
    void push(AVPacket* pkt);
    AVPacket* pop(bool blocking = true);
    void clear();
    void setStop(bool s);
    size_t size()const;
    bool isStopped()const;
private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<AVPacket*> q_;
    bool stop_{false};
};

class Player : public QObject
{
Q_OBJECT
public:
    Player(VideoWidget* videoWidget);
    ~Player();
    // 打开文件
    bool openFile(const std::string& url);
    // 开始播放
    bool play();
    // 暂停/继续
    void pause();
    // 停止
    void stop();
    // 跳转
    void seek(double pos);

    // 设置音量
    void setVolume(float volume);
    // 获取音量
    float getVolume() const;


    MediaState getState()const;
private:
    bool initFFmpegCtx();
    // 解复用线程
    void demuxThreadFunc();
    // 音频解码线程
    void audioThreadFunc();
    // 视频解码线程
    void videoThreadFunc();
    static void sdlAudioCallback(void* userdata,uint8_t* stream,int len);

    void openCodecs();
    void closeCodecs();
    void openAudio();
    void closeAudio();
    void resetQueues();
    void flushDecoders();

private:
    std::string url_;
    VideoWidget* videoWidget_ = nullptr;
    AVFormatContext* fmtCtx_ = nullptr;
    AVCodecContext* audioCtx_ = nullptr;
    AVCodecContext* videoCtx_ = nullptr;
    int audioStreamIndex_ = -1;
    int videoStreamIndex_ = -1;
    AVStream* audioStream_ = nullptr;
    AVStream* videoStream_ = nullptr;


    // 视频总时长
    double duration_ = 0.0;

    mutable std::mutex mtx_;

    std::thread demuxThread_;
    std::thread audioThread_;
    std::thread videoThread_;

    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> initCtx_{false};

    PacketQueue audioPktQ_;
    PacketQueue videoPktQ_;
    std::unique_ptr<AudioPlayer> audioPlayer_;

    std::atomic<double> audioClock_{0.0};

    SwrContext* swrCtx_ = nullptr;
    AVSampleFormat outFmt_ = AV_SAMPLE_FMT_S16;
    int outRate_ = 44100;
    int outChannels_ = 2;

    const size_t maxAudioPkts_ = 30;
    const size_t maxVideoPkts_ = 30;

    MediaState state_;

    // 是否读取到末尾了
    std::atomic<bool> isEof_ = false;

    bool seekChangeClock_ = false;
    // 音量大小
    float volume_ = 1.f;
signals:
    void playbackProgress(double currentTime, double totalTime);
    void playFinish();
};

#endif // PLAYER_H
