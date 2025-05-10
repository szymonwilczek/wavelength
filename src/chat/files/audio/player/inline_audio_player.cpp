#include "inline_audio_player.h"

#include <QApplication>
#include <QRandomGenerator>
#include <QVBoxLayout>

#include "../../../../app/managers/translation_manager.h"

InlineAudioPlayer::InlineAudioPlayer(const QByteArray &audio_data, const QString &mime_type,
                                     QWidget *parent): QFrame(parent), m_audioData(audio_data), mime_type_(mime_type),
                                                       scanline_opacity_(0.15), spectrum_intensity_(0.6),
                                                       translator_(nullptr) {
    translator_ = TranslationManager::GetInstance();

    setFixedSize(480, 120);
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 10);
    layout->setSpacing(8);

    const auto top_panel = new QWidget(this);
    const auto top_layout = new QHBoxLayout(top_panel);
    top_layout->setContentsMargins(3, 3, 3, 3);
    top_layout->setSpacing(5);

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

    status_label_ = new QLabel(this);
    status_label_->setStyleSheet(
        "color: #80c0ff;"
        "background-color: rgba(15, 30, 60, 180);"
        "border: 1px solid #6080cc;"
        "padding: 3px 8px;"
        "font-family: 'Consolas';"
        "font-size: 8pt;"
    );
    status_label_->setText(translator_->Translate("AudioPlayer.Initialization", "INITIALIZING..."));
    top_layout->addWidget(status_label_);

    layout->addWidget(top_panel);

    spectrum_view_ = new QWidget(this);
    spectrum_view_->setFixedHeight(35);
    spectrum_view_->setStyleSheet("background-color: rgba(20, 15, 40, 120); border: 1px solid #6040aa;");

    spectrum_timer_ = new QTimer(this);
    connect(spectrum_timer_, &QTimer::timeout, spectrum_view_, QOverload<>::of(&QWidget::update));
    spectrum_timer_->start(50);

    spectrum_view_->installEventFilter(this);
    layout->addWidget(spectrum_view_);

    const auto control_panel = new QWidget(this);
    const auto control_layout = new QHBoxLayout(control_panel);
    control_layout->setContentsMargins(3, 1, 3, 1);
    control_layout->setSpacing(8);

    play_button_ = new CyberAudioButton("▶", this);
    play_button_->setFixedSize(28, 24);

    progress_slider_ = new CyberAudioSlider(Qt::Horizontal, this);

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

    volume_button_ = new CyberAudioButton("🔊", this);
    volume_button_->setFixedSize(24, 24);

    volume_slider_ = new CyberAudioSlider(Qt::Horizontal, this);
    volume_slider_->setRange(0, 100);
    volume_slider_->setValue(100);
    volume_slider_->setFixedWidth(60);

    control_layout->addWidget(play_button_);
    control_layout->addWidget(progress_slider_, 1);
    control_layout->addWidget(time_label_);
    control_layout->addWidget(volume_button_);
    control_layout->addWidget(volume_slider_);

    layout->addWidget(control_panel);

    decoder_ = std::make_shared<AudioDecoder>(audio_data, this);

    connect(play_button_, &QPushButton::clicked, this, &InlineAudioPlayer::TogglePlayback);
    connect(progress_slider_, &QSlider::sliderMoved, this, &InlineAudioPlayer::UpdateTimeLabel);
    connect(progress_slider_, &QSlider::sliderPressed, this, &InlineAudioPlayer::OnSliderPressed);
    connect(progress_slider_, &QSlider::sliderReleased, this, &InlineAudioPlayer::OnSliderReleased);
    connect(decoder_.get(), &AudioDecoder::error, this, &InlineAudioPlayer::HandleError);
    connect(decoder_.get(), &AudioDecoder::audioInfo, this, &InlineAudioPlayer::HandleAudioInfo);
    connect(decoder_.get(), &AudioDecoder::playbackFinished, this, [this]() {
        playback_finished_ = true;
        play_button_->setText("↻");
        status_label_->setText(translator_->Translate("AudioPlayer.PlaybackFinished", "PLAYBACK FINISHED"));
        DecreaseSpectrumIntensity();
        update();
    });

    connect(volume_slider_, &QSlider::valueChanged, this, &InlineAudioPlayer::AdjustVolume);
    connect(volume_button_, &QPushButton::clicked, this, &InlineAudioPlayer::ToggleMute);

    connect(decoder_.get(), &AudioDecoder::positionChanged, this, &InlineAudioPlayer::UpdateSliderPosition);

    ui_timer_ = new QTimer(this);
    ui_timer_->setInterval(50);
    connect(ui_timer_, &QTimer::timeout, this, &InlineAudioPlayer::UpdateUI);
    ui_timer_->start();

    QTimer::singleShot(100, this, [this]() {
        decoder_->start(QThread::HighPriority);
    });

    connect(qApp, &QApplication::aboutToQuit, this, [this]() {
        if (active_player_ == this) {
            active_player_ = nullptr;
        }
        ReleaseResources();
    });
}

