#include "audio_decoder.h"

extern "C" {
    #include <libavutil/opt.h>
}



AudioDecoder::AudioDecoder(const QByteArray &audioData, QObject *parent): QThread(parent), m_audioData(audioData), m_stopped(false) {
    // Zainicjuj domyślne wartości
    m_formatContext = nullptr;
    m_audioCodecContext = nullptr;
    m_swrContext = nullptr;
    m_audioFrame = nullptr;
    m_audioStream = -1;
    m_readPosition = 0;
    m_audioOutput = nullptr;
    m_audioDevice = nullptr;
    m_paused = true;
    m_reachedEndOfStream = false;
    m_seeking = false;
    m_currentPosition = 0;
    m_initialized = false;

    // Zainicjuj bufor z danymi audio
    m_ioBuffer = static_cast<unsigned char *>(av_malloc(m_audioData.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!m_ioBuffer) {
        emit error("Nie można zaalokować pamięci dla danych audio");
        return;
    }
    memcpy(m_ioBuffer, m_audioData.constData(), m_audioData.size());

    m_ioContext = avio_alloc_context(
        m_ioBuffer, m_audioData.size(), 0, this,
        &AudioDecoder::readPacket, nullptr, &AudioDecoder::seekPacket
    );
    if (!m_ioContext) {
        av_free(m_ioBuffer);
        m_ioBuffer = nullptr;
        emit error("Nie można utworzyć kontekstu I/O");
        return;
    }
}

AudioDecoder::~AudioDecoder() {
    // Upewnij się, że wszystkie zasoby są zwolnione
    stop();
    wait(); // Poczekaj aż wątek się zakończy przed zwolnieniem zasobów
    releaseResources();
}

void AudioDecoder::releaseResources() {
    QMutexLocker locker(&m_mutex);

    // Zatrzymaj odtwarzanie audio
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
        m_audioDevice = nullptr;
    }

    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }

    if (m_audioCodecContext) {
        avcodec_close(m_audioCodecContext);
        avcodec_free_context(&m_audioCodecContext);
        m_audioCodecContext = nullptr;
    }

    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }

    if (m_formatContext) {
        if (m_formatContext->pb == m_ioContext)
            m_formatContext->pb = nullptr;
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    // Nie zwalniamy bufora IO od razu, tylko przy zniszczeniu dekodera lub reinicjalizacji
    m_initialized = false;
}

