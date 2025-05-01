#include "audio_decoder.h"

extern "C" {
    #include <libavutil/opt.h>
}

AudioDecoder::AudioDecoder(const QByteArray &audio_data, QObject *parent): QThread(parent), audio_data_(audio_data), stopped_(false) {
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

    // Zainicjuj bufor z danymi audio
    io_buffer_ = static_cast<unsigned char *>(av_malloc(audio_data_.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!io_buffer_) {
        emit error("Nie można zaalokować pamięci dla danych audio");
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
        emit error("Nie można utworzyć kontekstu I/O");
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
        avcodec_close(audio_codec_context_);
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
            emit error("Nie można zaalokować pamięci dla danych audio");
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
            emit error("Nie można utworzyć kontekstu I/O");
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
        emit error("Nie można utworzyć kontekstu formatu");
        return false;
    }

    format_context_->pb = io_context_;

    if (avformat_open_input(&format_context_, "", nullptr, nullptr) < 0) {
        emit error("Nie można otworzyć strumienia audio");
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit error("Nie można znaleźć informacji o strumieniu");
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
        emit error("Nie znaleziono strumienia audio");
        return false;
    }

    const AVCodec* audio_codec = avcodec_find_decoder(
        format_context_->streams[audio_stream_]->codecpar->codec_id
    );
    if (!audio_codec) {
        emit error("Nie znaleziono dekodera dla tego formatu audio");
        return false;
    }

    audio_codec_context_ = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context_) {
        emit error("Nie można utworzyć kontekstu kodeka audio");
        return false;
    }

    // Skopiuj parametry kodeka audio
    if (avcodec_parameters_to_context(
            audio_codec_context_,
            format_context_->streams[audio_stream_]->codecpar
        ) < 0) {
        emit error("Nie można skopiować parametrów kodeka audio");
        return false;
    }

    // Otwórz kodek audio
    if (avcodec_open2(audio_codec_context_, audio_codec, nullptr) < 0) {
        emit error("Nie można otworzyć kodeka audio");
        return false;
    }

    // Alokuj ramkę audio
    audio_frame_ = av_frame_alloc();
    if (!audio_frame_) {
        emit error("Nie można zaalokować ramki audio");
        return false;
    }

    // Utwórz kontekst resamplingu
    swr_context_ = swr_alloc();
    if (!swr_context_) {
        emit error("Nie można utworzyć kontekstu resamplingu audio");
        return false;
    }

    // Inicjalizacja layoutu kanałów wyjściowych (stereo)
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 2); // 2 kanały (stereo)

    // Konfiguracja resamplingu do formatu PCM S16LE stereo 44.1kHz
    av_opt_set_chlayout(swr_context_, "in_chlayout", &audio_codec_context_->ch_layout, 0);
    av_opt_set_chlayout(swr_context_, "out_chlayout", &out_layout, 0);
    av_opt_set_int(swr_context_, "in_sample_rate", audio_codec_context_->sample_rate, 0);
    av_opt_set_int(swr_context_, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(swr_context_, "in_sample_fmt", audio_codec_context_->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_context_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(swr_context_, "resampler_quality", 7, 0);

    if (swr_init(swr_context_) < 0) {
        emit error("Nie można zainicjalizować kontekstu resamplingu audio");
        av_channel_layout_uninit(&out_layout);
        return false;
    }

    // Zwolnij pamięć layoutu kanałów wyjściowych
    av_channel_layout_uninit(&out_layout);

    // Konfiguracja formatu audio dla Qt
    audio_format_.setSampleRate(44100);
    audio_format_.setChannelCount(2);
    audio_format_.setSampleType(QAudioFormat::SignedInt);
    audio_format_.setSampleSize(16);
    audio_format_.setCodec("audio/pcm");

    // Inicjalizacja wyjścia audio
    audio_output_ = new QAudioOutput(audio_format_, nullptr);
    audio_output_->setVolume(1.0);
    constexpr int suggested_buffer_size = 44100 * 2 * 2 / 2;
    audio_output_->setBufferSize(qMax(suggested_buffer_size, audio_output_->bufferSize()));
    audio_device_ = audio_output_->start();

    if (!audio_device_ || !audio_device_->isOpen()) {
        emit error("Nie można otworzyć urządzenia audio");
        return false;
    }

    // Wyemituj informacje o audio
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

    // Bezpieczne ograniczenie pozycji
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

    // Resetuj pozycję strumienia na początek
    if (format_context_ && audio_stream_ >= 0) {
        av_seek_frame(format_context_, audio_stream_, 0, AVSEEK_FLAG_BACKWARD);
        if (audio_codec_context_) {
            avcodec_flush_buffers(audio_codec_context_);
        }
        current_position_ = 0;
        emit positionChanged(0);

        // Upewniamy się, że urządzenie audio jest gotowe
        if (audio_output_) {
            audio_output_->reset();
            audio_device_ = audio_output_->start();
        }
    }

    // Resetuj flagi stanu
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

    // Potrzebna zmienna do śledzenia poprzedniej pozycji
    double last_emitted_position = 0;
    constexpr int position_update_interval = 250; // ms

    // Limiter tempa odtwarzania
    // QElapsedTimer playbackTimer;
    // playbackTimer.start();

    // Ile próbek zostało przetworzone
    // int64_t samplesProcessed = 0;
    // int64_t lastReportedSamples = 0;

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
                // playbackTimer.restart();

                // Aktualizacja pozycji po przeszukiwaniu
                current_position_ = seek_position_ * av_q2d(format_context_->streams[audio_stream_]->time_base);
                emit positionChanged(current_position_);

                last_emitted_position = current_position_;
                // samplesProcessed = 0;
                // lastReportedSamples = 0;

                // Reset bufora audio
                if (audio_output_) {
                    audio_output_->reset();
                    audio_device_ = audio_output_->start();
                }
            }
        }
        locker.unlock();

        // Sprawdź czy kontekst formatu jest nadal ważny
        if (!format_context_) break;

        // Czytaj kolejny pakiet
        const int read_result = av_read_frame(format_context_, &packet);
        if (read_result < 0) {
            // Koniec strumienia
            emit playbackFinished();
            QMutexLocker locker2(&mutex_);
            paused_ = true;
            reached_end_of_stream_ = true;
            continue;
        }

        if (packet.stream_index == audio_stream_ && audio_codec_context_) {
            avcodec_send_packet(audio_codec_context_, &packet);
            while (avcodec_receive_frame(audio_codec_context_, audio_frame_) == 0) {
                // Aktualizacja pozycji odtwarzania
                if (audio_frame_->pts != AV_NOPTS_VALUE) {
                    QMutexLocker position_locker(&mutex_);
                    current_position_ = audio_frame_->pts *
                                        av_q2d(format_context_->streams[audio_stream_]->time_base);
                    position_locker.unlock();

                    // Wysyłamy powiadomienia o zmianie pozycji w kontrolowanych odstępach czasu
                    if (abs(current_position_ - last_emitted_position) > 0.25 ||
                        position_timer.elapsed() > position_update_interval) {
                        emit positionChanged(current_position_);
                        last_emitted_position = current_position_;
                        position_timer.restart();
                    }
                }

                // Dodaj liczbę próbek do licznika
                // samplesProcessed += m_audioFrame->nb_samples;
                //
                // // Obliczamy ile czasu powinno minąć dla tej liczby próbek
                // double expectedTime = (samplesProcessed - lastReportedSamples) * 1000.0 / 44100.0;
                //
                // // Jeśli przetwarzamy zbyt szybko, zaczekaj
                // if (playbackTimer.elapsed() < expectedTime) {
                //     QThread::msleep(static_cast<unsigned long>(expectedTime - playbackTimer.elapsed()));
                // }

                // Dekodujemy i wysyłamy ramkę audio
                DecodeAudioFrame(audio_frame_);

                // Aktualizujemy licznik i timer
                // if (samplesProcessed - lastReportedSamples > 44100) {  // co około 1 sekundę
                //     lastReportedSamples = samplesProcessed;
                //     playbackTimer.restart();
                // }

                {
                    QMutexLocker mutex_locker(&mutex_);
                    if (audio_output_ && audio_device_ && audio_device_->isOpen()) {
                        // Poczekaj, jeśli mniej niż np. 1/4 bufora jest wolna
                        const int threshold = audio_output_->bufferSize() / 4;
                        while (audio_output_->bytesFree() < threshold && !paused_ && !stopped_ && !seeking_) {
                            mutex_locker.unlock(); // Zwolnij mutex na czas czekania
                            msleep(10); // Krótka pauza, aby nie obciążać CPU
                            mutex_locker.relock();   // Zablokuj ponownie przed sprawdzeniem warunku

                        }
                    }
                } // Mutex zwolniony
            }
        }

        av_packet_unref(&packet);
    }
}

