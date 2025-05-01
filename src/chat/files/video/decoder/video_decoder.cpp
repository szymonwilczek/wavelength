#include "video_decoder.h"

VideoDecoder::VideoDecoder(const QByteArray &video_data, QObject *parent): QThread(parent), video_data_(video_data), paused_(true) {
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
        emit error("Nie można otworzyć strumienia wideo (avformat_open_input)");
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit error("Nie można znaleźć informacji o strumieniu (avformat_find_stream_info)");
        return false;
    }

    video_stream_ = -1;
    audio_stream_index_ = -1;
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (video_stream_ == -1 && format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_ = i;
        } else if (audio_stream_index_ == -1 && format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index_ = i;
        }
    }

    if (video_stream_ == -1) {
        emit error("Nie znaleziono strumienia wideo");
        return false;
    }

    const AVCodecParameters* codec_params = format_context_->streams[video_stream_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        emit error("Nie znaleziono kodeka wideo");
        return false;
    }

    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) return false;
    if (avcodec_parameters_to_context(codec_context_, codec_params) < 0) return false;
    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        emit error("Nie można otworzyć kodeka wideo");
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
        emit error("Nie można utworzyć kontekstu konwersji wideo (sws_getContext)");
        return false;
    }

    const AVRational frame_rate_rational = av_guess_frame_rate(format_context_, format_context_->streams[video_stream_], nullptr);
    frame_rate_ = frame_rate_rational.num > 0 && frame_rate_rational.den > 0 ? av_q2d(frame_rate_rational) : 30.0;
    duration_ = format_context_->duration > 0 ? static_cast<double>(format_context_->duration) / AV_TIME_BASE : 0.0;

    if (audio_stream_index_ != -1) {
        audio_decoder_ = new AudioDecoder(video_data_, nullptr);
        connect(audio_decoder_, &AudioDecoder::error, this, &VideoDecoder::HandleAudioError);
        connect(audio_decoder_, &AudioDecoder::playbackFinished, this, &VideoDecoder::HandleAudioFinished);
        connect(audio_decoder_, &AudioDecoder::positionChanged, this, &VideoDecoder::UpdateAudioPosition);
        audio_initialized_ = false;
    } else {
        qDebug() << "Nie znaleziono strumienia audio w pliku.";
    }

    emit videoInfo(codec_context_->width, codec_context_->height, frame_rate_, duration_, HasAudio());
    return true;
}

void VideoDecoder::ExtractFirstFrame() {
    // Ta metoda działa synchronicznie i nie używa głównej pętli run()
    // ani zewnętrznego AudioDecoder.

    // 1. Podstawowa inicjalizacja FFmpeg (podobna do initialize(), ale bez audio)
    AVFormatContext* format_context = avformat_alloc_context();
    if (!format_context) return;

    // Użyj kopii bufora, aby nie zakłócać głównego odtwarzania
    QBuffer temp_buffer;
    temp_buffer.setData(video_data_);
    temp_buffer.open(QIODevice::ReadOnly);

    AVIOContext* io_ctx = avio_alloc_context(
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
        avformat_close_input(&format_context); // Zamknij przed zwolnieniem IO
        av_freep(&io_ctx->buffer);
        avio_context_free(&io_ctx);
        return;
    }

    // 2. Znajdź strumień wideo
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

    // 3. Otwórz kodek wideo
    const AVCodecParameters* codec_params = format_context->streams[video_stream_idx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) { /* cleanup */ return; }
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) { /* cleanup */ return; }
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) { /* cleanup */ return; }
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) { /* cleanup */ return; }

    // 4. Przygotuj ramkę i kontekst konwersji
    AVFrame* frame = av_frame_alloc();
    if (!frame) { /* cleanup */ return; }
    SwsContext* sws_context = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
        SWS_LANCZOS, nullptr, nullptr, nullptr
    );
    if (!sws_context) { /* cleanup */ return; }

    // 5. Dekoduj aż do pierwszej klatki wideo
    AVPacket packet;
    bool frame_decoded = false;
    while (av_read_frame(format_context, &packet) >= 0 && !frame_decoded) {
        if (packet.stream_index == video_stream_idx) {
            if (avcodec_send_packet(codec_ctx, &packet) == 0) {
                if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // Mamy klatkę! Konwertuj i emituj.
                    QImage frame_image(codec_ctx->width, codec_ctx->height, QImage::Format_RGB888);
                    uint8_t* dst_data[4] = { frame_image.bits(), nullptr, nullptr, nullptr };
                    const int dst_linesize[4] = { frame_image.bytesPerLine(), 0, 0, 0 };
                    sws_scale(sws_context, (const uint8_t* const*)frame->data, frame->linesize, 0,
                              codec_ctx->height, dst_data, dst_linesize);

                    emit frameReady(frame_image); // Emituj sygnał
                    frame_decoded = true;
                }
            }
        }
        av_packet_unref(&packet);
    }

    // 6. Cleanup
    sws_freeContext(sws_context);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_context);
    av_freep(&io_ctx->buffer);
    avio_context_free(&io_ctx);
}

