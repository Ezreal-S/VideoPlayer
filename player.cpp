#include <SDL2/SDL.h>
#undef main

#include "player.h"
#include <iostream>
#include <QDebug>
#include "yuv420pframe.h"


void PacketQueue::push(AVPacket *pkt)
{
    std::lock_guard<std::mutex> lock(mtx_);
    q_.push(pkt);
    cv_.notify_one();
}

AVPacket *PacketQueue::pop(bool blocking)
{
    std::unique_lock<std::mutex> lock(mtx_);
    if(blocking){
        cv_.wait(lock,[&]{
            return !q_.empty() || this->stop_;
        });
    }
    if(q_.empty()) return nullptr;
    AVPacket* pkt = q_.front();
    q_.pop();
    return pkt;
}


void PacketQueue::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    while(!q_.empty()){
        AVPacket* pkt = q_.front();
        q_.pop();
        av_packet_free(&pkt);
    }
}

void PacketQueue::setStop(bool s)
{
    std::lock_guard<std::mutex> lock(mtx_);
    stop_ = s;
    cv_.notify_all();
}

size_t PacketQueue::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return q_.size();
}

bool PacketQueue::isStopped()const{
    std::lock_guard<std::mutex> lock(mtx_);
    return stop_;
}

Player::Player(VideoWidget *videoWidget)
    :videoWidget_(videoWidget)
{
    avformat_network_init();
    // 初始化SDL
    if(SDL_Init(SDL_INIT_AUDIO) != 0){
        throw std::runtime_error(std::string("SDL_Init失败") + SDL_GetError());
    }

    // 初始化播放器状态为停止
    state_ = MediaState::Stop;
}

Player::~Player()
{
    stop();
    SDL_Quit();
}

bool Player::openFile(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mtx_);
    url_ = url;
    lock.unlock();
    int ret = initFFmpegCtx();
    qDebug()<<"call oepnfile";
    return ret;
}

bool Player::play()
{
    std::unique_lock<std::mutex> lock(mtx_);
    if(running_) return false;
    if(url_ == "")
        return false;
    if(!initCtx_ ){
        lock.unlock();
        // initFFmpegCtx调用stop也会获取锁，所以必须解除，不然会死锁
        if(!initFFmpegCtx())
            return false;
        lock.lock();
    }

    running_ = true;
    paused_ = false;

    audioPktQ_.setStop(false);
    videoPktQ_.setStop(false);

    demuxThread_ = std::thread(&Player::demuxThreadFunc,this);
    audioThread_ = std::thread(&Player::audioThreadFunc,this);
    videoThread_ = std::thread(&Player::videoThreadFunc,this);

    // 开启音频播放
    audioPlayer_->play();
    // 更新播放状态
    state_ = MediaState::Play;
    isEof_ = false;
    qDebug()<<"call play";
    return true;
}

void Player::pause()
{
    std::lock_guard<std::mutex> lock(mtx_);
    // 如果是停止状态，则直接返回不做处理
    if(state_ == MediaState::Stop)
        return;
    // 变换状态：播放->暂停，暂停->播放
    bool p = !paused_.load();
    paused_.store(p);
    state_ = p ? MediaState::Pause: MediaState::Play;
    // 播放/暂停音频设备
    if(audioPlayer_)
        audioPlayer_->pause(paused_);
    qDebug()<<"call pause";
}

void Player::stop()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if(!running_)
        return;

    running_ = false;
    paused_ = false;

    audioPktQ_.setStop(true);
    videoPktQ_.setStop(true);

    // 调用AudioPlayer的停止函数，否则在暂停状态下调用Player::stop会发生死锁
    if(audioPlayer_){
        audioPlayer_->stop();
    }

    if(demuxThread_.joinable())
        demuxThread_.join();
    if(videoThread_.joinable())
        videoThread_.join();
    if(audioThread_.joinable())
        audioThread_.join();

    resetQueues();

    closeAudio();

    closeCodecs();

    if(fmtCtx_ != nullptr){
        avformat_close_input(&fmtCtx_);
        fmtCtx_ = nullptr;
    }


    isEof_ = false;
    audioClock_ = 0.0;
    initCtx_.store(false);
    // 更新播放状态
    state_ = MediaState::Stop;
    qDebug()<<"call stop";
}