bool AudioDecoder::reinitialize() {
    QMutexLocker locker(&m_mutex);

    // Najpierw uwolnij istniejące zasoby
    releaseResources();

    // Zresetuj stany
    m_stopped = false;
    m_paused = true;
    m_reachedEndOfStream = false;
    m_seeking = false;
    m_currentPosition = 0;
    m_readPosition = 0;

    // Ponownie utwórz bufor IO jeśli został zwolniony
    if (!m_ioBuffer) {
        m_ioBuffer = static_cast<unsigned char *>(av_malloc(m_audioData.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!m_ioBuffer) {
            emit error("Nie można zaalokować pamięci dla danych audio");
            return false;
        }
        memcpy(m_ioBuffer, m_audioData.constData(), m_audioData.size());
    }

    if (!m_ioContext) {
        m_ioContext = avio_alloc_context(
            m_ioBuffer, m_audioData.size(), 0, this,
            &AudioDecoder::readPacket, nullptr, &AudioDecoder::seekPacket
        );
        if (!m_ioContext) {
            av_free(m_ioBuffer);
            m_ioBuffer = nullptr;
            emit error("Nie można utworzyć kontekstu I/O");
            return false;
        }
    }

    return true;
}

void AudioDecoder::setVolume(const float volume) const {
    QMutexLocker locker(&m_mutex);
    if (m_audioOutput) {
        m_audioOutput->setVolume(volume);
    }
}

float AudioDecoder::getVolume() const {
    QMutexLocker locker(&m_mutex);
    return m_audioOutput ? m_audioOutput->volume() : 0.0f;
}

bool AudioDecoder::initialize() {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) return true;  // Nie inicjalizuj ponownie, jeśli już zainicjalizowano

    // Utwórz kontekst formatu
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) {
        emit error("Nie można utworzyć kontekstu formatu");
        return false;
    }

    m_formatContext->pb = m_ioContext;

    // Otwórz strumień audio
    if (avformat_open_input(&m_formatContext, "", nullptr, nullptr) < 0) {
        emit error("Nie można otworzyć strumienia audio");
        return false;
    }

    // Znajdź informacje o strumieniu
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        emit error("Nie można znaleźć informacji o strumieniu");
        return false;
    }

    // Znajdź pierwszy strumień audio
    m_audioStream = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStream = i;
            break;
        }
    }

    if (m_audioStream == -1) {
        emit error("Nie znaleziono strumienia audio");
        return false;
    }

    // Znajdź dekoder audio
    const AVCodec* audioCodec = avcodec_find_decoder(
        m_formatContext->streams[m_audioStream]->codecpar->codec_id
    );
    if (!audioCodec) {
        emit error("Nie znaleziono dekodera dla tego formatu audio");
        return false;
    }

    // Utwórz kontekst kodeka audio
    m_audioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!m_audioCodecContext) {
        emit error("Nie można utworzyć kontekstu kodeka audio");
        return false;
    }

    // Skopiuj parametry kodeka audio
    if (avcodec_parameters_to_context(
            m_audioCodecContext,
            m_formatContext->streams[m_audioStream]->codecpar
        ) < 0) {
        emit error("Nie można skopiować parametrów kodeka audio");
        return false;
    }

    // Otwórz kodek audio
    if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) < 0) {
        emit error("Nie można otworzyć kodeka audio");
        return false;
    }

    // Alokuj ramkę audio
    m_audioFrame = av_frame_alloc();
    if (!m_audioFrame) {
        emit error("Nie można zaalokować ramki audio");
        return false;
    }

    // Utwórz kontekst resamplingu
    m_swrContext = swr_alloc();
    if (!m_swrContext) {
        emit error("Nie można utworzyć kontekstu resamplingu audio");
        return false;
    }

    // Inicjalizacja layoutu kanałów wyjściowych (stereo)
    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, 2); // 2 kanały (stereo)

    // Konfiguracja resamplingu do formatu PCM S16LE stereo 44.1kHz
    av_opt_set_chlayout(m_swrContext, "in_chlayout", &m_audioCodecContext->ch_layout, 0);
    av_opt_set_chlayout(m_swrContext, "out_chlayout", &outLayout, 0);
    av_opt_set_int(m_swrContext, "in_sample_rate", m_audioCodecContext->sample_rate, 0);
    av_opt_set_int(m_swrContext, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", m_audioCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(m_swrContext, "resampler_quality", 7, 0);

    if (swr_init(m_swrContext) < 0) {
        emit error("Nie można zainicjalizować kontekstu resamplingu audio");
        av_channel_layout_uninit(&outLayout);
        return false;
    }

    // Zwolnij pamięć layoutu kanałów wyjściowych
    av_channel_layout_uninit(&outLayout);

    // Konfiguracja formatu audio dla Qt
    m_audioFormat.setSampleRate(44100);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");

    // Inicjalizacja wyjścia audio
    m_audioOutput = new QAudioOutput(m_audioFormat, nullptr);
    m_audioOutput->setVolume(1.0);
    constexpr int suggestedBufferSize = 44100 * 2 * 2 / 2;
    m_audioOutput->setBufferSize(qMax(suggestedBufferSize, m_audioOutput->bufferSize()));
    m_audioDevice = m_audioOutput->start();

    if (!m_audioDevice || !m_audioDevice->isOpen()) {
        emit error("Nie można otworzyć urządzenia audio");
        return false;
    }

    // Wyemituj informacje o audio
    const double duration = m_formatContext->duration / static_cast<double>(AV_TIME_BASE);
    emit audioInfo(m_audioCodecContext->sample_rate, m_audioCodecContext->ch_layout.nb_channels, duration);

    m_initialized = true;
    return true;
}

void AudioDecoder::stop() {
    QMutexLocker locker(&m_mutex);
    m_stopped = true;
    locker.unlock();
    m_waitCondition.wakeOne();
}

void AudioDecoder::pause() {
    QMutexLocker locker(&m_mutex);
    m_paused = !m_paused;
    locker.unlock();

    if (!m_paused)
        m_waitCondition.wakeOne();
}

bool AudioDecoder::isPaused() const {
    QMutexLocker locker(&m_mutex);
    return m_paused;
}