void VideoDecoder::Reset() {
    QMutexLocker locker(&mutex_);
    paused_ = true; // Zatrzymaj na czas resetu
    video_finished_ = false;
    audio_finished_ = !HasAudio(); // Resetuj flagę audio
    first_frame_ = true;
    last_frame_pts_ = AV_NOPTS_VALUE;
    current_audio_position_ = 0.0; // Resetuj pozycję audio

    // Przewiń strumień wideo do początku
    seek_position_ = 0;
    seeking_ = true; // Ustaw flagę seek, pętla run() to obsłuży

    locker.unlock();

    // Przewiń zewnętrzny dekoder audio
    if (audio_decoder_) {
        audio_decoder_->Seek(0.0);
        // Upewnij się, że audio jest spauzowane
        if (!audio_decoder_->IsPaused()) {
            audio_decoder_->Pause();
        }
    }

    // Obudź wątek, jeśli czekał
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
    audio_finished_ = !HasAudio(); // Reset audio finished only if audio exists
    mutex_.unlock();

    if (paused_)
        wait_condition_.wakeOne();
}

void VideoDecoder::run() {
    if (!format_context_ && !Initialize()) {
        return;
    }

    if (audio_decoder_ && !audio_initialized_) {
        if (!audio_decoder_->Initialize()) {
            qWarning() << "Nie udało się zainicjalizować zewnętrznego AudioDecoder.";
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
            qDebug() << "Koniec strumienia wideo (av_read_frame < 0)";
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
                            QThread::msleep(sleep_time);
                        }
                    }
                    first_frame_ = false;
                    last_frame_pts_ = frame_->pts;
                    frame_timer_.restart();
                }

                QImage frame_image(codec_context_->width, codec_context_->height, QImage::Format_RGB888);
                uint8_t* dst_data[4] = { frame_image.bits(), nullptr, nullptr, nullptr };
                const int dst_linesize[4] = { frame_image.bytesPerLine(), 0, 0, 0 };
                sws_scale(sws_context_, (const uint8_t* const*)frame_->data, frame_->linesize, 0,
                          codec_context_->height, dst_data, dst_linesize);

                emit frameReady(frame_image);
                av_frame_unref(frame_);
            }
        } else if (packet.stream_index == audio_stream_index_) {
            // Ignoruj pakiety audio
        }

        av_packet_unref(&packet);
    }
    qDebug() << "VideoDecoder::run() zakończył pętlę.";
}

void VideoDecoder::HandleAudioFinished() {
    qDebug() << "AudioDecoder zakończył odtwarzanie.";
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
        avcodec_close(codec_context_);
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
    const auto buffer = static_cast<QBuffer*>(opaque);
    const qint64 bytes_read = buffer->read(reinterpret_cast<char *>(buf), buf_size);
    return static_cast<int>(bytes_read);
}

int64_t VideoDecoder::SeekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto buffer = static_cast<QBuffer*>(opaque);
    if (whence == AVSEEK_SIZE) {
        return buffer->size();
    }
    if (buffer->seek(offset)) {
        return buffer->pos();
    }
    return -1;
}