void Player::seek(double pos) {
    if (!fmtCtx_ || videoStreamIndex_ < 0 || audioStreamIndex_ < 0)
        return;

    stop();
    initFFmpegCtx();
    // 获取总时长（微秒）
    int64_t duration = fmtCtx_->duration;
    if (duration <= 0)
        return;

    // 计算目标位置（秒）
    double sec = (pos * static_cast<double>(duration))/double(1000000);
    int64_t m_nSeekingPos = (int64_t)sec / av_q2d(fmtCtx_->streams[videoStreamIndex_]->time_base);


    // 执行跳转（使用视频流作为参考）
    if (av_seek_frame(fmtCtx_,videoStreamIndex_ , m_nSeekingPos, AVSEEK_FLAG_BACKWARD) < 0) {
        qDebug() << "Seek failed";
        return;
    }

    // 重置时钟标志，解复用线程在跳转后读到第一个音频包的情况下，会计算pts传到音频时钟
    seekChangeClock_ = true;
    play();

    qDebug() << "Seek to:" << pos << "(" << sec << "s)";
}

// void Player::seek(double pos) {
//     std::unique_lock<std::mutex> lock(mtx_);

//     if (!fmtCtx_ || !running_)
//         return;

//     // 1. 暂停播放但不释放资源
//     lock.unlock();
//     pause();
//     lock.lock();
//     audioPlayer_->setSeeking(true);
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     // 2. 清空数据包队列
//     audioPktQ_.clear();
//     videoPktQ_.clear();
//     audioPlayer_->clearBuf();

//     // 3. 刷新解码器
//     if (audioCtx_) avcodec_flush_buffers(audioCtx_);
//     if (videoCtx_) avcodec_flush_buffers(videoCtx_);

//     // 4. 计算跳转位置
//     int64_t target_us = static_cast<int64_t>(pos * fmtCtx_->duration);

//     // 5. 执行跳转（关键修改）
//     int ret = av_seek_frame(fmtCtx_, -1, target_us, AVSEEK_FLAG_BACKWARD);
//     if (ret < 0) {
//         qDebug() << "Seek failed:" ;//<< av_err2str(ret);
//         return;
//     }

//     // 6. 重置时钟
//     audioClock_ = static_cast<double>(target_us) / AV_TIME_BASE;
//     if(audioPlayer_) {
//         audioPlayer_->setAudioClock(audioClock_);
//     }

//     // 7. 重置状态
//     isEof_ = false;
//     audioPlayer_->setSeeking(false);
//     // 8. 恢复播放
//     lock.unlock();
//     pause();
//     lock.lock();
//     qDebug() << "Seek to:" << pos;
// }

MediaState Player::getState() const
{
    return state_;
}

