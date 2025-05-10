#include "gif_decoder.h"

#include <QDebug>
#include <QImage>

GifDecoder::GifDecoder(const QByteArray &gif_data, QObject *parent): QThread(parent), gif_data_(gif_data) {
    format_context_ = nullptr;
    codec_context_ = nullptr;
    sws_context_ = nullptr;
    frame_ = nullptr;
    frame_rgb_ = nullptr;
    gif_stream_ = -1;
    buffer_ = nullptr;
    buffer_size_ = 0;
    paused_ = true;
    current_position_ = 0;
    reached_end_of_stream_ = false;
    initialized_ = false;
    frame_delay_ = 100;

    io_buffer_ = static_cast<unsigned char *>(av_malloc(gif_data_.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!io_buffer_) {
        emit error("[GIF DECODER] Unable to allocate memory for GIF data.");
        return;
    }
    memcpy(io_buffer_, gif_data_.constData(), gif_data_.size());

    io_context_ = avio_alloc_context(
        io_buffer_, gif_data_.size(), 0, this,
        &GifDecoder::ReadPacket, nullptr, &GifDecoder::SeekPacket
    );
    if (!io_context_) {
        av_free(io_buffer_);
        io_buffer_ = nullptr;
        emit error("[GIF DECODER] Unable to create I/O context.");
        return;
    }
}

GifDecoder::~GifDecoder() {
    Stop();
    wait(500);

    ReleaseResources();

    if (io_context_) {
        if (io_context_->buffer) {
            av_free(io_context_->buffer);
            io_context_->buffer = nullptr;
        }
        avio_context_free(&io_context_);
        io_context_ = nullptr;
    } else if (io_buffer_) {
        av_free(io_buffer_);
        io_buffer_ = nullptr;
    }
}

void GifDecoder::ReleaseResources() {
    QMutexLocker locker(&mutex_);

    if (buffer_) {
        av_free(buffer_);
        buffer_ = nullptr;
    }

    if (frame_rgb_) {
        av_frame_free(&frame_rgb_);
        frame_rgb_ = nullptr;
    }

    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }

    if (codec_context_) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }

    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }

    if (format_context_) {
        if (format_context_->pb == io_context_)
            format_context_->pb = nullptr;
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }

    initialized_ = false;
}

bool GifDecoder::Reinitialize() {
    QMutexLocker locker(&mutex_);

    if (IsDecoderRunning()) {
        stopped_ = true;
        wait_condition_.wakeAll();
        locker.unlock();
        wait(500);
        locker.relock();
    }

    ReleaseResources();

    stopped_ = false;
    paused_ = true;
    current_position_ = 0;
    read_position_ = 0;
    reached_end_of_stream_ = false;

    if (!io_buffer_) {
        io_buffer_ = static_cast<unsigned char *>(av_malloc(gif_data_.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!io_buffer_) {
            emit error("[GIF DECODER] Unable to allocate memory for GIF data.");
            return false;
        }
        memcpy(io_buffer_, gif_data_.constData(), gif_data_.size());
    }

    if (!io_context_) {
        io_context_ = avio_alloc_context(
            io_buffer_, gif_data_.size(), 0, this,
            &GifDecoder::ReadPacket, nullptr, &GifDecoder::SeekPacket
        );
        if (!io_context_) {
            av_free(io_buffer_);
            io_buffer_ = nullptr;
            emit error("[GIF DECODER] Unable to create I/O context.");
            return false;
        }
    }

    return true;
}

bool GifDecoder::Initialize() {
    QMutexLocker locker(&mutex_);

    if (initialized_) return true;

    format_context_ = avformat_alloc_context();
    if (!format_context_) {
        emit error("[GIF DECODER] Cannot create a format context.");
        return false;
    }

    format_context_->pb = io_context_;

    if (avformat_open_input(&format_context_, "", nullptr, nullptr) < 0) {
        emit error("[GIF DECODER] Unable to open GIF stream.");
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit error("[GIF DECODER] Unable to find stream information.");
        return false;
    }

    gif_stream_ = -1;
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            gif_stream_ = i;
            break;
        }
    }

    if (gif_stream_ == -1) {
        emit error("[GIF DECODER] GIF stream not found.");
        return false;
    }

    const AVStream *stream = format_context_->streams[gif_stream_];
    if (stream->avg_frame_rate.den && stream->avg_frame_rate.num) {
        frame_rate_ = av_q2d(stream->avg_frame_rate);
        frame_delay_ = 1000.0 / frame_rate_; // ms
    } else if (stream->r_frame_rate.den && stream->r_frame_rate.num) {
        frame_rate_ = av_q2d(stream->r_frame_rate);
        frame_delay_ = 1000.0 / frame_rate_; // ms
    } else {
        frame_rate_ = 10; // default value if wasn't set (nor recognized)
        frame_delay_ = 100; // 100 ms = 10 FPS
    }

    const AVCodec *codec = avcodec_find_decoder(format_context_->streams[gif_stream_]->codecpar->codec_id);
    if (!codec) {
        emit error("[GIF DECODER] No decoder found for GIF format.");
        return false;
    }

    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) {
        emit error("[GIF DECODER] Unable to create codec context.");
        return false;
    }

    if (avcodec_parameters_to_context(codec_context_, format_context_->streams[gif_stream_]->codecpar) < 0) {
        emit error("[GIF DECODER] Unable to copy codec parameters.");
        return false;
    }

    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        emit error("[GIF DECODER] Unable to open codec.");
        return false;
    }

    frame_ = av_frame_alloc();
    frame_rgb_ = av_frame_alloc();

    if (!frame_ || !frame_rgb_) {
        emit error("[GIF DECODER] Unable to allocate GIF frames.");
        return false;
    }

    buffer_size_ = av_image_get_buffer_size(
        AV_PIX_FMT_RGBA,
        codec_context_->width,
        codec_context_->height,
        1
    );

    buffer_ = static_cast<uint8_t *>(av_malloc(buffer_size_));
    if (!buffer_) {
        emit error("[GIF DECODER] Unable to allocate GIF buffer.");
        return false;
    }

    av_image_fill_arrays(
        frame_rgb_->data, frame_rgb_->linesize, buffer_,
        AV_PIX_FMT_RGBA, codec_context_->width, codec_context_->height, 1
    );

    sws_context_ = sws_getContext(
        codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
        codec_context_->width, codec_context_->height, AV_PIX_FMT_RGBA, // rgba for handling opacity
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_context_) {
        emit error("[GIF DECODER] Cannot create a conversion context.");
        return false;
    }

    emit gifInfo(
        codec_context_->width,
        codec_context_->height,
        format_context_->duration / AV_TIME_BASE,
        frame_rate_,
        format_context_->nb_streams // should be only 1 (for GIF)
    );

    ExtractAndEmitFirstFrameInternal();

    initialized_ = true;
    paused_ = true;
    stopped_ = false;
    return true;
}