void InlineAudioPlayer::ReleaseResources() {
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

    if (active_player_ && active_player_ != this) {
        active_player_->Deactivate();
    }

    if (!decoder_->isRunning()) {
        if (!decoder_->Reinitialize()) {
            qDebug() << "[INLINE AUDIO PLAYER] Failed to initialize decoder.";
            return;
        }
        decoder_->start(QThread::HighPriority);
    }

    active_player_ = this;
    is_active_ = true;

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

    if (decoder_ && !decoder_->IsPaused()) {
        decoder_->Pause();
    }

    if (active_player_ == this) {
        active_player_ = nullptr;
    }

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

    constexpr QColor border_color(140, 80, 240);
    painter.setPen(QPen(border_color, 1));

    QPainterPath frame;
    constexpr int clip_size = 15;

    // top edge
    frame.moveTo(clip_size, 0);
    frame.lineTo(width() - clip_size, 0);

    // right-top
    frame.lineTo(width(), clip_size);

    // right edge
    frame.lineTo(width(), height() - clip_size);

    // right-bottom
    frame.lineTo(width() - clip_size, height());

    // bottom edge
    frame.lineTo(clip_size, height());

    // left-bottom edge
    frame.lineTo(0, height() - clip_size);

    // left edge
    frame.lineTo(0, clip_size);

    // left-top
    frame.lineTo(clip_size, 0);

    painter.drawPath(frame);

    painter.setPen(border_color.lighter(120));
    painter.setFont(QFont("Consolas", 7));
}

