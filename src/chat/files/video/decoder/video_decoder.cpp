#include "video_decoder.h"

#include <QImage>
#include <QDebug>

#include "../../audio/decoder/audio_decoder.h"

VideoDecoder::VideoDecoder(const QByteArray &video_data, QObject *parent): QThread(parent), video_data_(video_data),
                                                                           paused_(true) {
    buffer_.setData(video_data_);
    buffer_.open(QIODevice::ReadOnly);
    io_context_ = avio_alloc_context(
        static_cast<unsigned char *>(av_malloc(8192)), 8192, 0, &buffer_,
        &VideoDecoder::ReadPacket, nullptr, &VideoDecoder::SeekPacket
    );
}

VideoDecoder::~VideoDecoder() {
    Stop();
    wait();
    CleanupFFmpegResources();
    if (io_context_) {
        av_freep(&io_context_->buffer);
        avio_context_free(&io_context_);
        io_context_ = nullptr;
    }
}

bool VideoDecoder::Initialize() {
    format_context_ = avformat_alloc_context();
    if (!format_context_) return false;
    format_context_->pb = io_context_;
    format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_open_input(&format_context_, nullptr, nullptr, nullptr) != 0) {
        emit error("[VIDEO DECODER] Video stream cannot be opened (avformat_open_input).");
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit error("[VIDEO DECODER] Unable to find stream information (avformat_find_stream_info).");
        return false;
    }

    video_stream_ = -1;
    audio_stream_index_ = -1;
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (video_stream_ == -1 && format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_ = i;
        } else if (audio_stream_index_ == -1 && format_context_->streams[i]->codecpar->codec_type ==
                   AVMEDIA_TYPE_AUDIO) {
            audio_stream_index_ = i;
        }
    }

    if (video_stream_ == -1) {
        emit error("[VIDEO DECODER] No video stream found.");
        return false;
    }

    const AVCodecParameters *codec_params = format_context_->streams[video_stream_]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        emit error("[VIDEO DECODER] Video codec not found.");
        return false;
    }

    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) return false;
    if (avcodec_parameters_to_context(codec_context_, codec_params) < 0) return false;
    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        emit error("[VIDEO DECODER] Video codec cannot be opened.");
        return false;
    }

    frame_ = av_frame_alloc();
    if (!frame_) return false;

    sws_context_ = sws_getContext(
        codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
        codec_context_->width, codec_context_->height, AV_PIX_FMT_RGB24,
        SWS_LANCZOS, nullptr, nullptr, nullptr
    );
    if (!sws_context_) {
        emit error("[VIDEO DECODER] Unable to create video conversion context (sws_getContext).");
        return false;
    }

    const AVRational frame_rate_rational = av_guess_frame_rate(format_context_, format_context_->streams[video_stream_],
                                                               nullptr);
    frame_rate_ = frame_rate_rational.num > 0 && frame_rate_rational.den > 0 ? av_q2d(frame_rate_rational) : 30.0;
    duration_ = format_context_->duration > 0 ? static_cast<double>(format_context_->duration) / AV_TIME_BASE : 0.0;

    if (audio_stream_index_ != -1) {
        audio_decoder_ = new AudioDecoder(video_data_, nullptr);
        connect(audio_decoder_, &AudioDecoder::error, this, &VideoDecoder::HandleAudioError);
        connect(audio_decoder_, &AudioDecoder::playbackFinished, this, &VideoDecoder::HandleAudioFinished);
        connect(audio_decoder_, &AudioDecoder::positionChanged, this, &VideoDecoder::UpdateAudioPosition);
        audio_initialized_ = false;
    } else {
        qDebug() << "[VIDEO DECODER] Audio stream not found in file.";
    }

    emit videoInfo(codec_context_->width, codec_context_->height, frame_rate_, duration_, HasAudio());
    return true;
}

