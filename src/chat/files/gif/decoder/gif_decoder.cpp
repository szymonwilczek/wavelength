#include "gif_decoder.h"

GifDecoder::GifDecoder(const QByteArray &gifData, QObject *parent): QThread(parent), m_gifData(gifData), m_stopped(false) {
    // Zainicjuj domyślne wartości
    m_formatContext = nullptr;
    m_codecContext = nullptr;
    m_swsContext = nullptr;
    m_frame = nullptr;
    m_frameRGB = nullptr;
    m_gifStream = -1;
    m_buffer = nullptr;
    m_bufferSize = 0;
    m_paused = true;
    m_seeking = false;
    m_currentPosition = 0;
    m_reachedEndOfStream = false;
    m_initialized = false;
    m_frameDelay = 100; // Domyślne opóźnienie w ms

    // Zainicjuj bufor z danymi GIF
    m_ioBuffer = (unsigned char*)av_malloc(m_gifData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!m_ioBuffer) {
        emit error("Nie można zaalokować pamięci dla danych GIF");
        return;
    }
    memcpy(m_ioBuffer, m_gifData.constData(), m_gifData.size());

    m_ioContext = avio_alloc_context(
        m_ioBuffer, m_gifData.size(), 0, this,
        &GifDecoder::readPacket, nullptr, &GifDecoder::seekPacket
    );
    if (!m_ioContext) {
        av_free(m_ioBuffer);
        m_ioBuffer = nullptr;
        emit error("Nie można utworzyć kontekstu I/O");
        return;
    }
}

GifDecoder::~GifDecoder() {
    // Zatrzymaj wątek
    stop();
    wait(500); // Daj mu trochę czasu na zakończenie

    // Zwolnij zasoby FFmpeg i inne zasoby
    releaseResources();

    // Zwolnij zasoby IO, które są trzymane oddzielnie
    if (m_ioContext) {
        if (m_ioContext->buffer) {
            av_free(m_ioContext->buffer);
            m_ioContext->buffer = nullptr;
        }
        avio_context_free(&m_ioContext);
        m_ioContext = nullptr;
    } else if (m_ioBuffer) {
        av_free(m_ioBuffer);
        m_ioBuffer = nullptr;
    }
}

void GifDecoder::releaseResources() {
    QMutexLocker locker(&m_mutex);

    if (m_buffer) {
        av_free(m_buffer);
        m_buffer = nullptr;
    }

    if (m_frameRGB) {
        av_frame_free(&m_frameRGB);
        m_frameRGB = nullptr;
    }

    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_codecContext) {
        avcodec_close(m_codecContext);
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_formatContext) {
        if (m_formatContext->pb == m_ioContext)
            m_formatContext->pb = nullptr;
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    m_initialized = false;
}