bool Player::initFFmpegCtx()
{
    stop();
    int ret = avformat_open_input(&fmtCtx_,url_.c_str(),nullptr,nullptr);
    if(ret < 0){
        std::cerr<<"打开文件失败"<<std::endl;
        return false;
    }
    ret = avformat_find_stream_info(fmtCtx_,nullptr);
    if(ret < 0){
        std::cerr<<"读取流信息失败"<<std::endl;
        return false;
    }

    audioStreamIndex_ = -1;
    videoStreamIndex_ = -1;

    for(unsigned i = 0; i < fmtCtx_->nb_streams; ++i){
        if(fmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audioStreamIndex_ = i;
        if(fmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            videoStreamIndex_ = i;
    }
    if(audioStreamIndex_ < 0){
        std::cerr<<"未找到音频流"<<std::endl;
        return false;
    }
    if(videoStreamIndex_ < 0){
        std::cerr<<"未找到视频流"<<std::endl;
        return false;
    }
    audioStream_ = fmtCtx_->streams[audioStreamIndex_];
    videoStream_ = fmtCtx_->streams[videoStreamIndex_];
    // 视频总时长
    duration_ = fmtCtx_->duration;

    openCodecs();
    openAudio();
    initCtx_.store(true);
    qDebug()<<"call initFFmpegCtx";
    return true;
}

void Player::demuxThreadFunc()
{
    while(running_){

        if(paused_){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 如果已经读取到结尾了，休眠等待
        if(isEof_){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if(audioPktQ_.size() >= maxAudioPkts_ /*|| videoPktQ_.size() >= maxVideoPkts_*/){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        AVPacket* pkt = av_packet_alloc();
        if(!pkt){
            continue;
        }
        int ret = av_read_frame(fmtCtx_,pkt);
        if(ret < 0){
            if(ret == AVERROR_EOF && !isEof_){
                qDebug()<<"yes";
                isEof_ = true;
                audioPktQ_.setStop(true);
                videoPktQ_.setStop(true);
                av_packet_free(&pkt);
                break;
            }
            av_packet_free(&pkt);
            continue;
        }

        if (pkt->stream_index == audioStreamIndex_) {
            if(seekChangeClock_){
                // 计算PTS
                double pts = pkt->pts * av_q2d(audioStream_->time_base);
                audioPlayer_->setAudioClock(pts);

                seekChangeClock_ = false;
            }
            audioPktQ_.push(pkt);
        }
        else if(pkt->stream_index == videoStreamIndex_){
            videoPktQ_.push(pkt);
        }else{
            av_packet_free(&pkt);
        }


    }
}

void Player::audioThreadFunc()
{
    AVFrame* frame = av_frame_alloc();
    int bytesPerSample = av_get_bytes_per_sample(outFmt_) * outChannels_;
    audioPlayer_->setEof(false);
    while(running_){
        while(paused_ && running_){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if(!running_){
            break;
        }

        AVPacket* pkt = audioPktQ_.pop(true);
        if(!pkt){
            if(audioPktQ_.isStopped()){
                avcodec_send_packet(audioCtx_,nullptr);
                while(avcodec_receive_frame(audioCtx_,frame) ==0){
                    /*重采样，计算大小*/
                    int dst_nb_samples = av_rescale_rnd(
                        swr_get_delay(swrCtx_, frame->sample_rate) + frame->nb_samples,
                        outRate_, frame->sample_rate, AV_ROUND_UP);

                    int buf_size = av_samples_get_buffer_size(
                        nullptr, outChannels_, dst_nb_samples, outFmt_, 1);

                    uint8_t* audio_buf = (uint8_t*)av_malloc(buf_size);
                    int audio_buf_size = swr_convert(swrCtx_, &audio_buf, dst_nb_samples,
                                                     (const uint8_t**)frame->data, frame->nb_samples) * bytesPerSample;

                    audioPlayer_->enqueue(audio_buf,audio_buf_size);
                    av_free(audio_buf);
                }
                break;
            }

            continue;
        }

        int ret = avcodec_send_packet(audioCtx_,pkt);
        av_packet_free(&pkt);
        if(ret < 0)
            continue;

        while(ret >= 0 && running_){
            ret = avcodec_receive_frame(audioCtx_,frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                av_frame_unref(frame);
                break;
            }

            if(ret < 0){
                av_frame_unref(frame);
                break;
            }

            {
                /*重采样，计算大小*/
                int dst_nb_samples = av_rescale_rnd(
                    swr_get_delay(swrCtx_, frame->sample_rate) + frame->nb_samples,
                    outRate_, frame->sample_rate, AV_ROUND_UP);

                int buf_size = av_samples_get_buffer_size(
                    nullptr, outChannels_, dst_nb_samples, outFmt_, 1);

                uint8_t* audio_buf = (uint8_t*)av_malloc(buf_size);
                int audio_buf_size = swr_convert(swrCtx_, &audio_buf, dst_nb_samples,
                                             (const uint8_t**)frame->data, frame->nb_samples) * bytesPerSample;
                audioPlayer_->enqueue(audio_buf,audio_buf_size);
                av_free(audio_buf);
            }
        }
    }
    audioPlayer_->setEof(true);
    av_frame_free(&frame);
}

void Player::videoThreadFunc()
{
    AVFrame* frame = av_frame_alloc();
    AVRational vtb = fmtCtx_->streams[videoStreamIndex_]->time_base;
    double totalTime = fmtCtx_->streams[videoStreamIndex_]->duration * av_q2d(vtb);
    while(running_){
        while(paused_ && running_){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if(!running_){
            break;
        }


        AVPacket* pkt = videoPktQ_.pop(true);
        if(!pkt){
            if(videoPktQ_.isStopped()  && isEof_){
                // flush 解码器
                avcodec_send_packet(videoCtx_, nullptr);
                while (avcodec_receive_frame(videoCtx_, frame) == 0) {

                    double pts = 0.0;
                    if(frame->best_effort_timestamp != AV_NOPTS_VALUE)
                        pts = frame->best_effort_timestamp * av_q2d(vtb);
                    else if(frame->pts != AV_NOPTS_VALUE)
                        pts = frame->pts * av_q2d(vtb);

                    double diff = pts - audioPlayer_->getAudioClock();

                    if(diff > 0)
                        std::this_thread::sleep_for(std::chrono::duration<double>(diff));
                    else if(diff < -0.1){
                        continue;
                        // 丢帧
                    }

                    if(frame->format == AV_PIX_FMT_YUV420P){
                        // 发送进度信号
                        emit playbackProgress(pts,totalTime);
                        // 通知ui渲染
                        std::shared_ptr<Yuv420PFrame> yuvFrame(std::make_shared<Yuv420PFrame>(frame));
                        emit videoWidget_->setFrame(yuvFrame);
                        av_frame_unref(frame);
                    }
                }
                emit playFinish();
                break;
            }
            continue;
        }

        int ret = avcodec_send_packet(videoCtx_,pkt);
        av_packet_free(&pkt);
        if(ret < 0)
            continue;
        while(ret >=0 && running_){
            ret = avcodec_receive_frame(videoCtx_,frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if(ret < 0) break;

            double pts = 0.0;
            if(frame->best_effort_timestamp != AV_NOPTS_VALUE)
                pts = frame->best_effort_timestamp * av_q2d(vtb);
            else if(frame->pts != AV_NOPTS_VALUE)
                pts = frame->pts * av_q2d(vtb);

            double diff = pts - audioPlayer_->getAudioClock();

            if(diff > 0)
                std::this_thread::sleep_for(std::chrono::duration<double>(diff));
            else if(diff < -0.1){
                continue;
                // 丢帧
            }

            if(frame->format == AV_PIX_FMT_YUV420P){
                // 发送进度信号
                emit playbackProgress(pts,totalTime);
                // 通知ui渲染
                std::shared_ptr<Yuv420PFrame> yuvFrame(std::make_shared<Yuv420PFrame>(frame));
                emit videoWidget_->setFrame(yuvFrame);
                av_frame_unref(frame);
            }
        }
    }
    if(isEof_){
        emit playFinish();
    }
    qDebug()<<"video queit";
    av_frame_free(&frame);
}



void Player::openCodecs()
{
    AVCodec* ac = avcodec_find_decoder(fmtCtx_->streams[audioStreamIndex_]->codecpar->codec_id);
    audioCtx_ = avcodec_alloc_context3(ac);
    avcodec_parameters_to_context(audioCtx_,fmtCtx_->streams[audioStreamIndex_]->codecpar);
    avcodec_open2(audioCtx_,ac,nullptr);

    swrCtx_ = swr_alloc_set_opts(nullptr,av_get_default_channel_layout(outChannels_),
                                outFmt_,outRate_,
                                av_get_default_channel_layout(audioCtx_->channels),
                                audioCtx_->sample_fmt,audioCtx_->sample_rate,
                                0,nullptr);
    swr_init(swrCtx_);
    AVCodec* vc = avcodec_find_decoder(fmtCtx_->streams[videoStreamIndex_]->codecpar->codec_id);
    videoCtx_ = avcodec_alloc_context3(vc);
    avcodec_parameters_to_context(videoCtx_,fmtCtx_->streams[videoStreamIndex_]->codecpar);
    avcodec_open2(videoCtx_,vc,nullptr);
    std::cout << "Audio stream codec ID: " << fmtCtx_->streams[audioStreamIndex_]->codecpar->codec_id << std::endl;
    std::cout << "Audio sample rate: " << fmtCtx_->streams[audioStreamIndex_]->codecpar->sample_rate << std::endl;
    std::cout << "Audio channels: " << fmtCtx_->streams[audioStreamIndex_]->codecpar->channels << std::endl;

}

void Player::closeCodecs()
{
    if(swrCtx_){
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }
    if(audioCtx_){
        avcodec_free_context(&audioCtx_);
        audioCtx_ = nullptr;
    }
    if(videoCtx_){
        avcodec_free_context(&videoCtx_);
        videoCtx_ = nullptr;
    }
}

void Player::openAudio()
{
    if (audioStreamIndex_ < 0) return;

    // 如果之前已经打开了音频设备，先关闭它
    closeAudio();
    audioPlayer_ = std::make_unique<AudioPlayer>(44100,2);
}

void Player::closeAudio()
{
    if (audioPlayer_ != nullptr) {
        audioPlayer_->stop();
        audioPlayer_.reset();
    }
}

void Player::resetQueues()
{
    audioPktQ_.clear();
    videoPktQ_.clear();
}

void Player::flushDecoders()
{
    if(audioCtx_) avcodec_flush_buffers(audioCtx_);
    if(videoCtx_) avcodec_flush_buffers(videoCtx_);
}