void VideoDecoder::ExtractFirstFrame() {
    // 1. basic FFmpeg initialization (similar to initialize(), but without audio)
    AVFormatContext *format_context = avformat_alloc_context();
    if (!format_context) return;

    QBuffer temp_buffer;
    temp_buffer.setData(video_data_);
    temp_buffer.open(QIODevice::ReadOnly);

    AVIOContext *io_ctx = avio_alloc_context(
        static_cast<unsigned char *>(av_malloc(8192)), 8192, 0, &temp_buffer,
        &VideoDecoder::ReadPacket, nullptr, &VideoDecoder::SeekPacket
    );
    if (!io_ctx) {
        avformat_free_context(format_context);
        return;
    }
    format_context->pb = io_ctx;
    format_context->flags |= AVFMT_FLAG_CUSTOM_IO;

    if (avformat_open_input(&format_context, nullptr, nullptr, nullptr) != 0) {
        av_freep(&io_ctx->buffer);
        avio_context_free(&io_ctx);
        avformat_free_context(format_context);
        return;
    }
    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        avformat_close_input(&format_context);
        av_freep(&io_ctx->buffer);
        avio_context_free(&io_ctx);
        return;
    }

    // 2. find the video stream
    int video_stream_idx = -1;
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) {
        avformat_close_input(&format_context);
        av_freep(&io_ctx->buffer);
        avio_context_free(&io_ctx);
        return;
    }

    // 3. open the video codec
    const AVCodecParameters *codec_params = format_context->streams[video_stream_idx]->codecpar;

    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) return;

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) return;
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) return;
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) return;

    // 4. prepare the frame and conversion context
    AVFrame *frame = av_frame_alloc();
    if (!frame) return;

    SwsContext *sws_context = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
        SWS_LANCZOS, nullptr, nullptr, nullptr
    );
    if (!sws_context) return;

    // 5.decode all the way to the first frame of video
    AVPacket packet;
    bool frame_decoded = false;
    while (av_read_frame(format_context, &packet) >= 0 && !frame_decoded) {
        if (packet.stream_index == video_stream_idx) {
            if (avcodec_send_packet(codec_ctx, &packet) == 0) {
                if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // success! we have a frame! convert and emit.
                    QImage frame_image(codec_ctx->width, codec_ctx->height, QImage::Format_RGB888);

                    uint8_t *dst_data[4] = {frame_image.bits(), nullptr, nullptr, nullptr};
                    const int dst_linesize[4] = {frame_image.bytesPerLine(), 0, 0, 0};

                    sws_scale(sws_context, frame->data, frame->linesize, 0,
                              codec_ctx->height, dst_data, dst_linesize);

                    emit frameReady(frame_image);
                    frame_decoded = true;
                }
            }
        }
        av_packet_unref(&packet);
    }

    // cleanup
    sws_freeContext(sws_context);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_context);
    av_freep(&io_ctx->buffer);
    avio_context_free(&io_ctx);
}

void VideoDecoder::Reset() {
    QMutexLocker locker(&mutex_);
    paused_ = true;
    video_finished_ = false;
    audio_finished_ = !HasAudio();
    first_frame_ = true;
    last_frame_pts_ = AV_NOPTS_VALUE;
    current_audio_position_ = 0.0;
    seek_position_ = 0;
    seeking_ = true;
    locker.unlock();

    if (audio_decoder_) {
        audio_decoder_->Seek(0.0);
        if (!audio_decoder_->IsPaused()) {
            audio_decoder_->Pause();
        }
    }

    wait_condition_.wakeOne();
}

void VideoDecoder::Stop() {
    QMutexLocker locker(&mutex_);
    if (stopped_) return;
    stopped_ = true;
    if (audio_decoder_) {
        audio_decoder_->Stop();
    }
    locker.unlock();
    wait_condition_.wakeOne();
}

void VideoDecoder::Pause() {
    mutex_.lock();
    paused_ = !paused_;
    if (audio_decoder_) {
        if (paused_ != audio_decoder_->IsPaused()) {
            audio_decoder_->Pause();
        }
    }
    mutex_.unlock();

    if (!paused_)
        wait_condition_.wakeOne();
}

void VideoDecoder::Seek(const double position) {
    if (audio_decoder_) {
        audio_decoder_->Seek(position);
    }

    int64_t timestamp = position * AV_TIME_BASE;
    timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q, format_context_->streams[video_stream_]->time_base);

    mutex_.lock();
    seek_position_ = timestamp;
    seeking_ = true;
    video_finished_ = false;
    audio_finished_ = !HasAudio();
    mutex_.unlock();

    if (paused_)
        wait_condition_.wakeOne();
}

void VideoDecoder::SetVolume(float volume) const {
    if (audio_decoder_) {
        audio_decoder_->SetVolume(volume);
    }
}

float VideoDecoder::GetVolume() const {
    return audio_decoder_ ? audio_decoder_->GetVolume() : 0.0f;
}

