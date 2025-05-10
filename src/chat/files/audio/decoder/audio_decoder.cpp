#include "audio_decoder.h"

extern "C" {
#include <libavutil/opt.h>
}

AudioDecoder::AudioDecoder(const QByteArray &audio_data, QObject *parent): QThread(parent), audio_data_(audio_data) {
    format_context_ = nullptr;
    audio_codec_context_ = nullptr;
    swr_context_ = nullptr;
    audio_frame_ = nullptr;
    audio_stream_ = -1;
    read_position_ = 0;
    audio_output_ = nullptr;
    audio_device_ = nullptr;
    paused_ = true;
    reached_end_of_stream_ = false;
    seeking_ = false;
    current_position_ = 0;
    initialized_ = false;

    io_buffer_ = static_cast<unsigned char *>(av_malloc(audio_data_.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!io_buffer_) {
        emit error("[AUDIO DECODER] Unable to allocate memory for audio data.");
        return;
    }
    memcpy(io_buffer_, audio_data_.constData(), audio_data_.size());

    io_context_ = avio_alloc_context(
        io_buffer_, audio_data_.size(), 0, this,
        &AudioDecoder::ReadPacket, nullptr, &AudioDecoder::SeekPacket
    );
    if (!io_context_) {
        av_free(io_buffer_);
        io_buffer_ = nullptr;
        emit error("[AUDIO DECODER] Unable to create I/O context.");
        return;
    }
}

AudioDecoder::~AudioDecoder() {
    Stop();
    wait();
    ReleaseResources();
}

void AudioDecoder::ReleaseResources() {
    QMutexLocker locker(&mutex_);

    if (audio_output_) {
        audio_output_->stop();
        delete audio_output_;
        audio_output_ = nullptr;
        audio_device_ = nullptr;
    }

    if (audio_frame_) {
        av_frame_free(&audio_frame_);
        audio_frame_ = nullptr;
    }

    if (audio_codec_context_) {
        avcodec_free_context(&audio_codec_context_);
        audio_codec_context_ = nullptr;
    }

    if (swr_context_) {
        swr_free(&swr_context_);
        swr_context_ = nullptr;
    }

    if (format_context_) {
        if (format_context_->pb == io_context_)
            format_context_->pb = nullptr;
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }

    initialized_ = false;
}

bool AudioDecoder::Reinitialize() {
    QMutexLocker locker(&mutex_);

    ReleaseResources();

    stopped_ = false;
    paused_ = true;
    reached_end_of_stream_ = false;
    seeking_ = false;
    current_position_ = 0;
    read_position_ = 0;

    if (!io_buffer_) {
        io_buffer_ = static_cast<unsigned char *>(av_malloc(audio_data_.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!io_buffer_) {
            emit error("[AUDIO DECODER] Unable to allocate memory for audio data.");
            return false;
        }
        memcpy(io_buffer_, audio_data_.constData(), audio_data_.size());
    }

    if (!io_context_) {
        io_context_ = avio_alloc_context(
            io_buffer_, audio_data_.size(), 0, this,
            &AudioDecoder::ReadPacket, nullptr, &AudioDecoder::SeekPacket
        );
        if (!io_context_) {
            av_free(io_buffer_);
            io_buffer_ = nullptr;
            emit error("[AUDIO DECODER] Unable to create I/O context.");
            return false;
        }
    }

    return true;
}

void AudioDecoder::SetVolume(const float volume) const {
    QMutexLocker locker(&mutex_);
    if (audio_output_) {
        audio_output_->setVolume(volume);
    }
}

float AudioDecoder::GetVolume() const {
    QMutexLocker locker(&mutex_);
    return audio_output_ ? audio_output_->volume() : 0.0f;
}

bool AudioDecoder::Initialize() {
    QMutexLocker locker(&mutex_);

    if (initialized_) return true;

    format_context_ = avformat_alloc_context();
    if (!format_context_) {
        emit error("[AUDIO DECODER] Cannot create a format context.");
        return false;
    }

    format_context_->pb = io_context_;

    if (avformat_open_input(&format_context_, "", nullptr, nullptr) < 0) {
        emit error("[AUDIO DECODER] Unable to open audio stream.");
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit error("[AUDIO DECODER] Unable to find stream information.");
        return false;
    }

    audio_stream_ = -1;
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_ = i;
            break;
        }
    }

    if (audio_stream_ == -1) {
        emit error("[AUDIO DECODER] Audio stream not found.");
        return false;
    }

    const AVCodec *audio_codec = avcodec_find_decoder(
        format_context_->streams[audio_stream_]->codecpar->codec_id
    );
    if (!audio_codec) {
        emit error("[AUDIO DECODER] No decoder found for this audio format.");
        return false;
    }

    audio_codec_context_ = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context_) {
        emit error("[AUDIO DECODER] Unable to create audio codec context.");
        return false;
    }

    if (avcodec_parameters_to_context(
            audio_codec_context_,
            format_context_->streams[audio_stream_]->codecpar
        ) < 0) {
        emit error("[AUDIO DECODER] Unable to copy audio codec parameters.");
        return false;
    }

    if (avcodec_open2(audio_codec_context_, audio_codec, nullptr) < 0) {
        emit error("[AUDIO DECODER] Unable to open audio codec.");
        return false;
    }

    audio_frame_ = av_frame_alloc();
    if (!audio_frame_) {
        emit error("[AUDIO DECODER] Audio frame cannot be allocated.");
        return false;
    }

    // resampling context
    swr_context_ = swr_alloc();
    if (!swr_context_) {
        emit error("[AUDIO DECODER] Unable to create audio resampling context.");
        return false;
    }

    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 2); // 2 channels (stereo)

    // resampling configuration to PCM S16LE stereo 44.1 kHz format
    av_opt_set_chlayout(swr_context_, "in_chlayout", &audio_codec_context_->ch_layout, 0);
    av_opt_set_chlayout(swr_context_, "out_chlayout", &out_layout, 0);
    av_opt_set_int(swr_context_, "in_sample_rate", audio_codec_context_->sample_rate, 0);
    av_opt_set_int(swr_context_, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(swr_context_, "in_sample_fmt", audio_codec_context_->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_context_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(swr_context_, "resampler_quality", 7, 0);

    if (swr_init(swr_context_) < 0) {
        emit error("[AUDIO DECODER] Audio resampling context cannot be initialized.");
        av_channel_layout_uninit(&out_layout);
        return false;
    }

    av_channel_layout_uninit(&out_layout);

    audio_format_.setSampleRate(44100);
    audio_format_.setChannelCount(2);
    audio_format_.setSampleType(QAudioFormat::SignedInt);
    audio_format_.setSampleSize(16);
    audio_format_.setCodec("audio/pcm");

    audio_output_ = new QAudioOutput(audio_format_, nullptr);
    audio_output_->setVolume(1.0);
    constexpr int suggested_buffer_size = 44100 * 2 * 2 / 2;
    audio_output_->setBufferSize(qMax(suggested_buffer_size, audio_output_->bufferSize()));
    audio_device_ = audio_output_->start();

    if (!audio_device_ || !audio_device_->isOpen()) {
        emit error("[AUDIO DECODER] Unable to open audio device.");
        return false;
    }

    const double duration = format_context_->duration / static_cast<double>(AV_TIME_BASE);
    emit audioInfo(audio_codec_context_->sample_rate, audio_codec_context_->ch_layout.nb_channels, duration);

    initialized_ = true;
    return true;
}

void AudioDecoder::Stop() {
    QMutexLocker locker(&mutex_);
    stopped_ = true;
    locker.unlock();
    wait_condition_.wakeOne();
}

void AudioDecoder::Pause() {
    QMutexLocker locker(&mutex_);
    paused_ = !paused_;
    locker.unlock();

    if (!paused_)
        wait_condition_.wakeOne();
}

bool AudioDecoder::IsPaused() const {
    QMutexLocker locker(&mutex_);
    return paused_;
}

void AudioDecoder::Seek(const double position) {
    QMutexLocker locker(&mutex_);

    if (!format_context_ || audio_stream_ < 0) return;

    int64_t timestamp = position * AV_TIME_BASE;

    // safe position check
    if (timestamp < 0) timestamp = 0;
    if (timestamp > format_context_->duration)
        timestamp = format_context_->duration > 0 ? format_context_->duration - 1 : 0;

    timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q,
                             format_context_->streams[audio_stream_]->time_base);

    seek_position_ = timestamp;
    seeking_ = true;
    locker.unlock();

    if (paused_)
        wait_condition_.wakeOne();
}

