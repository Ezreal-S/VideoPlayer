#include <SDL2/SDL.h>
#undef main
#include "audioplayer.h"
#include <QDebug>


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
    buffer_(1<<20),outRate_(sampleRate),outChannels_(channels),bytesPerSample_(av_get_bytes_per_sample(fmt))
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


void AudioPlayer::audioCallbackWrapper(void *userdata, uint8_t *stream, int len)
{
    auto* self = static_cast<AudioPlayer*>(userdata);
    self->audioCallback(stream,len);
}

void AudioPlayer::audioCallback(uint8_t *stream, int len)
{
    if (buffer_.size() == 0) {
        SDL_memset(stream, 0, len);
        return;
    }
    size_t copied = buffer_.pop(stream,len);
    if(copied < static_cast<size_t>(len)){
        SDL_memset(stream + copied,0,len-copied);
    }
    audioClock_ = audioClock_ + double(copied)/(outChannels_*bytesPerSample_*outRate_);
}

