#include "inline_audio_player.h"

InlineAudioPlayer::InlineAudioPlayer(const QByteArray &audio_data, const QString &mime_type, QWidget *parent): QFrame(parent), m_audioData(audio_data), mime_type_(mime_type),
    scanline_opacity_(0.15), spectrum_intensity_(0.6) {

    // Ustawiamy styl i rozmiar - nadal zachowujemy mały rozmiar
    setFixedSize(480, 120);
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);

    // Główny layout
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 10);
    layout->setSpacing(8);

    // Panel górny z informacjami
    const auto top_panel = new QWidget(this);
    const auto top_layout = new QHBoxLayout(top_panel);
    top_layout->setContentsMargins(3, 3, 3, 3);
    top_layout->setSpacing(5);

    // Obszar tytułu/informacji o pliku audio
    audio_info_label_ = new QLabel(this);
    audio_info_label_->setStyleSheet(
        "color: #c080ff;"
        "background-color: rgba(30, 15, 60, 180);"
        "border: 1px solid #9060cc;"
        "padding: 3px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
        "font-weight: bold;"
    );
    audio_info_label_->setText("NEURAL AUDIO DECODER");
    top_layout->addWidget(audio_info_label_, 1);

    // Status odtwarzania
    status_label_ = new QLabel(this);
    status_label_->setStyleSheet(
        "color: #80c0ff;"
        "background-color: rgba(15, 30, 60, 180);"
        "border: 1px solid #6080cc;"
        "padding: 3px 8px;"
        "font-family: 'Consolas';"
        "font-size: 8pt;"
    );
    status_label_->setText("INICJALIZACJA...");
    top_layout->addWidget(status_label_);

    layout->addWidget(top_panel);

    // Wizualizator spektrum audio - mała sekcja z wizualizacją
    spectrum_view_ = new QWidget(this);
    spectrum_view_->setFixedHeight(35);
    spectrum_view_->setStyleSheet("background-color: rgba(20, 15, 40, 120); border: 1px solid #6040aa;");

    // Podłączamy timer aktualizacji spektrum
    spectrum_timer_ = new QTimer(this);
    connect(spectrum_timer_, &QTimer::timeout, spectrum_view_, QOverload<>::of(&QWidget::update));
    spectrum_timer_->start(50);

    // Nadpisanie metody paintEvent dla m_spectrumView
    spectrum_view_->installEventFilter(this);

    layout->addWidget(spectrum_view_);

    // Panel kontrolny w stylu cyberpunk
    const auto control_panel = new QWidget(this);
    const auto control_layout = new QHBoxLayout(control_panel);
    control_layout->setContentsMargins(3, 1, 3, 1);
    control_layout->setSpacing(8);

    // Cyberpunkowe przyciski
    play_button_ = new CyberAudioButton("▶", this);
    play_button_->setFixedSize(28, 24);

    // Cyberpunkowy slider postępu
    progress_slider_ = new CyberAudioSlider(Qt::Horizontal, this);

    // Etykieta czasu
    time_label_ = new QLabel("00:00 / 00:00", this);
    time_label_->setStyleSheet(
        "color: #b080ff;"
        "background-color: rgba(25, 15, 40, 180);"
        "border: 1px solid #6040aa;"
        "padding: 2px 4px;"
        "font-family: 'Consolas';"
        "font-size: 8pt;"
    );
    time_label_->setFixedWidth(90);

    // Przycisk głośności
    volume_button_ = new CyberAudioButton("🔊", this);
    volume_button_->setFixedSize(24, 24);

    // Slider głośności
    volume_slider_ = new CyberAudioSlider(Qt::Horizontal, this);
    volume_slider_->setRange(0, 100);
    volume_slider_->setValue(100);
    volume_slider_->setFixedWidth(60);

    // Dodanie kontrolek do layoutu
    control_layout->addWidget(play_button_);
    control_layout->addWidget(progress_slider_, 1);
    control_layout->addWidget(time_label_);
    control_layout->addWidget(volume_button_);
    control_layout->addWidget(volume_slider_);

    layout->addWidget(control_panel);

    // Dekoder audio
    decoder_ = std::make_shared<AudioDecoder>(audio_data, this);

    // Połącz sygnały
    connect(play_button_, &QPushButton::clicked, this, &InlineAudioPlayer::TogglePlayback);
    connect(progress_slider_, &QSlider::sliderMoved, this, &InlineAudioPlayer::UpdateTimeLabel);
    connect(progress_slider_, &QSlider::sliderPressed, this, &InlineAudioPlayer::OnSliderPressed);
    connect(progress_slider_, &QSlider::sliderReleased, this, &InlineAudioPlayer::OnSliderReleased);
    connect(decoder_.get(), &AudioDecoder::error, this, &InlineAudioPlayer::HandleError);
    connect(decoder_.get(), &AudioDecoder::audioInfo, this, &InlineAudioPlayer::HandleAudioInfo);
    connect(decoder_.get(), &AudioDecoder::playbackFinished, this, [this]() {
        playback_finished_ = true;
        play_button_->setText("↻");
        status_label_->setText("ZAKOŃCZONO ODTWARZANIE");
        DecreaseSpectrumIntensity();
        update();
    });

    // Połączenia dla kontrolek dźwięku
    connect(volume_slider_, &QSlider::valueChanged, this, &InlineAudioPlayer::AdjustVolume);
    connect(volume_button_, &QPushButton::clicked, this, &InlineAudioPlayer::ToggleMute);

    // Aktualizacja pozycji
    connect(decoder_.get(), &AudioDecoder::positionChanged, this, &InlineAudioPlayer::UpdateSliderPosition);

    // Timer dla animacji interfejsu
    ui_timer_ = new QTimer(this);
    ui_timer_->setInterval(50);
    connect(ui_timer_, &QTimer::timeout, this, &InlineAudioPlayer::UpdateUI);
    ui_timer_->start();

    // Inicjalizuj odtwarzacz w osobnym wątku
    QTimer::singleShot(100, this, [this]() {
        decoder_->start(QThread::HighPriority);
    });

    // Zabezpieczenie przy zamknięciu aplikacji
    connect(qApp, &QApplication::aboutToQuit, this, [this]() {
        if (active_player_ == this) {
            active_player_ = nullptr;
        }
        ReleaseResources();
    });
}

