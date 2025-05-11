#include "video_player.h"

#include <QCloseEvent>
#include <QLabel>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QVBoxLayout>

#include "../../../../ui/buttons/cyber_push_button.h"
#include "../../../../ui/sliders/cyber_slider.h"
#include "../decoder/video_decoder.h"
#include "../../../../app/managers/translation_manager.h"

VideoPlayer::VideoPlayer(const QByteArray &video_data, const QString &mime_type,
                         QWidget *parent): QDialog(parent), video_data_(video_data),
                                           mime_type_(mime_type),
                                           scanline_opacity_(0.15), grid_opacity_(0.1) {
    translator_ = TranslationManager::GetInstance();
    setWindowTitle("SYS> WVLNGTH-V-STREAM-PLAYER");
    setMinimumSize(900, 630);
    setModal(false);

    setStyleSheet(
        "QDialog {"
        "  background-color: #0a1520;"
        "  color: #00ccff;"
        "  font-family: 'Consolas';"
        "}"
    );

#ifdef Q_OS_WINDOWS
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        if (auto hwnd = reinterpret_cast<HWND>(this->winId())) {
            // attribute 20 for Windows 10 1809+ / Windows 11
            BOOL dark_mode = TRUE;
            ::DwmSetWindowAttribute(hwnd, 20, &dark_mode, sizeof(dark_mode));
        }
    }