bool GifDecoder::reinitialize() {
    QMutexLocker locker(&m_mutex);

    // Zatrzymaj wątek jeśli działa
    bool wasRunning = isRunning();
    if (wasRunning) {
        m_stopped = true;
        m_waitCondition.wakeAll();
        locker.unlock();
        wait(500); // Czekaj max 500ms
        locker.relock();
    }

    // Zwolnij wszystkie zasoby
    releaseResources();

    // Zresetuj stany
    m_stopped = false;
    m_paused = true;
    m_seeking = false;
    m_currentPosition = 0;
    m_readPosition = 0;
    m_reachedEndOfStream = false;

    // Przygotowanie buforów I/O jeśli zostały zwolnione
    if (!m_ioBuffer) {
        m_ioBuffer = (unsigned char*)av_malloc(m_gifData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!m_ioBuffer) {
            emit error("Nie można zaalokować pamięci dla danych GIF");
            return false;
        }
        memcpy(m_ioBuffer, m_gifData.constData(), m_gifData.size());
    }

    if (!m_ioContext) {
        m_ioContext = avio_alloc_context(
            m_ioBuffer, m_gifData.size(), 0, this,
            &GifDecoder::readPacket, nullptr, &GifDecoder::seekPacket
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

bool GifDecoder::initialize() {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) return true;  // Nie inicjalizuj ponownie, jeśli już zainicjalizowano

    // Utwórz kontekst formatu
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) {
        emit error("Nie można utworzyć kontekstu formatu");
        return false;
    }

    m_formatContext->pb = m_ioContext;

    // Otwórz strumień GIF
    if (avformat_open_input(&m_formatContext, "", nullptr, nullptr) < 0) {
        emit error("Nie można otworzyć strumienia GIF");
        return false;
    }

    // Znajdź informacje o strumieniu
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        emit error("Nie można znaleźć informacji o strumieniu");
        return false;
    }

    // Znajdź strumień wideo (GIF)
    m_gifStream = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_gifStream = i;
            break;
        }
    }

    if (m_gifStream == -1) {
        emit error("Nie znaleziono strumienia GIF");
        return false;
    }

    // Ustal częstotliwość klatek
    AVStream* stream = m_formatContext->streams[m_gifStream];
    if (stream->avg_frame_rate.den && stream->avg_frame_rate.num) {
        m_frameRate = av_q2d(stream->avg_frame_rate);
        m_frameDelay = 1000.0 / m_frameRate; // W milisekundach
    } else if (stream->r_frame_rate.den && stream->r_frame_rate.num) {
        m_frameRate = av_q2d(stream->r_frame_rate);
        m_frameDelay = 1000.0 / m_frameRate; // W milisekundach
    } else {
        m_frameRate = 10; // Domyślna wartość, jeśli nie można wykryć
        m_frameDelay = 100; // 100 ms = 10 FPS
    }

    qDebug() << "Wykryta częstotliwość klatek GIF:" << m_frameRate << "FPS, opóźnienie:" << m_frameDelay << "ms";

    // Znajdź dekoder
    const AVCodec* codec = avcodec_find_decoder(m_formatContext->streams[m_gifStream]->codecpar->codec_id);
    if (!codec) {
        emit error("Nie znaleziono dekodera dla formatu GIF");
        return false;
    }

    // Utwórz kontekst kodeka
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        emit error("Nie można utworzyć kontekstu kodeka");
        return false;
    }

    // Skopiuj parametry kodeka
    if (avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[m_gifStream]->codecpar) < 0) {
        emit error("Nie można skopiować parametrów kodeka");
        return false;
    }

    // Otwórz kodek
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        emit error("Nie można otworzyć kodeka");
        return false;
    }

    // Alokuj ramki
    m_frame = av_frame_alloc();
    m_frameRGB = av_frame_alloc();

    if (!m_frame || !m_frameRGB) {
        emit error("Nie można zaalokować ramek GIF");
        return false;
    }

    // Określ rozmiar bufora dla skonwertowanych ramek
    m_bufferSize = av_image_get_buffer_size(
        AV_PIX_FMT_RGBA,
        m_codecContext->width,
        m_codecContext->height,
        1
    );

    m_buffer = (uint8_t*)av_malloc(m_bufferSize);
    if (!m_buffer) {
        emit error("Nie można zaalokować bufora GIF");
        return false;
    }

    // Ustaw dane wyjściowe w frameRGB
    av_image_fill_arrays(
        m_frameRGB->data, m_frameRGB->linesize, m_buffer,
        AV_PIX_FMT_RGBA, m_codecContext->width, m_codecContext->height, 1
    );

    // Utwórz kontekst konwersji
    // Używamy RGBA, aby obsłużyć przezroczystość GIF-ów
    m_swsContext = sws_getContext(
        m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
        m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!m_swsContext) {
        emit error("Nie można utworzyć kontekstu konwersji");
        return false;
    }

    // Wyemituj informacje o GIF-ie
    emit gifInfo(
        m_codecContext->width,
        m_codecContext->height,
        m_formatContext->duration / AV_TIME_BASE,
        m_frameRate,
        m_formatContext->nb_streams  // Liczba strumieni (powinno być tylko 1 dla GIF)
    );

    extractAndEmitFirstFrameInternal(); // Wywołaj wewnętrzną metodę ekstrakcji

    m_initialized = true;
    m_paused = true; // Zacznij w stanie spauzowanym
    m_stopped = false; // Upewnij się, że nie jest zatrzymany
    return true;
}

void GifDecoder::stop() {
    QMutexLocker locker(&m_mutex);
    m_stopped = true;
    m_paused = false; // Aby pętla mogła sprawdzić m_stopped
    locker.unlock();
    m_waitCondition.wakeAll(); // Obudź, aby zakończyć
}

void GifDecoder::pause() {
    QMutexLocker locker(&m_mutex);
    if (!m_stopped) { // Nie pauzuj, jeśli jest zatrzymany
        m_paused = true;
        qDebug() << "GifDecoder::pause() - Pausing playback.";
    }
}

void GifDecoder::seek(double position) {
    QMutexLocker locker(&m_mutex);

    if (!m_formatContext || m_gifStream < 0) return;

    int64_t timestamp = position * AV_TIME_BASE;

    // Bezpieczne ograniczenie pozycji
    if (timestamp < 0) timestamp = 0;
    if (timestamp > m_formatContext->duration)
        timestamp = m_formatContext->duration > 0 ? m_formatContext->duration - 1 : 0;

    timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q,
                             m_formatContext->streams[m_gifStream]->time_base);

    m_seekPosition = timestamp;
    m_seeking = true;
    locker.unlock();

    if (m_paused)
        m_waitCondition.wakeOne();
}