bool InlineAudioPlayer::eventFilter(QObject *watched, QEvent *event) {
    if (watched == spectrum_view_ && event->type() == QEvent::Paint) {
        PaintSpectrum(spectrum_view_);
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

void InlineAudioPlayer::OnSliderPressed() {
    was_playing_ = !decoder_->IsPaused();

    if (was_playing_) {
        decoder_->Pause();
    }

    slider_dragging_ = true;
    status_label_->setText(translator_->Translate("AudioPlayer.Seeking", "SEEKING..."));
}

void InlineAudioPlayer::OnSliderReleased() {
    SeekAudio(progress_slider_->value());

    slider_dragging_ = false;

    if (was_playing_) {
        decoder_->Pause();
        play_button_->setText("❚❚");
        status_label_->setText(translator_->Translate("AudioPlayer.Playing", "PLAYING..."));

        IncreaseSpectrumIntensity();
    } else {
        status_label_->setText(translator_->Translate("AudioPlayer.Paused", "PAUSED"));
    }
}

void InlineAudioPlayer::UpdateTimeLabel(const int position) {
    if (!decoder_ || audio_duration_ <= 0)
        return;

    current_position_ = position / 1000.0; // ms to seconds
    const int current_seconds = static_cast<int>(current_position_);
    const int total_seconds = static_cast<int>(audio_duration_);

    time_label_->setText(QString("%1:%2 / %3:%4")
        .arg(current_seconds / 60, 2, 10, QChar('0'))
        .arg(current_seconds % 60, 2, 10, QChar('0'))
        .arg(total_seconds / 60, 2, 10, QChar('0'))
        .arg(total_seconds % 60, 2, 10, QChar('0')));
}

void InlineAudioPlayer::UpdateSliderPosition(double position) const {
    if (audio_duration_ <= 0)
        return;

    if (position < 0) position = 0;
    if (position > audio_duration_) position = audio_duration_;

    progress_slider_->blockSignals(true);
    progress_slider_->setValue(position * 1000);
    progress_slider_->blockSignals(false);

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

    const double seek_position = position / 1000.0; // ms to seconds
    decoder_->Seek(seek_position);
}

void InlineAudioPlayer::TogglePlayback() {
    if (!decoder_)
        return;

    Activate();

    if (playback_finished_) {
        decoder_->Reset();
        playback_finished_ = false;
        play_button_->setText("❚❚");
        status_label_->setText(translator_->Translate("AudioPlayer.Playing", "PLAYING..."));
        current_position_ = 0;
        UpdateTimeLabel(0);
        IncreaseSpectrumIntensity();
        decoder_->Pause(); // start playback (switch state)
    } else {
        if (decoder_->IsPaused()) {
            play_button_->setText("❚❚");
            status_label_->setText(translator_->Translate("AudioPlayer.Playing", "PLAYING..."));
            IncreaseSpectrumIntensity();
            decoder_->Pause(); // resume playback
        } else {
            play_button_->setText("▶");
            status_label_->setText(translator_->Translate("AudioPlayer.Paused", "PAUSED"));
            DecreaseSpectrumIntensity();
            decoder_->Pause(); // pause playback
        }
    }
}

void InlineAudioPlayer::HandleError(const QString &message) const {
    qDebug() << "[INLINE AUDIO PLAYER] Audio decoder error:" << message;
    status_label_->setText("ERROR: " + message.left(20));
    audio_info_label_->setText("⚠️ " + message.left(30));
}

void InlineAudioPlayer::HandleAudioInfo(const int sample_rate, const int channels, const double duration) {
    audio_duration_ = duration;
    progress_slider_->setRange(0, duration * 1000);

    QString audio_info = mime_type_;
    if (audio_info.isEmpty()) {
        audio_info = "Audio";
    }

    audio_info.replace("audio/", "");

    audio_info += QString(" (%1kHz, %2ch)").arg(sample_rate / 1000.0, 0, 'f', 1).arg(channels);
    audio_info_label_->setText(audio_info.toUpper());
    status_label_->setText(translator_->Translate("AudioPlayer.Ready", "READY"));

    for (int i = 0; i < 64; i++) {
        spectrum_data_.append(0.2 + QRandomGenerator::global()->bounded(60) / 100.0);
    }
}

void InlineAudioPlayer::UpdateUI() {
    if (decoder_ && !decoder_->IsPaused() && !playback_finished_) {
        const int random_update = QRandomGenerator::global()->bounded(100);

        if (random_update < 2) {
            status_label_->setText(QString("BUFFER: %1%").arg(QRandomGenerator::global()->bounded(95, 100)));
        } else if (random_update < 4) {
            status_label_->setText(QString("SYNC: %1%").arg(QRandomGenerator::global()->bounded(98, 100)));
        }

        for (int i = 0; i < spectrum_data_.size(); i++) {
            if (!decoder_->IsPaused()) {
                const double change = (QRandomGenerator::global()->bounded(100) - 50) / 250.0;
                spectrum_data_[i] += change;
                spectrum_data_[i] = qBound(0.05, spectrum_data_[i], 1.0);
            } else {
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

    painter.fillRect(target->rect(), QColor(10, 5, 20));

    painter.setPen(QPen(QColor(50, 30, 90, 100), 1, Qt::DotLine));

    const int step_x = target->width() / 8;
    for (int x = step_x; x < target->width(); x += step_x) {
        painter.drawLine(x, 0, x, target->height());
    }

    const int step_y = target->height() / 3;
    for (int y = step_y; y < target->height(); y += step_y) {
        painter.drawLine(0, y, target->width(), y);
    }

    if (!spectrum_data_.isEmpty()) {
        const int bar_count = qMin(32, spectrum_data_.size());
        const double bar_width = static_cast<double>(target->width()) / bar_count;

        for (int i = 0; i < bar_count; i++) {
            const double height = spectrum_data_[i] * spectrum_intensity_ * target->height() * 0.9;

            QLinearGradient gradient(0, target->height() - height, 0, target->height());
            gradient.setColorAt(0, QColor(170, 100, 255, 200));
            gradient.setColorAt(0.5, QColor(120, 80, 200, 150));
            gradient.setColorAt(1, QColor(60, 40, 120, 100));

            QRectF bar_rect(i * bar_width, target->height() - height, bar_width - 1, height);
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(bar_rect, 1, 1);

            if (height > 3) {
                painter.setBrush(QColor(255, 255, 255, 150));
                painter.drawRoundedRect(QRectF(i * bar_width, target->height() - height, bar_width - 1, 2), 1, 1);
            }
        }
    }
}