#endif

    auto main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(10);
    main_layout->setContentsMargins(15, 15, 15, 15);

    auto top_panel = new QWidget(this);
    auto top_layout = new QHBoxLayout(top_panel);
    top_layout->setContentsMargins(5, 5, 5, 5);

    auto title_label = new QLabel("VISUAL STREAM DECODER v2.5", this);
    title_label->setStyleSheet(
        "color: #00ffff;"
        "background-color: #001822;"
        "border: 1px solid #00aaff;"
        "padding: 4px;"
        "border-radius: 0px;"
        "font-weight: bold;"
    );

    status_label_ = new QLabel(translator_->Translate("VideoPlayer.Initializing", "INITIALIZING..."), this);
    status_label_->setStyleSheet(
        "color: #ffcc00;"
        "background-color: #221800;"
        "border: 1px solid #ffaa00;"
        "padding: 4px;"
        "border-radius: 0px;"
    );

    top_layout->addWidget(title_label);
    top_layout->addWidget(status_label_, 1);

    main_layout->addWidget(top_panel);

    auto video_container = new QWidget(this);
    auto video_layout = new QVBoxLayout(video_container);
    video_layout->setContentsMargins(20, 20, 20, 20);

    video_container->setStyleSheet(
        "background-color: rgba(0, 20, 30, 100);"
        "border: 1px solid #00aaff;"
    );

    video_label_ = new QLabel(this);
    video_label_->setAlignment(Qt::AlignCenter);
    video_label_->setMinimumSize(828, 466);
    video_label_->setText(translator_->Translate("VideoPlayer.BufferLoading", "BUFFER LOADING..."));
    video_label_->setStyleSheet(
        "color: #00ffff;"
        "background-color: #000000;"
        "border: 1px solid #005599;"
        "font-weight: bold;"
    );

    video_layout->addWidget(video_label_);
    main_layout->addWidget(video_container);

    auto control_panel = new QWidget(this);
    auto control_layout = new QHBoxLayout(control_panel);
    control_layout->setContentsMargins(5, 5, 5, 5);

    play_button_ = new CyberPushButton("▶", this);
    play_button_->setFixedSize(40, 30);

    progress_slider_ = new CyberSlider(Qt::Horizontal, this);

    time_label_ = new QLabel("00:00 / 00:00", this);
    time_label_->setStyleSheet(
        "color: #00ffff;"
        "background-color: #001822;"
        "border: 1px solid #00aaff;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );
    time_label_->setMinimumWidth(120);

    volume_button_ = new CyberPushButton("🔊", this);
    volume_button_->setFixedSize(30, 30);

    volume_slider_ = new CyberSlider(Qt::Horizontal, this);
    volume_slider_->setRange(0, 100);
    volume_slider_->setValue(100);
    volume_slider_->setFixedWidth(80);

    control_layout->addWidget(play_button_);
    control_layout->addWidget(progress_slider_, 1);
    control_layout->addWidget(time_label_);
    control_layout->addWidget(volume_button_);
    control_layout->addWidget(volume_slider_);

    main_layout->addWidget(control_panel);

    auto info_panel = new QWidget(this);
    auto info_layout = new QHBoxLayout(info_panel);
    info_layout->setContentsMargins(2, 2, 2, 2);

    // neon labels with technical data
    codec_label_ = new QLabel(translator_->Translate("VideoPlayer.CodecAnalyzing", "CODEC ANALYZING..."), this);
    resolution_label_ = new QLabel("RES: --x--", this);
    bitrate_label_ = new QLabel("BITRATE: --", this);
    fps_label_ = new QLabel("FPS: --", this);

    auto security_label = new QLabel(
        QString("SEC: LVL%1").arg(QRandomGenerator::global()->bounded(1, 6)), this);

    QString session_id = QString("%1-%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10000, 99999));
    auto session_label = new QLabel(QString("SESS: %1").arg(session_id), this);

    QString info_label_style =
            "color: #00ccff;"
            "background-color: transparent;"
            "font-family: 'Consolas';"
            "font-size: 8pt;";

    codec_label_->setStyleSheet(info_label_style);
    resolution_label_->setStyleSheet(info_label_style);
    bitrate_label_->setStyleSheet(info_label_style);
    fps_label_->setStyleSheet(info_label_style);
    security_label->setStyleSheet(info_label_style);
    session_label->setStyleSheet(info_label_style);

    info_layout->addWidget(codec_label_);
    info_layout->addWidget(resolution_label_);
    info_layout->addWidget(bitrate_label_);
    info_layout->addWidget(fps_label_);
    info_layout->addStretch();
    info_layout->addWidget(security_label);
    info_layout->addWidget(session_label);

    main_layout->addWidget(info_panel);

    connect(play_button_, &QPushButton::clicked, this, &VideoPlayer::TogglePlayback);
    connect(progress_slider_, &QSlider::sliderMoved, this, &VideoPlayer::UpdateTimeLabel);
    connect(progress_slider_, &QSlider::sliderPressed, this, &VideoPlayer::OnSliderPressed);
    connect(progress_slider_, &QSlider::sliderReleased, this, &VideoPlayer::OnSliderReleased);
    connect(volume_slider_, &QSlider::valueChanged, this, &VideoPlayer::AdjustVolume);
    connect(volume_button_, &QPushButton::clicked, this, &VideoPlayer::ToggleMute);

    // timer for animation and effects
    update_timer_ = new QTimer(this);
    update_timer_->setInterval(50);
    connect(update_timer_, &QTimer::timeout, this, &VideoPlayer::UpdateUI);
    update_timer_->start();

    // Timer for digital "noise"
    glitch_timer_ = new QTimer(this);
    glitch_timer_->setInterval(QRandomGenerator::global()->bounded(2000, 5000));
    connect(glitch_timer_, &QTimer::timeout, this, &VideoPlayer::TriggerGlitch);
    glitch_timer_->start();

    QTimer::singleShot(800, this, &VideoPlayer::InitializePlayer);
}

VideoPlayer::~VideoPlayer() {
    if (update_timer_) {
        update_timer_->stop();
    }
    if (glitch_timer_) {
        glitch_timer_->stop();
    }
    ReleaseResources();
}

void VideoPlayer::ReleaseResources() {
    if (decoder_) {
        disconnect(decoder_.get(), nullptr, this, nullptr);
        decoder_->Stop();
        decoder_->wait(1000);
        decoder_->ReleaseResources();
        decoder_.reset();
    }
}

void VideoPlayer::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (grid_opacity_ > 0.01) {
        painter.setPen(QPen(QColor(0, 150, 255, 30 * grid_opacity_), 1, Qt::DotLine));

        // horizontal grid lines
        for (int y = 0; y < height(); y += 40) {
            painter.drawLine(0, y, width(), y);
        }

        // vertical grid lines
        for (int x = 0; x < width(); x += 40) {
            painter.drawLine(x, 0, x, height());
        }
    }
}