void GifDecoder::reset() {
    QMutexLocker locker(&m_mutex);

    // Resetuj pozycję strumienia na początek
    if (m_formatContext && m_gifStream >= 0) {
        av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
        if (m_codecContext) {
            avcodec_flush_buffers(m_codecContext);
        }
        m_currentPosition = 0;
        emit positionChanged(0);
    }

    // Resetuj flagi stanu
    m_seeking = false;
    m_seekPosition = 0;
    m_reachedEndOfStream = false;
}

void GifDecoder::resume() {
    QMutexLocker locker(&m_mutex);
    if (m_stopped) return; // Nie wznawiaj, jeśli zatrzymany

    if (m_paused) {
        m_paused = false;
        qDebug() << "GifDecoder::resume() - Resuming playback.";
        if (!isRunning()) { // Jeśli wątek nie działa, uruchom go
            qDebug() << "GifDecoder::resume() - Thread not running, starting...";
            locker.unlock(); // Odblokuj przed startem wątku
            start(); // Uruchom pętlę run()
            locker.relock(); // Zablokuj ponownie
        } else {
            qDebug() << "GifDecoder::resume() - Waking up paused thread.";
            m_waitCondition.wakeAll(); // Obudź wątek czekający w run()
        }
    } else {
        qDebug() << "GifDecoder::resume() - Already running.";
    }
}

