#include "gif_decoder.h"

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
        emit error("Nie można zaalokować pamięci dla danych GIF");
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
        emit error("Nie można utworzyć kontekstu I/O");
        return;
    }
}

GifDecoder::~GifDecoder() {
    // Zatrzymaj wątek
    Stop();
    wait(500); // Daj mu trochę czasu na zakończenie

    // Zwolnij zasoby FFmpeg i inne zasoby
    ReleaseResources();

    // Zwolnij zasoby IO, które są trzymane oddzielnie
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
        avcodec_close(codec_context_);
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

    // Zatrzymaj wątek jeśli działa
    if (IsDecoderRunning()) {
        stopped_ = true;
        wait_condition_.wakeAll();
        locker.unlock();
        wait(500); // Czekaj max 500ms
        locker.relock();
    }

    // Zwolnij wszystkie zasoby
    ReleaseResources();

    // Zresetuj stany
    stopped_ = false;
    paused_ = true;
    current_position_ = 0;
    read_position_ = 0;
    reached_end_of_stream_ = false;

    // Przygotowanie buforów I/O jeśli zostały zwolnione
    if (!io_buffer_) {
        io_buffer_ = static_cast<unsigned char *>(av_malloc(gif_data_.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!io_buffer_) {
            emit error("Nie można zaalokować pamięci dla danych GIF");
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
            emit error("Nie można utworzyć kontekstu I/O");
            return false;
        }
    }

    return true;
}

bool GifDecoder::Initialize() {
    QMutexLocker locker(&mutex_);

    if (initialized_) return true;  // Nie inicjalizuj ponownie, jeśli już zainicjalizowano

    // Utwórz kontekst formatu
    format_context_ = avformat_alloc_context();
    if (!format_context_) {
        emit error("Nie można utworzyć kontekstu formatu");
        return false;
    }

    format_context_->pb = io_context_;

    // Otwórz strumień GIF
    if (avformat_open_input(&format_context_, "", nullptr, nullptr) < 0) {
        emit error("Nie można otworzyć strumienia GIF");
        return false;
    }

    // Znajdź informacje o strumieniu
    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit error("Nie można znaleźć informacji o strumieniu");
        return false;
    }

    // Znajdź strumień wideo (GIF)
    gif_stream_ = -1;
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            gif_stream_ = i;
            break;
        }
    }

    if (gif_stream_ == -1) {
        emit error("Nie znaleziono strumienia GIF");
        return false;
    }

    // Ustal częstotliwość klatek
    const AVStream* stream = format_context_->streams[gif_stream_];
    if (stream->avg_frame_rate.den && stream->avg_frame_rate.num) {
        frame_rate_ = av_q2d(stream->avg_frame_rate);
        frame_delay_ = 1000.0 / frame_rate_; // W milisekundach
    } else if (stream->r_frame_rate.den && stream->r_frame_rate.num) {
        frame_rate_ = av_q2d(stream->r_frame_rate);
        frame_delay_ = 1000.0 / frame_rate_; // W milisekundach
    } else {
        frame_rate_ = 10; // Domyślna wartość, jeśli nie można wykryć
        frame_delay_ = 100; // 100 ms = 10 FPS
    }

    qDebug() << "Wykryta częstotliwość klatek GIF:" << frame_rate_ << "FPS, opóźnienie:" << frame_delay_ << "ms";

    // Znajdź dekoder
    const AVCodec* codec = avcodec_find_decoder(format_context_->streams[gif_stream_]->codecpar->codec_id);
    if (!codec) {
        emit error("Nie znaleziono dekodera dla formatu GIF");
        return false;
    }

    // Utwórz kontekst kodeka
    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) {
        emit error("Nie można utworzyć kontekstu kodeka");
        return false;
    }

    // Skopiuj parametry kodeka
    if (avcodec_parameters_to_context(codec_context_, format_context_->streams[gif_stream_]->codecpar) < 0) {
        emit error("Nie można skopiować parametrów kodeka");
        return false;
    }

    // Otwórz kodek
    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        emit error("Nie można otworzyć kodeka");
        return false;
    }

    // Alokuj ramki
    frame_ = av_frame_alloc();
    frame_rgb_ = av_frame_alloc();

    if (!frame_ || !frame_rgb_) {
        emit error("Nie można zaalokować ramek GIF");
        return false;
    }

    // Określ rozmiar bufora dla skonwertowanych ramek
    buffer_size_ = av_image_get_buffer_size(
        AV_PIX_FMT_RGBA,
        codec_context_->width,
        codec_context_->height,
        1
    );

    buffer_ = static_cast<uint8_t *>(av_malloc(buffer_size_));
    if (!buffer_) {
        emit error("Nie można zaalokować bufora GIF");
        return false;
    }

    // Ustaw dane wyjściowe w frameRGB
    av_image_fill_arrays(
        frame_rgb_->data, frame_rgb_->linesize, buffer_,
        AV_PIX_FMT_RGBA, codec_context_->width, codec_context_->height, 1
    );

    // Utwórz kontekst konwersji
    // Używamy RGBA, aby obsłużyć przezroczystość GIF-ów
    sws_context_ = sws_getContext(
        codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
        codec_context_->width, codec_context_->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_context_) {
        emit error("Nie można utworzyć kontekstu konwersji");
        return false;
    }

    // Wyemituj informacje o GIF-ie
    emit gifInfo(
        codec_context_->width,
        codec_context_->height,
        format_context_->duration / AV_TIME_BASE,
        frame_rate_,
        format_context_->nb_streams  // Liczba strumieni (powinno być tylko 1 dla GIF)
    );

    ExtractAndEmitFirstFrameInternal(); // Wywołaj wewnętrzną metodę ekstrakcji

    initialized_ = true;
    paused_ = true; // Zacznij w stanie spauzowanym
    stopped_ = false; // Upewnij się, że nie jest zatrzymany
    return true;
}