void VideoPlayer::closeEvent(QCloseEvent *event) {
    if (update_timer_) {
        update_timer_->stop();
    }
    if (glitch_timer_) {
        glitch_timer_->stop();
    }

    ReleaseResources();
    event->accept();
}

void VideoPlayer::InitializePlayer() {
    if (!decoder_) {
        status_label_->setText(
            translator_->Translate("VideoPlayer.DecoderInitializing", "DECODER INITIALIZING...")
        );

        decoder_ = std::make_shared<VideoDecoder>(video_data_, nullptr);

        connect(decoder_.get(), &VideoDecoder::frameReady, this, &VideoPlayer::UpdateFrame,
                Qt::QueuedConnection);
        connect(decoder_.get(), &VideoDecoder::error, this, &VideoPlayer::HandleError, Qt::QueuedConnection);
        connect(decoder_.get(), &VideoDecoder::videoInfo, this, &VideoPlayer::HandleVideoInfo,
                Qt::QueuedConnection);
        connect(decoder_.get(), &VideoDecoder::playbackFinished, this, [this] {
            playback_finished_ = true;
            play_button_->setText("↻");
            status_label_->setText(
                translator_->Translate("VideoPlayer.PlaybackFinished", "PLAYBACK FINISHED")
            );
        }, Qt::QueuedConnection);
        connect(decoder_.get(), &VideoDecoder::positionChanged, this, &VideoPlayer::UpdateSliderPosition,
                Qt::QueuedConnection);

        decoder_->start(QThread::HighPriority);
        play_button_->setText("▶");
        status_label_->setText(translator_->Translate("VideoPlayer.Ready", "READY"));
    }
}