void VideoDecoder::run() {
    if (!format_context_ && !Initialize()) {
        return;
    }

    if (audio_decoder_ && !audio_initialized_) {
        if (!audio_decoder_->Initialize()) {
            qWarning() << "[VIDEO DECODER] Failed to initialize external AudioDecoder.";
            delete audio_decoder_;
            audio_decoder_ = nullptr;
        } else {
            audio_decoder_->start();
            audio_initialized_ = true;
            if (!audio_decoder_->IsPaused()) {
                audio_decoder_->Pause();
            }
        }
    }

    AVPacket packet;
    const double frame_duration = frame_rate_ > 0 ? 1000.0 / frame_rate_ : 33.3;

    frame_timer_.start();
    first_frame_ = true;
    video_finished_ = false;
    audio_finished_ = !HasAudio();

    while (!stopped_) {
        mutex_.lock();
        while (paused_ && !stopped_ && !seeking_) {
            wait_condition_.wait(&mutex_);
        }

        if (seeking_) {
            avcodec_flush_buffers(codec_context_);
            av_seek_frame(format_context_, video_stream_, seek_position_, AVSEEK_FLAG_BACKWARD);
            seeking_ = false;
            first_frame_ = true;
            last_frame_pts_ = AV_NOPTS_VALUE;
            frame_timer_.restart();
        }
        mutex_.unlock();

        if (stopped_) break;

        if (av_read_frame(format_context_, &packet) < 0) {
            QMutexLocker lock(&mutex_);
            video_finished_ = true;
            if (audio_finished_) {
                lock.unlock();
                emit playbackFinished();
                lock.relock();
            }
            paused_ = true;
            lock.unlock();
            msleep(50);
            continue;
        }

        if (packet.stream_index == video_stream_) {
            avcodec_send_packet(codec_context_, &packet);

            if (avcodec_receive_frame(codec_context_, frame_) == 0) {
                double video_pts = -1.0;
                if (frame_->pts != AV_NOPTS_VALUE) {
                    video_pts = frame_->pts * av_q2d(format_context_->streams[video_stream_]->time_base);
                }

                if (audio_decoder_ && video_pts >= 0) {
                    QMutexLocker sync_locker(&mutex_);
                    const double audio_pts = current_audio_position_;
                    sync_locker.unlock();

                    const double difference = video_pts - audio_pts;
                    constexpr double sync_threshold = 0.05;

                    if (difference > sync_threshold) {
                        constexpr double max_sleep = 100.0;
                        int sleep_time = qRound((difference - sync_threshold / 2.0) * 1000.0);
                        msleep(qMin(static_cast<int>(max_sleep), qMax(0, sleep_time)));
                    } else if (difference < -sync_threshold * 2.0) {
                        av_packet_unref(&packet);
                        av_frame_unref(frame_);
                        continue;
                    }
                } else if (!audio_decoder_) {
                    if (!first_frame_) {
                        const int64_t current_pts = frame_->pts;
                        double pts_difference = (current_pts - last_frame_pts_) *
                                                av_q2d(format_context_->streams[video_stream_]->time_base) * 1000.0;
                        if (pts_difference <= 0 || pts_difference > 1000.0) {
                            pts_difference = frame_duration;
                        }
                        const int elapsed_time = frame_timer_.elapsed();
                        const int sleep_time = qMax(0, qRound(pts_difference - elapsed_time));
                        if (sleep_time > 0) {
                            msleep(sleep_time);
                        }
                    }
                    first_frame_ = false;
                    last_frame_pts_ = frame_->pts;
                    frame_timer_.restart();
                }

                QImage frame_image(codec_context_->width, codec_context_->height, QImage::Format_RGB888);
                uint8_t *dst_data[4] = {frame_image.bits(), nullptr, nullptr, nullptr};
                const int dst_linesize[4] = {frame_image.bytesPerLine(), 0, 0, 0};
                sws_scale(sws_context_, frame_->data, frame_->linesize, 0,
                          codec_context_->height, dst_data, dst_linesize);

                emit frameReady(frame_image);
                av_frame_unref(frame_);
            }
        }
        av_packet_unref(&packet);
    }
}

void VideoDecoder::HandleAudioFinished() {
    QMutexLocker locker(&mutex_);
    audio_finished_ = true;
    if (video_finished_) {
        locker.unlock();
        emit playbackFinished();
    }
}

void VideoDecoder::UpdateAudioPosition(const double position) {
    QMutexLocker locker(&mutex_);
    current_audio_position_ = position;
    locker.unlock();
    emit positionChanged(position);
}

void VideoDecoder::CleanupFFmpegResources() {
    if (audio_decoder_) {
        audio_decoder_->Stop();
        audio_decoder_->wait(200);
        delete audio_decoder_;
        audio_decoder_ = nullptr;
    }
    audio_initialized_ = false;
    audio_finished_ = false;
    video_finished_ = false;

    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }
    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
    if (codec_context_) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }
    if (format_context_) {
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }
    audio_stream_index_ = -1;
    video_stream_ = -1;
}

int VideoDecoder::ReadPacket(void *opaque, uint8_t *buf, const int buf_size) {
    const auto buffer = static_cast<QBuffer *>(opaque);
    const qint64 bytes_read = buffer->read(reinterpret_cast<char *>(buf), buf_size);
    return static_cast<int>(bytes_read);
}

int64_t VideoDecoder::SeekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto buffer = static_cast<QBuffer *>(opaque);
    if (whence == AVSEEK_SIZE) {
        return buffer->size();
    }
    if (buffer->seek(offset)) {
        return buffer->pos();
    }
    return -1;
}
