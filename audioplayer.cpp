#include <SDL2/SDL.h>
#undef main
#include "audioplayer.h"
#include <QDebug>
#include <cstring>


void AudioRingBuffer::push(const uint8_t *data, size_t len)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cvNotFull_.wait(lock,[&]{
        return stop_ || buffer_.size() + len <= capacity_;
    });
    if(stop_)
        return;

    buffer_.insert(buffer_.end(),data,data+len);
    cvNotEmpty_.notify_one();
}

size_t AudioRingBuffer::pop(uint8_t *dst, size_t len)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cvNotEmpty_.wait(lock,[&]{
        return stop_ || !buffer_.empty();
    });
    if(stop_ && buffer_.empty())
        return 0;
    size_t toCopy = std::min(len,buffer_.size());
    std::copy_n(buffer_.begin(),toCopy,dst);
    buffer_.erase(buffer_.begin(),buffer_.begin()+toCopy);
    cvNotFull_.notify_one();
    return toCopy;
}

void AudioRingBuffer::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cvNotEmpty_.notify_all();
    cvNotFull_.notify_all();
}

size_t AudioRingBuffer::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size();
}

void AudioRingBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
}

bool AudioRingBuffer::isStopped() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stop_;
}


AudioPlayer::AudioPlayer(int sampleRate, int channels,AVSampleFormat fmt):
    buffer_(1<<20),
    outRate_(sampleRate),outChannels_(channels),bytesPerSample_(av_get_bytes_per_sample(fmt))
{

    SDL_AudioSpec spec{};
    spec.freq = outRate_;
    spec.format = AUDIO_S16SYS;
    spec.channels = outChannels_;
    spec.samples = 1024;
    spec.callback = &AudioPlayer::audioCallbackWrapper;
    spec.userdata = this;
    if(SDL_OpenAudio(&spec,nullptr) < 0)
        throw std::runtime_error("Failed to open SDL audio");
}

AudioPlayer::~AudioPlayer()
{
    buffer_.stop();
    SDL_CloseAudio();
}

void AudioPlayer::enqueue(const uint8_t *data, size_t len)
{
    buffer_.push(data,len);
}



void AudioPlayer::play()
{
    SDL_PauseAudio(0);
}

void AudioPlayer::pause(bool paused)
{
    SDL_PauseAudio(paused?1:0);
}

void AudioPlayer::stop()
{
    // 先暂停音频设备
    SDL_PauseAudio(1);

    // 停止缓冲区
    buffer_.stop();

    // 清空缓冲区
    buffer_.clear();

    audioClock_ = 0.0;
}

void AudioPlayer::clearBuf()
{
    buffer_.clear();
}



void AudioPlayer::setAudioClock(double v)
{
    audioClock_ = v;
}

double AudioPlayer::getAudioClock() const
{
    return audioClock_;
}

void AudioPlayer::setSpeed_(float speed)
{
    speed_ = speed;
}

void AudioPlayer::setVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
}

float AudioPlayer::getVolume() const {
    return volume_;
}


void AudioPlayer::audioCallbackWrapper(void *userdata, uint8_t *stream, int len)
{
    auto* self = static_cast<AudioPlayer*>(userdata);
    self->audioCallback(stream,len);
}

// void AudioPlayer::audioCallback(uint8_t *stream, int len)
// {
//     if (buffer_.size() == 0) {
//         SDL_memset(stream, 0, len);
//         return;
//     }
//     size_t copied = buffer_.pop(stream,len);
//     if(copied < static_cast<size_t>(len)){
//         SDL_memset(stream + copied,0,len-copied);
//     }
//     audioClock_ = audioClock_ + double(copied)/(outChannels_*bytesPerSample_*outRate_);
// }

void AudioPlayer::audioCallback(uint8_t *stream, int len) {
    SDL_memset(stream, 0, len); // 清空输出缓冲区

    if (buffer_.size() == 0) {
        return;
    }

    // 使用临时缓冲区
    std::vector<uint8_t> temp(len);
    size_t copied = buffer_.pop(temp.data(), len);

    if (copied < static_cast<size_t>(len)) {
        SDL_memset(temp.data() + copied, 0, len - copied);
    }

    // 应用音量控制 - 核心代码
    int sdlVolume = static_cast<int>(volume_ * SDL_MIX_MAXVOLUME);
    SDL_MixAudio(stream, temp.data(), len, sdlVolume);

    // 更新音频时钟
    audioClock_ = audioClock_ + double(copied) / (outChannels_ * bytesPerSample_ * outRate_);
}


// void AudioPlayer::audioCallback(uint8_t *stream, int len) {
//     SDL_memset(stream, 0, len);

//     if (buffer_.size() == 0) {
//         return;
//     }

//     // 从环形缓冲取数据
//     std::vector<uint8_t> temp(len);
//     size_t copied = buffer_.pop(temp.data(), len);

//     // 输入样本数（每声道）
//     int inputSamplesPerChannel = (copied / sizeof(short)) / outChannels_;

//     // 估算输出最大样本数（考虑变速）
//     int maxOutputSamplesPerChannel = static_cast<int>(inputSamplesPerChannel / speed_) + 1024;

//     std::vector<short> outputBuffer(maxOutputSamplesPerChannel * outChannels_);

//     // Sonic 处理
//     int samplesGeneratedPerChannel = sp_.process(
//         reinterpret_cast<short*>(temp.data()),
//         inputSamplesPerChannel,
//         outputBuffer.data(),
//         maxOutputSamplesPerChannel
//         );

//     int bytesGenerated = samplesGeneratedPerChannel * outChannels_ * sizeof(short);

//     int sdlVolume = static_cast<int>(volume_ * SDL_MIX_MAXVOLUME);

//     if (bytesGenerated > 0) {
//         int bytesToCopy = std::min(len, bytesGenerated);
//         SDL_MixAudio(stream,
//                      reinterpret_cast<uint8_t*>(outputBuffer.data()),
//                      bytesToCopy,
//                      sdlVolume);

//         if (bytesToCopy < len) {
//             SDL_memset(stream + bytesToCopy, 0, len - bytesToCopy);
//         }
//     } else {
//         SDL_MixAudio(stream, temp.data(),
//                      std::min(len, static_cast<int>(copied)),
//                      sdlVolume);
//     }

//     // 更新时钟：基于原始输入数据量
//     double secondsConsumed = static_cast<double>(copied) /
//                              (outChannels_ * bytesPerSample_ * outRate_);
//     audioClock_ = audioClock_ + secondsConsumed;
// }