void AudioDecoder::seek(const double position) {
    QMutexLocker locker(&m_mutex);

    if (!m_formatContext || m_audioStream < 0) return;

    int64_t timestamp = position * AV_TIME_BASE;

    // Bezpieczne ograniczenie pozycji
    if (timestamp < 0) timestamp = 0;
    if (timestamp > m_formatContext->duration)
        timestamp = m_formatContext->duration > 0 ? m_formatContext->duration - 1 : 0;

    timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q,
                             m_formatContext->streams[m_audioStream]->time_base);

    m_seekPosition = timestamp;
    m_seeking = true;
    locker.unlock();

    if (m_paused)
        m_waitCondition.wakeOne();
}

void AudioDecoder::reset() {
    QMutexLocker locker(&m_mutex);

    // Resetuj pozycję strumienia na początek
    if (m_formatContext && m_audioStream >= 0) {
        av_seek_frame(m_formatContext, m_audioStream, 0, AVSEEK_FLAG_BACKWARD);
        if (m_audioCodecContext) {
            avcodec_flush_buffers(m_audioCodecContext);
        }
        m_currentPosition = 0;
        emit positionChanged(0);

        // Upewniamy się, że urządzenie audio jest gotowe
        if (m_audioOutput) {
            m_audioOutput->reset();
            m_audioDevice = m_audioOutput->start();
        }
    }

    // Resetuj flagi stanu
    m_seeking = false;
    m_seekPosition = 0;
    m_reachedEndOfStream = false;
}