void AudioDecoder::Reset() {
    QMutexLocker locker(&mutex_);

    if (format_context_ && audio_stream_ >= 0) {
        av_seek_frame(format_context_, audio_stream_, 0, AVSEEK_FLAG_BACKWARD);
        if (audio_codec_context_) {
            avcodec_flush_buffers(audio_codec_context_);
        }
        current_position_ = 0;
        emit positionChanged(0);

        if (audio_output_) {
            audio_output_->reset();
            audio_device_ = audio_output_->start();
        }
    }

    seeking_ = false;
    seek_position_ = 0;
    reached_end_of_stream_ = false;
}

void AudioDecoder::run() {
    if (!Initialize()) {
        return;
    }

    AVPacket packet;
    QElapsedTimer position_timer;
    position_timer.start();

    double last_emitted_position = 0;
    constexpr int position_update_interval = 250; // ms

    while (!stopped_) {
        QMutexLocker locker(&mutex_);

        if (paused_) {
            wait_condition_.wait(&mutex_);
            locker.unlock();

            continue;
        }

        if (seeking_) {
            if (format_context_ && audio_stream_ >= 0) {
                av_seek_frame(format_context_, audio_stream_, seek_position_, AVSEEK_FLAG_BACKWARD);
                if (audio_codec_context_) {
                    avcodec_flush_buffers(audio_codec_context_);
                }
                seeking_ = false;
                position_timer.restart();

                current_position_ = seek_position_ * av_q2d(format_context_->streams[audio_stream_]->time_base);
                emit positionChanged(current_position_);

                last_emitted_position = current_position_;

                if (audio_output_) {
                    audio_output_->reset();
                    audio_device_ = audio_output_->start();
                }
            }
        }
        locker.unlock();

        if (!format_context_) break;

        const int read_result = av_read_frame(format_context_, &packet);

        if (read_result < 0) {
            emit playbackFinished();
            QMutexLocker locker2(&mutex_);
            paused_ = true;
            reached_end_of_stream_ = true;
            continue;
        }

        if (packet.stream_index == audio_stream_ && audio_codec_context_) {
            avcodec_send_packet(audio_codec_context_, &packet);
            while (avcodec_receive_frame(audio_codec_context_, audio_frame_) == 0) {
                if (audio_frame_->pts != AV_NOPTS_VALUE) {
                    QMutexLocker position_locker(&mutex_);
                    current_position_ = audio_frame_->pts *
                                        av_q2d(format_context_->streams[audio_stream_]->time_base);
                    position_locker.unlock();

                    if (abs(current_position_ - last_emitted_position) > 0.25 ||
                        position_timer.elapsed() > position_update_interval) {
                        emit positionChanged(current_position_);
                        last_emitted_position = current_position_;
                        position_timer.restart();
                    }
                }

                DecodeAudioFrame(audio_frame_); {
                    QMutexLocker mutex_locker(&mutex_);
                    if (audio_output_ && audio_device_ && audio_device_->isOpen()) {
                        const int threshold = audio_output_->bufferSize() / 4;
                        while (audio_output_->bytesFree() < threshold && !paused_ && !stopped_ && !seeking_) {
                            mutex_locker.unlock();
                            msleep(10);
                            mutex_locker.relock();
                        }
                    }
                }
            }
        }

        av_packet_unref(&packet);
    }
}