void InlineAudioPlayer::ReleaseResources() {
    // Zatrzymujemy timery
    if (ui_timer_) {
        ui_timer_->stop();
    }
    if (spectrum_timer_) {
        spectrum_timer_->stop();
    }

    if (decoder_) {
        decoder_->Stop();
        decoder_->wait();
        decoder_->ReleaseResources();
    }

    if (active_player_ == this) {
        active_player_ = nullptr;
    }
    is_active_ = false;
}

void InlineAudioPlayer::Activate() {
    if (is_active_)
        return;

    // Jeśli istnieje inny aktywny odtwarzacz, deaktywuj go najpierw
    if (active_player_ && active_player_ != this) {
        active_player_->Deactivate();
    }

    // Upewnij się, że dekoder jest w odpowiednim stanie
    if (!decoder_->isRunning()) {
        if (!decoder_->Reinitialize()) {
            qDebug() << "Nie udało się zainicjalizować dekodera";
            return;
        }
        decoder_->start(QThread::HighPriority);
    }

    // Ustaw ten odtwarzacz jako aktywny
    active_player_ = this;
    is_active_ = true;

    // Animacja aktywacji
    const auto spectrum_animation = new QPropertyAnimation(this, "spectrumIntensity");
    spectrum_animation->setDuration(700);
    spectrum_animation->setStartValue(0.1);
    spectrum_animation->setEndValue(0.6);
    spectrum_animation->setEasingCurve(QEasingCurve::OutQuad);
    spectrum_animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void InlineAudioPlayer::Deactivate() {
    if (!is_active_)
        return;

    // Zatrzymaj odtwarzanie, jeśli trwa
    if (decoder_ && !decoder_->IsPaused()) {
        decoder_->Pause(); // Zatrzymaj odtwarzanie
    }

    // Jeśli ten odtwarzacz jest aktywny globalnie, wyczyść referencję
    if (active_player_ == this) {
        active_player_ = nullptr;
    }

    // Animacja dezaktywacji
    const auto spectrum_animation = new QPropertyAnimation(this, "spectrumIntensity");
    spectrum_animation->setDuration(500);
    spectrum_animation->setStartValue(spectrum_intensity_);
    spectrum_animation->setEndValue(0.1);
    spectrum_animation->start(QPropertyAnimation::DeleteWhenStopped);

    is_active_ = false;
}

void InlineAudioPlayer::AdjustVolume(const int volume) const {
    if (!decoder_)
        return;

    const float volume_float = volume / 100.0f;
    decoder_->SetVolume(volume_float);
    UpdateVolumeIcon(volume_float);
}

void InlineAudioPlayer::ToggleMute() {
    if (!decoder_)
        return;

    if (volume_slider_->value() > 0) {
        last_volume_ = volume_slider_->value();
        volume_slider_->setValue(0);
    } else {
        volume_slider_->setValue(last_volume_ > 0 ? last_volume_ : 100);
    }
}

void InlineAudioPlayer::UpdateVolumeIcon(const float volume) const {
    if (volume <= 0.01f) {
        volume_button_->setText("🔈");
    } else if (volume < 0.5f) {
        volume_button_->setText("🔉");
    } else {
        volume_button_->setText("🔊");
    }
}

void InlineAudioPlayer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Ramki w stylu AR
    constexpr QColor border_color(140, 80, 240);
    painter.setPen(QPen(border_color, 1));

    // Technologiczna ramka
    QPainterPath frame;
    constexpr int clip_size = 15;

    // Górna krawędź
    frame.moveTo(clip_size, 0);
    frame.lineTo(width() - clip_size, 0);

    // Prawy górny róg
    frame.lineTo(width(), clip_size);

    // Prawa krawędź
    frame.lineTo(width(), height() - clip_size);

    // Prawy dolny róg
    frame.lineTo(width() - clip_size, height());

    // Dolna krawędź
    frame.lineTo(clip_size, height());

    // Lewy dolny róg
    frame.lineTo(0, height() - clip_size);

    // Lewa krawędź
    frame.lineTo(0, clip_size);

    // Lewy górny róg
    frame.lineTo(clip_size, 0);

    painter.drawPath(frame);

    // Dane techniczne w rogach
    painter.setPen(border_color.lighter(120));
    painter.setFont(QFont("Consolas", 7));
}