void GifDecoder::Stop() {
    QMutexLocker locker(&mutex_);
    stopped_ = true;
    paused_ = false; // Aby pętla mogła sprawdzić m_stopped
    locker.unlock();
    wait_condition_.wakeAll(); // Obudź, aby zakończyć
}

void GifDecoder::Pause() {
    QMutexLocker locker(&mutex_);
    if (!stopped_) { // Nie pauzuj, jeśli jest zatrzymany
        paused_ = true;
        qDebug() << "GifDecoder::pause() - Pausing playback.";
    }
}

void GifDecoder::Reset() {
    QMutexLocker locker(&mutex_);

    // Resetuj pozycję strumienia na początek
    if (format_context_ && gif_stream_ >= 0) {
        av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
        if (codec_context_) {
            avcodec_flush_buffers(codec_context_);
        }
        current_position_ = 0;
        emit positionChanged(0);
    }

    // Resetuj flagi stanu
    seek_position_ = 0;
    reached_end_of_stream_ = false;
}

void GifDecoder::Resume() {
    QMutexLocker locker(&mutex_);
    if (stopped_) return; // Nie wznawiaj, jeśli zatrzymany

    if (paused_) {
        paused_ = false;
        qDebug() << "GifDecoder::resume() - Resuming playback.";
        if (!IsDecoderRunning()) { // Jeśli wątek nie działa, uruchom go
            qDebug() << "GifDecoder::resume() - Thread not running, starting...";
            locker.unlock(); // Odblokuj przed startem wątku
            start(); // Uruchom pętlę run()
            locker.relock(); // Zablokuj ponownie
        } else {
            qDebug() << "GifDecoder::resume() - Waking up paused thread.";
            wait_condition_.wakeAll(); // Obudź wątek czekający w run()
        }
    } else {
        qDebug() << "GifDecoder::resume() - Already running.";
    }
}