void GifDecoder::Stop() {
    QMutexLocker locker(&mutex_);
    stopped_ = true;
    paused_ = false;
    locker.unlock();
    wait_condition_.wakeAll();
}

void GifDecoder::Pause() {
    QMutexLocker locker(&mutex_);
    if (!stopped_) {
        paused_ = true;
    }
}

void GifDecoder::Reset() {
    QMutexLocker locker(&mutex_);

    if (format_context_ && gif_stream_ >= 0) {
        av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
        if (codec_context_) {
            avcodec_flush_buffers(codec_context_);
        }
        current_position_ = 0;
        emit positionChanged(0);
    }

    seek_position_ = 0;
    reached_end_of_stream_ = false;
}

void GifDecoder::Resume() {
    QMutexLocker locker(&mutex_);
    if (stopped_) return;

    if (paused_) {
        paused_ = false;
        if (!IsDecoderRunning()) {
            locker.unlock();
            start();
            locker.relock();
        } else {
            wait_condition_.wakeAll();
        }
    } else {
    }
}

void GifDecoder::run() {
    if (!Initialize()) {
        qDebug() << "[GIF DECODER]::run() - Initialization failed. Exiting thread.";
        return;
    }

    AVPacket packet;
    int frame_counter = 0;
    frame_timer_.start();
    bool is_first_frame_after_resume = true;

    while (true) {
        {
            QMutexLocker locker(&mutex_);
            if (stopped_) {
                break;
            }
            if (paused_) {
                wait_condition_.wait(&mutex_);
                continue;
            }
        }

        const int read_result = av_read_frame(format_context_, &packet);

        if (read_result < 0) {
            if (read_result == AVERROR_EOF) {
                QMutexLocker locker(&mutex_);
                const int seek_ret = av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);

                if (seek_ret < 0) {
                    qDebug() << "[GIF DECODER]::run() - Loop seek failed:" << seek_ret;
                    stopped_ = true;
                    break;
                }

                if (codec_context_) avcodec_flush_buffers(codec_context_);
                current_position_ = 0;
                frame_timer_.restart();
                is_first_frame_after_resume = true;
                locker.unlock();
                emit positionChanged(0);
                locker.relock();
                av_packet_unref(&packet);
                continue;
            }

            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {};
            av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, read_result);
            qDebug() << "[GIF DECODER]::run() - Error reading frame:" << read_result << errbuf;
            stopped_ = true;
            break;
        }

        // packet processing
        if (packet.stream_index == gif_stream_) {
            const int send_result = avcodec_send_packet(codec_context_, &packet);
            if (send_result < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, send_result);
                qDebug() << "[GIF DECODER]::run() - Error sending packet to decoder:" << send_result << errbuf;
            } else {
                while (true) {
                    const int receive_result = avcodec_receive_frame(codec_context_, frame_);

                    if (receive_result == 0) {
                        // success -> we have a frame!
                        frame_counter++;

                        if (frame_->pts != AV_NOPTS_VALUE) {
                            QMutexLocker posLocker(&mutex_);
                            current_position_ = frame_->pts * av_q2d(format_context_->streams[gif_stream_]->time_base);
                            posLocker.unlock();
                            emit positionChanged(current_position_);
                        }

                        sws_scale(
                            sws_context_,
                            frame_->data, frame_->linesize, 0, codec_context_->height,
                            frame_rgb_->data, frame_rgb_->linesize
                        );

                        QImage frame_image(
                            frame_rgb_->data[0],
                            codec_context_->width,
                            codec_context_->height,
                            frame_rgb_->linesize[0],
                            QImage::Format_RGBA8888 // rgba for handling opacity
                        );

                        emit frameReady(frame_image.copy());

                        qint64 delay_time = static_cast<qint64>(frame_delay_);

                        if (!is_first_frame_after_resume) {
                            const qint64 elapsed = frame_timer_.elapsed();
                            delay_time = qMax(static_cast<qint64>(0), delay_time - elapsed);
                        }

                        if (delay_time > 0) {
                            msleep(delay_time);
                        }
                        frame_timer_.restart();
                        is_first_frame_after_resume = false;
                    } else if (receive_result == AVERROR(EAGAIN) || receive_result == AVERROR_EOF) {
                        break;
                    } else {
                        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, receive_result);
                        qDebug() << "[GIF DECODER]::run() - Error receiving frame from decoder:" << receive_result <<
                                errbuf;
                        stopped_ = true; // critical error, stop the thread
                        break;
                    }
                }
            }
        }

        av_packet_unref(&packet);
        if (stopped_) break;
    }
}

