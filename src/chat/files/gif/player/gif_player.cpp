#include "gif_player.h"

#include <QApplication>
#include <QTimer>
#include <QVBoxLayout>

#include "../../../../app/managers/translation_manager.h"

GifPlayer::GifPlayer(const QByteArray &gif_data, QWidget *parent): QFrame(parent), gif_data_(gif_data) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    translator_ = TranslationManager::GetInstance();

    gif_label_ = new QLabel(this);
    gif_label_->setAlignment(Qt::AlignCenter);
    gif_label_->setStyleSheet("background-color: transparent; color: #ffffff;");
    gif_label_->setText(translator_->Translate("GifPlayer.Loading", "LOADING GIF..."));
    gif_label_->setScaledContents(true);
    layout->addWidget(gif_label_);

    decoder_ = std::make_shared<GifDecoder>(gif_data, this);

    connect(decoder_.get(), &GifDecoder::firstFrameReady, this, &GifPlayer::DisplayThumbnail,
            Qt::QueuedConnection);
    connect(decoder_.get(), &GifDecoder::frameReady, this, &GifPlayer::UpdateFrame, Qt::QueuedConnection);
    connect(decoder_.get(), &GifDecoder::error, this, &GifPlayer::HandleError, Qt::QueuedConnection);
    connect(decoder_.get(), &GifDecoder::gifInfo, this, &GifPlayer::HandleGifInfo, Qt::QueuedConnection);

    QTimer::singleShot(0, this, [this]() {
        if (decoder_) {
            if (!decoder_->Initialize()) {
                qDebug() << "[INLINE GIF PLAYER] Decoder initialization failed.";
            }
        }
    });

    connect(qApp, &QApplication::aboutToQuit, this, &GifPlayer::ReleaseResources);

    setMouseTracking(true);
    gif_label_->setMouseTracking(true);
}

void GifPlayer::ReleaseResources() {
    if (decoder_) {
        decoder_->Stop();
        if (decoder_->IsDecoderRunning()) {
            decoder_->wait(500);
        }
        decoder_.reset();
        is_playing_ = false;
    }
}

QSize GifPlayer::sizeHint() const {
    if (gif_width_ > 0 && gif_height_ > 0) {
        return QSize(gif_width_, gif_height_);
    }
    return QFrame::sizeHint();
}

void GifPlayer::StartPlayback() {
    if (decoder_ && !is_playing_) {
        is_playing_ = true;
        decoder_->Resume();
    }
}

void GifPlayer::StopPlayback() {
    if (decoder_ && is_playing_) {
        is_playing_ = false;
        decoder_->Pause();
    }
}

void GifPlayer::enterEvent(QEvent *event) {
    StartPlayback();
    QFrame::enterEvent(event);
}

void GifPlayer::leaveEvent(QEvent *event) {
    StopPlayback();
    QFrame::leaveEvent(event);
}

void GifPlayer::DisplayThumbnail(const QImage &frame) {
    if (!frame.isNull()) {
        thumbnail_pixmap_ = QPixmap::fromImage(frame);
        gif_label_->setPixmap(thumbnail_pixmap_.scaled(
            gif_label_->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    } else {
        gif_label_->setText(translator_->Translate("GifPlayer.ThumbnailError", "⚠️ THUMBNAIL LOADING ERROR..."));
    }
}

void GifPlayer::UpdateFrame(const QImage &frame) const {
    if (frame.isNull() || !is_playing_) return;
    gif_label_->setPixmap(QPixmap::fromImage(frame));
}

void GifPlayer::HandleError(const QString &message) {
    qDebug() << "[INLINE GIF PLAYER] GIF decoder error:" << message;
    gif_label_->setText("⚠️ " + message);
    setMinimumSize(100, 50);
    updateGeometry();
}

void GifPlayer::HandleGifInfo(const int width, const int height, const double duration, const double frame_rate) {
    gif_width_ = width;
    gif_height_ = height;
    gif_duration_ = duration;
    frame_rate_ = frame_rate;

    updateGeometry();
    emit gifLoaded();

    if (!thumbnail_pixmap_.isNull() && !is_playing_) {
        gif_label_->setPixmap(thumbnail_pixmap_.scaled(
            gif_label_->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }
}