void GifDecoder::run() {
    qDebug() << "GifDecoder::run() - Starting thread..."; // Log startu wątku
    if (!Initialize()) {
        qDebug() << "GifDecoder::run() - Initialization failed. Exiting thread.";
        return;
    }
    qDebug() << "GifDecoder::run() - Initialization successful.";

    AVPacket packet;
    int frame_counter = 0; // Licznik wyemitowanych klatek
    frame_timer_.start();
    bool is_first_frame_after_resume = true;

    while (true) {
        { // Ograniczenie zakresu lockera
            QMutexLocker locker(&mutex_);
            if (stopped_) {
                qDebug() << "GifDecoder::run() - Stopped flag set, exiting loop.";
                break; // Wyjdź z pętli, jeśli zatrzymano
            }
            if (paused_) {
                qDebug() << "GifDecoder::run() - Paused, waiting...";
                wait_condition_.wait(&mutex_);
                qDebug() << "GifDecoder::run() - Woken up.";
                continue; // Wróć na początek pętli
            }
        } // Koniec zakresu lockera

        const int read_result = av_read_frame(format_context_, &packet);

        if (read_result < 0) {
            if (read_result == AVERROR_EOF) {
                // Koniec strumienia - zapętlanie
                QMutexLocker locker(&mutex_);
                qDebug() << "GifDecoder::run() - End of stream, looping...";
                const int seek_ret = av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
                if (seek_ret < 0) {
                    qDebug() << "GifDecoder::run() - Loop seek failed:" << seek_ret;
                    stopped_ = true; // Zatrzymaj, jeśli przewinięcie się nie powiodło
                    break;
                }

                if (codec_context_) avcodec_flush_buffers(codec_context_);
                current_position_ = 0;
                frame_timer_.restart(); // Reset timera przy zapętleniu
                is_first_frame_after_resume = true;
                locker.unlock();
                emit positionChanged(0);
                locker.relock();
                av_packet_unref(&packet);
                continue;
            }
            // Inny błąd odczytu
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = {};
            av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, read_result);
            qDebug() << "GifDecoder::run() - Error reading frame:" << read_result << errbuf;
            stopped_ = true; // Zatrzymaj wątek w przypadku błędu
            break; // Wyjdź z pętli
        }

        // --- Przetwarzanie pakietu ---
        if (packet.stream_index == gif_stream_) {
            const int send_result = avcodec_send_packet(codec_context_, &packet);
            if (send_result < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, send_result);
                qDebug() << "GifDecoder::run() - Error sending packet to decoder:" << send_result << errbuf;
                // Można spróbować kontynuować lub zatrzymać, zależnie od błędu
                // if (sendResult != AVERROR(EAGAIN) && sendResult != AVERROR_EOF) {
                //     m_stopped = true;
                // }
            } else {
                // Odbieranie klatek (może być więcej niż jedna na pakiet)
                while (true) {
                    // qDebug() << "GifDecoder::run() - Receiving frame from decoder...";
                    const int receive_result = avcodec_receive_frame(codec_context_, frame_);
                    // qDebug() << "GifDecoder::run() - avcodec_receive_frame result:" << receiveResult;

                    if (receive_result == 0) {
                        // SUKCES - Mamy klatkę!
                        qDebug() << "GifDecoder::run() - Frame received successfully! Index:" << frame_counter;
                        frame_counter++;

                        // Aktualizacja pozycji (przeniesiono tutaj)
                        if (frame_->pts != AV_NOPTS_VALUE) {
                            QMutexLocker posLocker(&mutex_);
                            current_position_ = frame_->pts * av_q2d(format_context_->streams[gif_stream_]->time_base);
                            posLocker.unlock();
                            emit positionChanged(current_position_);
                        }

                        // Konwersja klatki
                        sws_scale(
                            sws_context_,
                            (const uint8_t* const*)frame_->data, frame_->linesize, 0, codec_context_->height,
                            frame_rgb_->data, frame_rgb_->linesize
                        );

                        // Tworzenie QImage
                        QImage frame_image( // Zmieniono nazwę zmiennej, aby uniknąć konfliktu z m_frame
                            frame_rgb_->data[0],
                            codec_context_->width,
                            codec_context_->height,
                            frame_rgb_->linesize[0],
                            QImage::Format_RGBA8888 // Użyj RGBA dla przezroczystości
                        );

                        // EMISJA SYGNAŁU
                        qDebug() << "GifDecoder::run() - Emitting frameReady for frame index:" << (frame_counter - 1);
                        emit frameReady(frame_image.copy()); // Emituj kopię

                        // Opóźnienie
                        qint64 delay_time = static_cast<qint64>(frame_delay_); // Użyj m_frameDelay
                        if (!is_first_frame_after_resume) { // Użyj nowej flagi
                            const qint64 elapsed = frame_timer_.elapsed();
                            delay_time = qMax(static_cast<qint64>(0), delay_time - elapsed);
                        }
                        if (delay_time > 0) {
                            // Użyj usleep lub innego mechanizmu czekania, który można przerwać
                            // QThread::msleep jest prosty, ale może nie być idealny do szybkiego wznawiania
                            msleep(delay_time); // Użyj msleep z QThread
                        }
                        frame_timer_.restart();
                        is_first_frame_after_resume = false; // Zresetuj flagę

                    } else if (receive_result == AVERROR(EAGAIN) || receive_result == AVERROR_EOF) {
                        // EAGAIN: Potrzeba więcej danych (następny pakiet)
                        // EOF: Dekoder został opróżniony
                        // qDebug() << "GifDecoder::run() - Decoder needs more data or is flushed. Breaking receive loop.";
                        break; // Wyjdź z pętli odbierania klatek
                    } else {
                        // Inny błąd odbierania
                        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, receive_result);
                        qDebug() << "GifDecoder::run() - Error receiving frame from decoder:" << receive_result << errbuf;
                        stopped_ = true; // Poważny błąd, zatrzymaj wątek
                        break; // Wyjdź z pętli odbierania
                    }
                } // Koniec pętli odbierania klatek
            }
        } else {
            // Pakiet nie należy do naszego strumienia, ignoruj
        }

        av_packet_unref(&packet); // Zwolnij pakiet

        if (stopped_) break; // Sprawdź flagę zatrzymania po przetworzeniu pakietu

    } // Koniec głównej pętli while

    qDebug() << "GifDecoder::run() - Exiting thread loop. Total frames emitted:" << frame_counter;
    // Sygnał zakończenia można by tu emitować, jeśli nie zapętlamy w nieskończoność
    // emit playbackFinished();
}