void AudioDecoder::DecodeAudioFrame(const AVFrame *audio_frame) const {
    // <<< ZMIANA: Przeniesiono deklaracje na zewnątrz bloku mutexa >>>
    QByteArray buffer_to_write; { // <<< Początek bloku dla mutexa >>>
        QMutexLocker locker(&mutex_);
        if (!audio_device_ || !audio_device_->isOpen() || paused_) {
            // Jeśli nie powinniśmy pisać, wyjdź (mutex zostanie zwolniony)
            return;
        }

        // Oblicz rozmiar bufora wyjściowego
        const int out_samples = audio_frame->nb_samples;
        const int out_buffer_size = out_samples * 2 * 2;
        auto out_buffer = static_cast<uint8_t *>(av_malloc(out_buffer_size));
        if (!out_buffer) return; // Wyjdź, jeśli alokacja się nie powiedzie

        const int converted_samples = swr_convert(
            swr_context_,
            &out_buffer, out_samples,
            audio_frame->extended_data, audio_frame->nb_samples
        );

        if (converted_samples > 0) {
            const int actual_size = converted_samples * 2 * 2;
            // <<< ZMIANA: Skopiuj dane do lokalnej QByteArray ZANIM zwolnisz mutex >>>
            buffer_to_write = QByteArray(reinterpret_cast<char *>(out_buffer), actual_size);
        }

        av_freep(&out_buffer);
    } // <<< Koniec bloku dla mutexa - mutex jest zwalniany >>>


        // Dodatkowe sprawdzenie, czy nie jesteśmy w pauzie, która mogła zostać ustawiona
        // między zwolnieniem mutexa a tym miejscem (choć mało prawdopodobne)
        QMutexLocker pause_check_locker(&mutex_);
        pause_check_locker.unlock();


            audio_device_->write(buffer_to_write);


}

int AudioDecoder::ReadPacket(void *opaque, uint8_t *buf, const int buf_size) {
    const auto decoder = static_cast<AudioDecoder*>(opaque);
    const int size = qMin(buf_size, decoder->read_position_ >= decoder->audio_data_.size() ?
                                  0 : decoder->audio_data_.size() - decoder->read_position_);

    if (size <= 0)
        return AVERROR_EOF;

    memcpy(buf, decoder->audio_data_.constData() + decoder->read_position_, size);
    decoder->read_position_ += size;
    return size;
}

int64_t AudioDecoder::SeekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto decoder = static_cast<AudioDecoder*>(opaque);

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