bool InlineAudioPlayer::eventFilter(QObject *watched, QEvent *event) {
    // Obsługa malowania spektrum audio
    if (watched == spectrum_view_ && event->type() == QEvent::Paint) {
        PaintSpectrum(spectrum_view_);
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

void InlineAudioPlayer::OnSliderPressed() {
    // Zapamiętaj stan odtwarzania przed przesunięciem
    was_playing_ = !decoder_->IsPaused();

    // Zatrzymaj odtwarzanie na czas przesuwania
    if (was_playing_) {
        decoder_->Pause();
    }

    slider_dragging_ = true;
    status_label_->setText("WYSZUKIWANIE...");
}

void InlineAudioPlayer::OnSliderReleased() {
    // Wykonaj faktyczne przesunięcie
    SeekAudio(progress_slider_->value());

    slider_dragging_ = false;

    // Przywróć odtwarzanie jeśli było aktywne wcześniej
    if (was_playing_) {
        decoder_->Pause(); // Przełącza stan pauzy (wznawia)
        play_button_->setText("❚❚");
        status_label_->setText("ODTWARZANIE");

        // Aktywujemy wizualizację
        IncreaseSpectrumIntensity();
    } else {
        status_label_->setText("PAUZA");
    }
}

void InlineAudioPlayer::UpdateTimeLabel(const int position) {
    if (!decoder_ || audio_duration_ <= 0)
        return;

    current_position_ = position / 1000.0; // Milisekundy na sekundy
    const int current_seconds = static_cast<int>(current_position_);
    const int total_seconds = static_cast<int>(audio_duration_);

    time_label_->setText(QString("%1:%2 / %3:%4")
        .arg(current_seconds / 60, 2, 10, QChar('0'))
        .arg(current_seconds % 60, 2, 10, QChar('0'))
        .arg(total_seconds / 60, 2, 10, QChar('0'))
        .arg(total_seconds % 60, 2, 10, QChar('0')));
}

void InlineAudioPlayer::UpdateSliderPosition(double position) const {
    // Bezpośrednia aktualizacja pozycji suwaka z dekodera
    if (audio_duration_ <= 0)
        return;

    // Zabezpieczenie przed niepoprawnymi wartościami
    if (position < 0) position = 0;
    if (position > audio_duration_) position = audio_duration_;

    // Aktualizacja suwaka - bez emitowania sygnałów
    progress_slider_->blockSignals(true);
    progress_slider_->setValue(position * 1000);
    progress_slider_->blockSignals(false);

    // Aktualizacja etykiety czasu
    const int seconds = static_cast<int>(position) % 60;
    const int minutes = static_cast<int>(position) / 60;

    const int total_seconds = static_cast<int>(audio_duration_) % 60;
    const int total_minutes = static_cast<int>(audio_duration_) / 60;

    time_label_->setText(
        QString("%1:%2 / %3:%4")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(total_minutes, 2, 10, QChar('0'))
        .arg(total_seconds, 2, 10, QChar('0'))
    );
}

void InlineAudioPlayer::SeekAudio(const int position) const {
    if (!decoder_ || audio_duration_ <= 0)
        return;

    const double seek_position = position / 1000.0; // Milisekundy na sekundy
    decoder_->Seek(seek_position);
}

void InlineAudioPlayer::TogglePlayback() {
    if (!decoder_)
        return;

    // Aktywuj ten odtwarzacz przed odtwarzaniem
    Activate();

    if (playback_finished_) {
        // Resetuj odtwarzacz
        decoder_->Reset();
        playback_finished_ = false;
        play_button_->setText("❚❚");
        status_label_->setText("ODTWARZANIE");
        current_position_ = 0;
        UpdateTimeLabel(0);
        IncreaseSpectrumIntensity();
        decoder_->Pause(); // Rozpocznij odtwarzanie (przełącz stan)
    } else {
        if (decoder_->IsPaused()) {
            play_button_->setText("❚❚");
            status_label_->setText("ODTWARZANIE");
            IncreaseSpectrumIntensity();
            decoder_->Pause(); // Wznów odtwarzanie
        } else {
            play_button_->setText("▶");
            status_label_->setText("PAUZA");
            DecreaseSpectrumIntensity();
            decoder_->Pause(); // Wstrzymaj odtwarzanie
        }
    }
}

void InlineAudioPlayer::HandleError(const QString &message) const {
    qDebug() << "Audio decoder error:" << message;
    status_label_->setText("ERROR: " + message.left(20));
    audio_info_label_->setText("⚠️ " + message.left(30));
}

void InlineAudioPlayer::HandleAudioInfo(const int sample_rate, const int channels, const double duration) {
    audio_duration_ = duration;
    progress_slider_->setRange(0, duration * 1000);

    // Wyświetl informacje o audio
    QString audio_info = mime_type_;
    if (audio_info.isEmpty()) {
        audio_info = "Audio";
    }

    // Usuwamy "audio/" z początku typu MIME jeśli istnieje
    audio_info.replace("audio/", "");

    // Dodajemy informacje o parametrach audio
    audio_info += QString(" (%1kHz, %2ch)").arg(sample_rate/1000.0, 0, 'f', 1).arg(channels);
    audio_info_label_->setText(audio_info.toUpper());
    status_label_->setText("GOTOWY");

    // Przygotowujemy losowe dane dla wizualizacji spektrum
    for (int i = 0; i < 64; i++) {
        spectrum_data_.append(0.2 + QRandomGenerator::global()->bounded(60) / 100.0);
    }
}

void InlineAudioPlayer::UpdateUI() {
    // Aktualizacja stanu UI i animacje
    if (decoder_ && !decoder_->IsPaused() && !playback_finished_) {
        // Co pewien czas aktualizacja statusu
        const int random_update = QRandomGenerator::global()->bounded(100);
        if (random_update < 2) {
            status_label_->setText(QString("BUFFER: %1%").arg(QRandomGenerator::global()->bounded(95, 100)));
        } else if (random_update < 4) {
            status_label_->setText(QString("SYNC: %1%").arg(QRandomGenerator::global()->bounded(98, 100)));
        }

        // Aktualizacja danych spektrum - losowe fluktuacje dla efektów wizualnych
        for (int i = 0; i < spectrum_data_.size(); i++) {
            if (!decoder_->IsPaused()) {
                // Bardziej dynamiczne zmiany podczas odtwarzania
                const double change = (QRandomGenerator::global()->bounded(100) - 50) / 250.0;
                spectrum_data_[i] += change;
                spectrum_data_[i] = qBound(0.05, spectrum_data_[i], 1.0);
            } else {
                // Opadające słupki podczas pauzy
                if (spectrum_data_[i] > 0.2) {
                    spectrum_data_[i] *= 0.98;
                }
            }
        }
    }
}

void InlineAudioPlayer::PaintSpectrum(QWidget *target) {
    QPainter painter(target);
    painter.setRenderHint(QPainter::Antialiasing);

    // Wypełniamy tło
    painter.fillRect(target->rect(), QColor(10, 5, 20));

    // Rysujemy siatkę
    painter.setPen(QPen(QColor(50, 30, 90, 100), 1, Qt::DotLine));

    const int step_x = target->width() / 8;
    for (int x = step_x; x < target->width(); x += step_x) {
        painter.drawLine(x, 0, x, target->height());
    }

    const int step_y = target->height() / 3;
    for (int y = step_y; y < target->height(); y += step_y) {
        painter.drawLine(0, y, target->width(), y);
    }

    // Rysujemy słupki spektrum
    if (!spectrum_data_.isEmpty()) {
        const int bar_count = qMin(32, spectrum_data_.size());
        const double bar_width = static_cast<double>(target->width()) / bar_count;

        for (int i = 0; i < bar_count; i++) {
            // Wysokość słupka zależna od danych i intensywności
            const double height = spectrum_data_[i] * spectrum_intensity_ * target->height() * 0.9;

            // Gradient dla słupka - od jasnego po ciemny
            QLinearGradient gradient(0, target->height() - height, 0, target->height());
            gradient.setColorAt(0, QColor(170, 100, 255, 200));
            gradient.setColorAt(0.5, QColor(120, 80, 200, 150));
            gradient.setColorAt(1, QColor(60, 40, 120, 100));

            // Rysujemy słupek
            QRectF bar_rect(i * bar_width, target->height() - height, bar_width - 1, height);
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(bar_rect, 1, 1);

            // Dodajemy "błysk" na górze słupka
            if (height > 3) {
                painter.setBrush(QColor(255, 255, 255, 150));
                painter.drawRoundedRect(QRectF(i * bar_width, target->height() - height, bar_width - 1, 2), 1, 1);
            }
        }
    }
}