int GifDecoder::ReadPacket(void *opaque, uint8_t *buf, const int buf_size) {
    const auto decoder = static_cast<GifDecoder*>(opaque);
    const int size = qMin(buf_size, decoder->read_position_ >= decoder->gif_data_.size() ?
                                  0 : decoder->gif_data_.size() - decoder->read_position_);

    if (size <= 0)
        return AVERROR_EOF;

    memcpy(buf, decoder->gif_data_.constData() + decoder->read_position_, size);
    decoder->read_position_ += size;
    return size;
}

int64_t GifDecoder::SeekPacket(void *opaque, const int64_t offset, const int whence) {
    const auto decoder = static_cast<GifDecoder*>(opaque);

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
        qWarning() << "GifDecoder: Cannot extract first frame, context not ready.";
        return;
    }

    // Przewiń na początek (może nie być konieczne, jeśli initialize jest pierwsze)
    av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codec_context_);
    read_position_ = 0; // Resetuj pozycję odczytu dla AVIOContext

    AVPacket packet;
    av_init_packet(&packet); // Zainicjuj pakiet
    packet.data = nullptr;
    packet.size = 0;

    bool frame_decoded = false;
    while (av_read_frame(format_context_, &packet) >= 0 && !frame_decoded) {
        if (packet.stream_index == gif_stream_) {
            if (avcodec_send_packet(codec_context_, &packet) >= 0) {
                if (avcodec_receive_frame(codec_context_, frame_) == 0) {
                    // Konwertuj ramkę do RGBA
                    sws_scale(
                        sws_context_,
                        (const uint8_t* const*)frame_->data, frame_->linesize, 0, codec_context_->height,
                        frame_rgb_->data, frame_rgb_->linesize
                    );

                    // Utwórz QImage
                    QImage firstFrame(
                        frame_rgb_->data[0],
                        codec_context_->width,
                        codec_context_->height,
                        frame_rgb_->linesize[0],
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
    av_seek_frame(format_context_, gif_stream_, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codec_context_);
    read_position_ = 0; // Resetuj pozycję odczytu ponownie

    if (!frame_decoded) {
        qWarning() << "GifDecoder: Failed to extract the first frame.";
        // Można wyemitować pusty obraz lub sygnał błędu
        emit firstFrameReady(QImage());
    }
}