int GifDecoder::ReadPacket(void *opaque, uint8_t *buf, const int buf_size) {
    const auto decoder = static_cast<GifDecoder *>(opaque);
    const int size = qMin(buf_size, decoder->read_position_ >= decoder->gif_data_.size()
                                        ? 0
                                        : decoder->gif_data_.size() - decoder->read_position_);

    if (size <= 0)
        return AVERROR_EOF;

    memcpy(buf, decoder->gif_data_.constData() + decoder->read_position_, size);
    decoder->read_position_ += size;
    return size;
}

int64_t GifDecoder::SeekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto decoder = static_cast<GifDecoder *>(opaque);

    switch (whence) {
        case SEEK_SET:
            decoder->read_position_ = offset;
            break;
        case SEEK_CUR:
            decoder->read_position_ += offset;
            break;
        case SEEK_END:
            decoder->read_position_ = decoder->gif_data_.size() + offset;
            break;
        case AVSEEK_SIZE:
            return decoder->gif_data_.size();
        default:
            return -1;
    }

    return decoder->read_position_;
}

void GifDecoder::ExtractAndEmitFirstFrameInternal() {
    if (!format_context_ || gif_stream_ < 0 || !codec_context_ || !sws_context_ || !frame_ || !frame_rgb_) {
        qWarning() << "[GIF DECODER] Cannot extract first frame, context not ready.";
        return;
    }

    av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codec_context_);
    read_position_ = 0;

    AVPacket packet;
    // ReSharper disable once CppDeprecatedEntity
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    bool frame_decoded = false;
    while (av_read_frame(format_context_, &packet) >= 0 && !frame_decoded) {
        if (packet.stream_index == gif_stream_) {
            if (avcodec_send_packet(codec_context_, &packet) >= 0) {
                if (avcodec_receive_frame(codec_context_, frame_) == 0) {
                    sws_scale(
                        sws_context_,
                        frame_->data, frame_->linesize, 0, codec_context_->height,
                        frame_rgb_->data, frame_rgb_->linesize
                    );

                    QImage firstFrame(
                        frame_rgb_->data[0],
                        codec_context_->width,
                        codec_context_->height,
                        frame_rgb_->linesize[0],
                        QImage::Format_RGBA8888
                    );

                    emit firstFrameReady(firstFrame.copy());
                    frame_decoded = true;
                }
            }
        }
        av_packet_unref(&packet);
    }
    av_packet_unref(&packet);

    av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codec_context_);
    read_position_ = 0;

    if (!frame_decoded) {
        qWarning() << "[GIF DECODER] Failed to extract the first frame.";
        emit firstFrameReady(QImage());
    }
}