void AudioDecoder::DecodeAudioFrame(const AVFrame *audio_frame) const {
    QByteArray buffer_to_write; {
        QMutexLocker locker(&mutex_);
        if (!audio_device_ || !audio_device_->isOpen() || paused_) {
            return;
        }

        const int out_samples = audio_frame->nb_samples;
        const int out_buffer_size = out_samples * 2 * 2;
        auto out_buffer = static_cast<uint8_t *>(av_malloc(out_buffer_size));
        if (!out_buffer) return;

        const int converted_samples = swr_convert(
            swr_context_,
            &out_buffer, out_samples,
            audio_frame->extended_data, audio_frame->nb_samples
        );

        if (converted_samples > 0) {
            const int actual_size = converted_samples * 2 * 2;
            buffer_to_write = QByteArray(reinterpret_cast<char *>(out_buffer), actual_size);
        }

        av_freep(&out_buffer);
    }

    QMutexLocker pause_check_locker(&mutex_);
    pause_check_locker.unlock();


    audio_device_->write(buffer_to_write);
}

int AudioDecoder::ReadPacket(void *opaque, uint8_t *buf, const int buf_size) {
    const auto decoder = static_cast<AudioDecoder *>(opaque);
    const int size = qMin(buf_size, decoder->read_position_ >= decoder->audio_data_.size()
                                        ? 0
                                        : decoder->audio_data_.size() - decoder->read_position_);

    if (size <= 0)
        return AVERROR_EOF;

    memcpy(buf, decoder->audio_data_.constData() + decoder->read_position_, size);
    decoder->read_position_ += size;
    return size;
}

int64_t AudioDecoder::SeekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto decoder = static_cast<AudioDecoder *>(opaque);

    switch (whence) {
        case SEEK_SET:
            decoder->read_position_ = offset;
            break;
        case SEEK_CUR:
            decoder->read_position_ += offset;
            break;
        case SEEK_END:
            decoder->read_position_ = decoder->audio_data_.size() + offset;
            break;
        case AVSEEK_SIZE:
            return decoder->audio_data_.size();
        default:
            return -1;
    }

    return decoder->read_position_;
}