void GifDecoder::run() {
    qDebug() << "GifDecoder::run() - Starting thread..."; // Log startu wątku
    if (!initialize()) {
        qDebug() << "GifDecoder::run() - Initialization failed. Exiting thread.";
        return;
    }
    qDebug() << "GifDecoder::run() - Initialization successful.";

    AVPacket packet;
    int frameCounter = 0; // Licznik wyemitowanych klatek
    m_frameTimer.start();
    bool isFirstFrameAfterResume = true;

    while (true) {
        { // Ograniczenie zakresu lockera
            QMutexLocker locker(&m_mutex);
            if (m_stopped) {
                qDebug() << "GifDecoder::run() - Stopped flag set, exiting loop.";
                break; // Wyjdź z pętli, jeśli zatrzymano
            }
            if (m_paused) {
                qDebug() << "GifDecoder::run() - Paused, waiting...";
                m_waitCondition.wait(&m_mutex);
                qDebug() << "GifDecoder::run() - Woken up.";
                // Po obudzeniu pętla sprawdzi m_stopped i m_paused ponownie
                if (!m_paused && !m_stopped) { // Jeśli wznowiono
                    m_frameTimer.restart(); // Zresetuj timer klatki
                    isFirstFrameAfterResume = true; // Traktuj następną klatkę jako pierwszą po wznowieniu
                }
                continue; // Wróć na początek pętli
            }
            // Jeśli nie jest zatrzymany i nie jest spauzowany, kontynuuj dekodowanie
            if (m_seeking) {
                qDebug() << "GifDecoder::run() - Seeking to position:" << m_seekPosition;
                if (m_formatContext && m_gifStream >= 0) {
                    // Użyj AVSEEK_FLAG_ANY dla GIFów, może być bardziej niezawodne
                    int seek_ret = av_seek_frame(m_formatContext, m_gifStream, m_seekPosition, AVSEEK_FLAG_BACKWARD); // lub AVSEEK_FLAG_ANY
                    if (seek_ret < 0) {
                        qDebug() << "GifDecoder::run() - Seek failed:" << seek_ret;
                    } else {
                        qDebug() << "GifDecoder::run() - Seek successful.";
                        if (m_codecContext) {
                            avcodec_flush_buffers(m_codecContext); // Wyczyszczenie buforów po szukaniu
                            qDebug() << "GifDecoder::run() - Codec buffers flushed.";
                        }
                    }
                    m_seeking = false;
                    m_frameTimer.restart();
                    isFirstFrameAfterResume = true;

                    // Aktualizacja pozycji po przeszukiwaniu
                    m_currentPosition = m_seekPosition * av_q2d(m_formatContext->streams[m_gifStream]->time_base);
                    locker.unlock(); // Odblokuj przed emisją sygnału
                    emit positionChanged(m_currentPosition);
                    locker.relock(); // Zablokuj ponownie (chociaż nie jest to ściśle konieczne przed continue)
                } else {
                    m_seeking = false; // Nie można szukać, zresetuj flagę
                }
                continue; // Wróć na początek pętli po seek
            }
        } // Koniec zakresu lockera

        int readResult = av_read_frame(m_formatContext, &packet);

        if (readResult < 0) {
            if (readResult == AVERROR_EOF) {
                // Koniec strumienia - zapętlanie
                QMutexLocker locker(&m_mutex);
                if (m_stopped) break; // Sprawdź ponownie przed seek
                qDebug() << "GifDecoder::run() - End of stream, looping...";
                int seek_ret = av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
                if (seek_ret < 0) {
                    qDebug() << "GifDecoder::run() - Loop seek failed:" << seek_ret;
                    m_stopped = true; // Zatrzymaj, jeśli przewinięcie się nie powiodło
                    break;
                } else {
                    if (m_codecContext) avcodec_flush_buffers(m_codecContext);
                    m_currentPosition = 0;
                    m_frameTimer.restart(); // Reset timera przy zapętleniu
                    isFirstFrameAfterResume = true;
                    locker.unlock();
                    emit positionChanged(0);
                    locker.relock();
                }
                av_packet_unref(&packet);
                continue;
            } else {
                // Inny błąd odczytu
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, readResult);
                qDebug() << "GifDecoder::run() - Error reading frame:" << readResult << errbuf;
                m_stopped = true; // Zatrzymaj wątek w przypadku błędu
                break; // Wyjdź z pętli
            }
        }

        // --- Przetwarzanie pakietu ---
        if (packet.stream_index == m_gifStream) {
            int sendResult = avcodec_send_packet(m_codecContext, &packet);
            if (sendResult < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, sendResult);
                qDebug() << "GifDecoder::run() - Error sending packet to decoder:" << sendResult << errbuf;
                // Można spróbować kontynuować lub zatrzymać, zależnie od błędu
                // if (sendResult != AVERROR(EAGAIN) && sendResult != AVERROR_EOF) {
                //     m_stopped = true;
                // }
            } else {
                // Odbieranie klatek (może być więcej niż jedna na pakiet)
                while (true) {
                    // qDebug() << "GifDecoder::run() - Receiving frame from decoder...";
                    int receiveResult = avcodec_receive_frame(m_codecContext, m_frame);
                    // qDebug() << "GifDecoder::run() - avcodec_receive_frame result:" << receiveResult;

                    if (receiveResult == 0) {
                        // SUKCES - Mamy klatkę!
                        qDebug() << "GifDecoder::run() - Frame received successfully! Index:" << frameCounter;
                        frameCounter++;

                        // Aktualizacja pozycji (przeniesiono tutaj)
                        if (m_frame->pts != AV_NOPTS_VALUE) {
                            QMutexLocker posLocker(&m_mutex);
                            m_currentPosition = m_frame->pts * av_q2d(m_formatContext->streams[m_gifStream]->time_base);
                            posLocker.unlock();
                            emit positionChanged(m_currentPosition);
                        }

                        // Konwersja klatki
                        sws_scale(
                            m_swsContext,
                            (const uint8_t* const*)m_frame->data, m_frame->linesize, 0, m_codecContext->height,
                            m_frameRGB->data, m_frameRGB->linesize
                        );

                        // Tworzenie QImage
                        QImage frameImage( // Zmieniono nazwę zmiennej, aby uniknąć konfliktu z m_frame
                            m_frameRGB->data[0],
                            m_codecContext->width,
                            m_codecContext->height,
                            m_frameRGB->linesize[0],
                            QImage::Format_RGBA8888 // Użyj RGBA dla przezroczystości
                        );

                        // EMISJA SYGNAŁU
                        qDebug() << "GifDecoder::run() - Emitting frameReady for frame index:" << (frameCounter - 1);
                        emit frameReady(frameImage.copy()); // Emituj kopię

                        // Opóźnienie
                        qint64 delayTime = static_cast<qint64>(m_frameDelay); // Użyj m_frameDelay
                        if (!isFirstFrameAfterResume) { // Użyj nowej flagi
                            qint64 elapsed = m_frameTimer.elapsed();
                            delayTime = qMax((qint64)0, delayTime - elapsed);
                        }
                        if (delayTime > 0) {
                            // Użyj usleep lub innego mechanizmu czekania, który można przerwać
                            // QThread::msleep jest prosty, ale może nie być idealny do szybkiego wznawiania
                            msleep(delayTime); // Użyj msleep z QThread
                        }
                        m_frameTimer.restart();
                        isFirstFrameAfterResume = false; // Zresetuj flagę

                    } else if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
                        // EAGAIN: Potrzeba więcej danych (następny pakiet)
                        // EOF: Dekoder został opróżniony
                        // qDebug() << "GifDecoder::run() - Decoder needs more data or is flushed. Breaking receive loop.";
                        break; // Wyjdź z pętli odbierania klatek
                    } else {
                        // Inny błąd odbierania
                        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, receiveResult);
                        qDebug() << "GifDecoder::run() - Error receiving frame from decoder:" << receiveResult << errbuf;
                        m_stopped = true; // Poważny błąd, zatrzymaj wątek
                        break; // Wyjdź z pętli odbierania
                    }
                } // Koniec pętli odbierania klatek
            }
        } else {
            // Pakiet nie należy do naszego strumienia, ignoruj
        }

        av_packet_unref(&packet); // Zwolnij pakiet

        if (m_stopped) break; // Sprawdź flagę zatrzymania po przetworzeniu pakietu

    } // Koniec głównej pętli while

    qDebug() << "GifDecoder::run() - Exiting thread loop. Total frames emitted:" << frameCounter;
    // Sygnał zakończenia można by tu emitować, jeśli nie zapętlamy w nieskończoność
    // emit playbackFinished();
}