void AudioDecoder::run() {
    if (!initialize()) {
        return;
    }

    AVPacket packet;
    QElapsedTimer positionTimer;
    positionTimer.start();

    // Potrzebna zmienna do śledzenia poprzedniej pozycji
    double lastEmittedPosition = 0;
    constexpr int positionUpdateInterval = 250; // ms

    // Limiter tempa odtwarzania
    // QElapsedTimer playbackTimer;
    // playbackTimer.start();

    // Ile próbek zostało przetworzone
    // int64_t samplesProcessed = 0;
    // int64_t lastReportedSamples = 0;

    while (!m_stopped) {
        QMutexLocker locker(&m_mutex);

        if (m_paused) {
            m_waitCondition.wait(&m_mutex);
            locker.unlock();

            continue;
        }

        if (m_seeking) {
            if (m_formatContext && m_audioStream >= 0) {
                av_seek_frame(m_formatContext, m_audioStream, m_seekPosition, AVSEEK_FLAG_BACKWARD);
                if (m_audioCodecContext) {
                    avcodec_flush_buffers(m_audioCodecContext);
                }
                m_seeking = false;
                positionTimer.restart();
                // playbackTimer.restart();

                // Aktualizacja pozycji po przeszukiwaniu
                m_currentPosition = m_seekPosition * av_q2d(m_formatContext->streams[m_audioStream]->time_base);
                emit positionChanged(m_currentPosition);

                lastEmittedPosition = m_currentPosition;
                // samplesProcessed = 0;
                // lastReportedSamples = 0;

                // Reset bufora audio
                if (m_audioOutput) {
                    m_audioOutput->reset();
                    m_audioDevice = m_audioOutput->start();
                }
            }
        }
        locker.unlock();

        // Sprawdź czy kontekst formatu jest nadal ważny
        if (!m_formatContext) break;

        // Czytaj kolejny pakiet
        const int readResult = av_read_frame(m_formatContext, &packet);
        if (readResult < 0) {
            // Koniec strumienia
            emit playbackFinished();
            QMutexLocker locker2(&m_mutex);
            m_paused = true;
            m_reachedEndOfStream = true;
            continue;
        }

        if (packet.stream_index == m_audioStream && m_audioCodecContext) {
            avcodec_send_packet(m_audioCodecContext, &packet);
            while (avcodec_receive_frame(m_audioCodecContext, m_audioFrame) == 0) {
                // Aktualizacja pozycji odtwarzania
                if (m_audioFrame->pts != AV_NOPTS_VALUE) {
                    QMutexLocker posLocker(&m_mutex);
                    m_currentPosition = m_audioFrame->pts *
                                        av_q2d(m_formatContext->streams[m_audioStream]->time_base);
                    posLocker.unlock();

                    // Wysyłamy powiadomienia o zmianie pozycji w kontrolowanych odstępach czasu
                    if (abs(m_currentPosition - lastEmittedPosition) > 0.25 ||
                        positionTimer.elapsed() > positionUpdateInterval) {
                        emit positionChanged(m_currentPosition);
                        lastEmittedPosition = m_currentPosition;
                        positionTimer.restart();
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
                decodeAudioFrame(m_audioFrame);

                // Aktualizujemy licznik i timer
                // if (samplesProcessed - lastReportedSamples > 44100) {  // co około 1 sekundę
                //     lastReportedSamples = samplesProcessed;
                //     playbackTimer.restart();
                // }

                {
                    QMutexLocker mutex_locker(&m_mutex);
                    if (m_audioOutput && m_audioDevice && m_audioDevice->isOpen()) {
                        // Poczekaj, jeśli mniej niż np. 1/4 bufora jest wolna
                        const int threshold = m_audioOutput->bufferSize() / 4;
                        while (m_audioOutput->bytesFree() < threshold && !m_paused && !m_stopped && !m_seeking) {
                            mutex_locker.unlock(); // Zwolnij mutex na czas czekania
                            QThread::msleep(10); // Krótka pauza, aby nie obciążać CPU
                            mutex_locker.relock();   // Zablokuj ponownie przed sprawdzeniem warunku

                        }
                    }
                } // Mutex zwolniony
            }
        }

        av_packet_unref(&packet);
    }
}

void AudioDecoder::decodeAudioFrame(const AVFrame *audioFrame) const {
    // <<< ZMIANA: Przeniesiono deklaracje na zewnątrz bloku mutexa >>>
    QByteArray bufferToWrite; { // <<< Początek bloku dla mutexa >>>
        QMutexLocker locker(&m_mutex);
        if (!m_audioDevice || !m_audioDevice->isOpen() || m_paused) {
            // Jeśli nie powinniśmy pisać, wyjdź (mutex zostanie zwolniony)
            return;
        }

        // Oblicz rozmiar bufora wyjściowego
        const int outSamples = audioFrame->nb_samples;
        const int outBufferSize = outSamples * 2 * 2;
        auto outBuffer = static_cast<uint8_t *>(av_malloc(outBufferSize));
        if (!outBuffer) return; // Wyjdź, jeśli alokacja się nie powiedzie

        const int convertedSamples = swr_convert(
            m_swrContext,
            &outBuffer, outSamples,
            audioFrame->extended_data, audioFrame->nb_samples
        );

        if (convertedSamples > 0) {
            const int actualSize = convertedSamples * 2 * 2;
            // <<< ZMIANA: Skopiuj dane do lokalnej QByteArray ZANIM zwolnisz mutex >>>
            bufferToWrite = QByteArray(reinterpret_cast<char *>(outBuffer), actualSize);
        }

        av_freep(&outBuffer);
    } // <<< Koniec bloku dla mutexa - mutex jest zwalniany >>>


        // Dodatkowe sprawdzenie, czy nie jesteśmy w pauzie, która mogła zostać ustawiona
        // między zwolnieniem mutexa a tym miejscem (choć mało prawdopodobne)
        QMutexLocker pauseCheckLocker(&m_mutex);
        pauseCheckLocker.unlock();


            m_audioDevice->write(bufferToWrite);


}

int AudioDecoder::readPacket(void *opaque, uint8_t *buf, const int buf_size) {
    const auto decoder = static_cast<AudioDecoder*>(opaque);
    const int size = qMin(buf_size, decoder->m_readPosition >= decoder->m_audioData.size() ?
                                  0 : decoder->m_audioData.size() - decoder->m_readPosition);

    if (size <= 0)
        return AVERROR_EOF;

    memcpy(buf, decoder->m_audioData.constData() + decoder->m_readPosition, size);
    decoder->m_readPosition += size;
    return size;
}

int64_t AudioDecoder::seekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto decoder = static_cast<AudioDecoder*>(opaque);

    switch (whence) {
        case SEEK_SET:
            decoder->m_readPosition = offset;
            break;
        case SEEK_CUR:
            decoder->m_readPosition += offset;
            break;
        case SEEK_END:
            decoder->m_readPosition = decoder->m_audioData.size() + offset;
            break;
        case AVSEEK_SIZE:
            return decoder->m_audioData.size();
        default:
            return -1;
    }

    return decoder->m_readPosition;
}