void VideoPlayer::TogglePlayback() {
    if (!decoder_) {
        InitializePlayer();
        return;
    }

    if (playback_finished_) {
        decoder_->Reset();
        playback_finished_ = false;
        play_button_->setText("▶");
        status_label_->setText(translator_->Translate("VideoPlayer.Finished", "FINISHED (READY)"));
        return;
    }

    decoder_->Pause();

    if (decoder_->IsPaused()) {
        play_button_->setText("▶");
        status_label_->setText(translator_->Translate("VideoPlayer.Paused", "PAUSED"));
    } else {
        play_button_->setText("❚❚");
        status_label_->setText(translator_->Translate("VideoPlayer.Playing", "PLAYING..."));
    }

    const auto grid_animationn = new QPropertyAnimation(this, "gridOpacity");
    grid_animationn->setDuration(500);
    grid_animationn->setStartValue(grid_opacity_);
    grid_animationn->setEndValue(0.5);
    grid_animationn->setKeyValueAt(0.5, 0.7);

    const auto scan_animation = new QPropertyAnimation(this, "scanlineOpacity");
    scan_animation->setDuration(500);
    scan_animation->setStartValue(scanline_opacity_);
    scan_animation->setEndValue(0.15);
    scan_animation->setKeyValueAt(0.5, 0.4);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(grid_animationn);
    group->addAnimation(scan_animation);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void VideoPlayer::OnSliderPressed() {
    was_playing_ = !decoder_->IsPaused();
    if (was_playing_) {
        decoder_->Pause();
    }
    slider_dragging_ = true;
    status_label_->setText(translator_->Translate("VideoPlayer.Seeking", "SEEKING..."));
}

void VideoPlayer::OnSliderReleased() {
    SeekVideo(progress_slider_->value());
    slider_dragging_ = false;
    if (was_playing_) {
        decoder_->Pause();
        play_button_->setText("❚❚");
        status_label_->setText(translator_->Translate("VideoPlayer.Playing", "PLAYING..."));
    } else {
        status_label_->setText(translator_->Translate("VideoPlayer.Paused", "PAUSED"));
    }
}

void VideoPlayer::UpdateTimeLabel(const int position) const {
    if (!decoder_ || video_duration_ <= 0)
        return;

    const double seek_position = position / 1000.0;
    const int seconds = static_cast<int>(seek_position) % 60;
    const int minutes = static_cast<int>(seek_position) / 60;
    const int total_seconds = static_cast<int>(video_duration_) % 60;
    const int total_minutes = static_cast<int>(video_duration_) / 60;

    time_label_->setText(
        QString("%1:%2 / %3:%4")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(total_minutes, 2, 10, QChar('0'))
        .arg(total_seconds, 2, 10, QChar('0'))
    );
}

void VideoPlayer::UpdateSliderPosition(const double position) const {
    if (video_duration_ <= 0)
        return;

    progress_slider_->setValue(position * 1000);

    const int seconds = static_cast<int>(position) % 60;
    const int minutes = static_cast<int>(position) / 60;
    const int total_seconds = static_cast<int>(video_duration_) % 60;
    const int total_minutes = static_cast<int>(video_duration_) / 60;

    time_label_->setText(
        QString("%1:%2 / %3:%4")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(total_minutes, 2, 10, QChar('0'))
        .arg(total_seconds, 2, 10, QChar('0'))
    );
}

void VideoPlayer::SeekVideo(const int position) const {
    if (!decoder_ || video_duration_ <= 0)
        return;

    const double seek_position = position / 1000.0;
    decoder_->Seek(seek_position);
}

void VideoPlayer::UpdateFrame(const QImage &frame) {
    if (thumbnail_frame_.isNull()) {
        thumbnail_frame_ = frame.copy();
    }

    const int display_width = video_label_->width();
    const int display_height = video_label_->height();

    QImage container_image(display_width, display_height, QImage::Format_RGB32);
    container_image.fill(Qt::black);

    QSize target_size = frame.size();
    target_size.scale(display_width, display_height, Qt::KeepAspectRatio);

    const QImage scaled_frame = frame.scaled(target_size.width(), target_size.height(),
                                             Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const int x_offset = (display_width - scaled_frame.width()) / 2;
    const int y_offset = (display_height - scaled_frame.height()) / 2;

    QPainter painter(&container_image);
    painter.drawImage(QPoint(x_offset, y_offset), scaled_frame);

    if (scanline_opacity_ > 0.05) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * scanline_opacity_));

        for (int y = 0; y < container_image.height(); y += 3) {
            painter.drawRect(0, y, container_image.width(), 1);
        }
    }

    if (show_hud_) {
        painter.setPen(QPen(QColor(0, 200, 255, 150), 1));
        constexpr int cornerSize = 20;

        // left-top
        painter.drawLine(5, 5, 5 + cornerSize, 5);
        painter.drawLine(5, 5, 5, 5 + cornerSize);

        // right-top
        painter.drawLine(container_image.width() - 5 - cornerSize, 5, container_image.width() - 5, 5);
        painter.drawLine(container_image.width() - 5, 5, container_image.width() - 5, 5 + cornerSize);

        // right-bottom
        painter.drawLine(container_image.width() - 5, container_image.height() - 5 - cornerSize,
                         container_image.width() - 5, container_image.height() - 5);
        painter.drawLine(container_image.width() - 5 - cornerSize, container_image.height() - 5,
                         container_image.width() - 5, container_image.height() - 5);

        // left-bottom
        painter.drawLine(5, container_image.height() - 5 - cornerSize, 5, container_image.height() - 5);
        painter.drawLine(5, container_image.height() - 5, 5 + cornerSize, container_image.height() - 5);

        painter.setFont(QFont("Consolas", 8));

        const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        painter.drawText(container_image.width() - 80, 20, timestamp);

        const int frame_number = frame_counter_ % 10000;
        painter.drawText(10, container_image.height() - 10,
                         QString("%1: %2").arg(translator_->Translate("VideoPlayer.Frame", "FRAME")).arg(
                             frame_number, 4, 10, QChar('0')));

        painter.drawText(container_image.width() - 120, container_image.height() - 10,
                         QString("%1x%2").arg(video_width_).arg(video_height_));

        frame_counter_++;
    }

    if (current_glitch_intensity_ > 0) {
        painter.setPen(Qt::NoPen);

        for (int i = 0; i < current_glitch_intensity_ * 20; ++i) {
            const int glitch_height = QRandomGenerator::global()->bounded(1, 5);
            const int glitch_y = QRandomGenerator::global()->bounded(container_image.height());
            const int glitch_x = QRandomGenerator::global()->bounded(container_image.width());
            const int glitch_width = QRandomGenerator::global()->bounded(20, 100);

            QColor glitch_color(
                QRandomGenerator::global()->bounded(0, 255),
                QRandomGenerator::global()->bounded(0, 255),
                QRandomGenerator::global()->bounded(0, 255),
                150
            );

            painter.setBrush(glitch_color);
            painter.drawRect(glitch_x, glitch_y, glitch_width, glitch_height);
        }

        current_glitch_intensity_ *= 0.9;
        if (current_glitch_intensity_ < 0.1) {
            current_glitch_intensity_ = 0;
        }
    }

    video_label_->setPixmap(QPixmap::fromImage(container_image));
}