int GifDecoder::readPacket(void *opaque, uint8_t *buf, int buf_size) {
    GifDecoder* decoder = static_cast<GifDecoder*>(opaque);
    int size = qMin(buf_size, decoder->m_readPosition >= decoder->m_gifData.size() ?
                                  0 : decoder->m_gifData.size() - decoder->m_readPosition);

    if (size <= 0)
        return AVERROR_EOF;

    memcpy(buf, decoder->m_gifData.constData() + decoder->m_readPosition, size);
    decoder->m_readPosition += size;
    return size;
}

int64_t GifDecoder::seekPacket(void *opaque, int64_t offset, int whence) {
    GifDecoder* decoder = static_cast<GifDecoder*>(opaque);

    switch (whence) {
        case SEEK_SET:
            decoder->m_readPosition = offset;
            break;
        case SEEK_CUR:
            decoder->m_readPosition += offset;
            break;
        case SEEK_END:
            decoder->m_readPosition = decoder->m_gifData.size() + offset;
            break;
        case AVSEEK_SIZE:
            return decoder->m_gifData.size();
        default:
            return -1;
    }

    return decoder->m_readPosition;
}

void GifDecoder::extractAndEmitFirstFrameInternal() {
    if (!m_formatContext || m_gifStream < 0 || !m_codecContext || !m_swsContext || !m_frame || !m_frameRGB) {
        qWarning() << "GifDecoder: Cannot extract first frame, context not ready.";
        return;
    }

    // Przewiń na początek (może nie być konieczne, jeśli initialize jest pierwsze)
    av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_codecContext);
    m_readPosition = 0; // Resetuj pozycję odczytu dla AVIOContext

    AVPacket packet;
    av_init_packet(&packet); // Zainicjuj pakiet
    packet.data = nullptr;
    packet.size = 0;

    bool frame_decoded = false;
    while (av_read_frame(m_formatContext, &packet) >= 0 && !frame_decoded) {
        if (packet.stream_index == m_gifStream) {
            if (avcodec_send_packet(m_codecContext, &packet) >= 0) {
                if (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
                    // Konwertuj ramkę do RGBA
                    sws_scale(
                        m_swsContext,
                        (const uint8_t* const*)m_frame->data, m_frame->linesize, 0, m_codecContext->height,
                        m_frameRGB->data, m_frameRGB->linesize
                    );

                    // Utwórz QImage
                    QImage firstFrame(
                        m_frameRGB->data[0],
                        m_codecContext->width,
                        m_codecContext->height,
                        m_frameRGB->linesize[0],
                        QImage::Format_RGBA8888
                    );

                    qDebug() << "GifDecoder: First frame extracted successfully.";
                    emit firstFrameReady(firstFrame.copy()); // Emituj kopię
                    frame_decoded = true;
                }
                // Ignoruj błędy EAGAIN/EOF przy odbieraniu pierwszej klatki
            }
            // Ignoruj błędy wysyłania przy pierwszej klatce (np. potrzeba więcej danych)
        }
        av_packet_unref(&packet); // Zwolnij pakiet wewnątrz pętli
    }
    av_packet_unref(&packet); // Upewnij się, że jest zwolniony po pętli

    // Przewiń z powrotem na początek, aby run() zaczął od początku
    av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_codecContext);
    m_readPosition = 0; // Resetuj pozycję odczytu ponownie

    if (!frame_decoded) {
        qWarning() << "GifDecoder: Failed to extract the first frame.";
        // Można wyemitować pusty obraz lub sygnał błędu
        emit firstFrameReady(QImage());
    }
}