void VideoPlayer::UpdateUI() {
    static qreal pulse = 0;
    pulse += 0.05;

    if (playback_started_ && !playback_finished_ && !decoder_->IsPaused()) {
        const qreal pulse_factor = 0.05 * sin(pulse);
        SetScanlineOpacity(0.15 + pulse_factor);
        SetGridOpacity(0.1 + pulse_factor);
    }

    if (decoder_ && playback_started_ && !status_label_->text().startsWith("ERROR")) {
        const int random_update = QRandomGenerator::global()->bounded(100);

        if (random_update < 2 && !decoder_->IsPaused()) {
            status_label_->setText(
                QString("%1: %2%").arg(translator_->Translate("VideoPlayer.FrameBuffer", "FRAME BUFFER")).arg(
                    QRandomGenerator::global()->bounded(90, 100)));
        } else if (random_update < 4 && !decoder_->IsPaused()) {
            status_label_->setText(
                QString("%1: %2%").arg(translator_->Translate("VideoPlayer.SignalStrength", "SIGNAL STRENGTH")).arg(
                    QRandomGenerator::global()->bounded(85, 99)));
        }
    }

    update();
}

void VideoPlayer::HandleError(const QString &message) const {
    qDebug() << "[VIDEO PLAYER] Video decoder error:" << message;
    status_label_->setText("ERROR: " + message);
    video_label_->setText("⚠️ " + message);
}

void VideoPlayer::HandleVideoInfo(const int width, const int height, const double fps, const double duration) {
    playback_started_ = true;
    video_width_ = width;
    video_height_ = height;
    video_duration_ = duration;
    video_fps_ = fps > 0 ? fps : 30; // default fps if weren't provided (nor recognized)

    progress_slider_->setRange(0, duration * 1000);

    QTimer::singleShot(100, this, [this] {
        if (decoder_ && !decoder_->isFinished()) {
            decoder_->ExtractFirstFrame();
        }
    });

    resolution_label_->setText(QString("RES: %1x%2").arg(width).arg(height));
    bitrate_label_->setText(QString("BITRATE: %1k").arg(QRandomGenerator::global()->bounded(800, 2500)));
    fps_label_->setText(QString("FPS: %1").arg(qRound(video_fps_)));
    codec_label_->setText("CODEC: AV1/H.265");

    if (update_timer_ && video_fps_ > 0) {
        const int interval = qMax(16, qRound(1000 / video_fps_));
        update_timer_->setInterval(interval);
    }

    status_label_->setText(translator_->Translate("VideoPlayer.Ready", "READY"));

    show_hud_ = true;
}

void VideoPlayer::AdjustVolume(const int volume) const {
    if (!decoder_) return;

    const float normalized_volume = volume / 100.0f;
    decoder_->SetVolume(normalized_volume);
    UpdateVolumeIcon(normalized_volume);
}

void VideoPlayer::ToggleMute() {
    if (!decoder_) return;

    if (volume_slider_->value() > 0) {
        last_volume_ = volume_slider_->value();
        volume_slider_->setValue(0);
    } else {
        volume_slider_->setValue(last_volume_ > 0 ? last_volume_ : 100);
    }
}

void VideoPlayer::UpdateVolumeIcon(const float volume) const {
    if (volume <= 0.01f) {
        volume_button_->setText("🔇");
    } else if (volume < 0.5f) {
        volume_button_->setText("🔉");
    } else {
        volume_button_->setText("🔊");
    }
}

void VideoPlayer::TriggerGlitch() {
    current_glitch_intensity_ = QRandomGenerator::global()->bounded(10, 50) / 100.0;
    glitch_timer_->setInterval(QRandomGenerator::global()->bounded(3000, 10000));
}
