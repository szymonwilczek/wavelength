#include "image_viewer.h"

#include <QVBoxLayout>

#include "../../../../app/managers/translation_manager.h"

InlineImageViewer::InlineImageViewer(const QByteArray &image_data, QWidget *parent): QFrame(parent),
    image_data_(image_data) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    translator_ = TranslationManager::GetInstance();

    image_label_ = new QLabel(this);
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setStyleSheet("background-color: transparent; color: #ffffff;");
    image_label_->setText(translator_->Translate("ImageViewer.Loading", "LOADING IMAGE..."));
    image_label_->setScaledContents(true);
    layout->addWidget(image_label_);

    decoder_ = std::make_shared<ImageDecoder>(image_data, this);

    connect(decoder_.get(), &ImageDecoder::imageReady, this, &InlineImageViewer::HandleImageReady);
    connect(decoder_.get(), &ImageDecoder::error, this, &InlineImageViewer::HandleError);
    connect(decoder_.get(), &ImageDecoder::imageInfo, this, &InlineImageViewer::HandleImageInfo);

    connect(qApp, &QApplication::aboutToQuit, this, &InlineImageViewer::ReleaseResources);

    QTimer::singleShot(0, this, &InlineImageViewer::LoadImage);
}

void InlineImageViewer::ReleaseResources() {
    if (decoder_) {
        decoder_->ReleaseResources();
        decoder_.reset();
    }
}

QSize InlineImageViewer::sizeHint() const {
    if (!original_image_.isNull()) {
        return original_image_.size();
    }
    return QFrame::sizeHint();
}

void InlineImageViewer::HandleImageReady(const QImage &image) {
    if (image.isNull()) {
        HandleError(translator_->Translate("ImageViewer.EmptyImage", "⚠️ Viewer got an empty image from decoder."));
        return;
    }
    original_image_ = image;
    image_label_->setPixmap(QPixmap::fromImage(original_image_));

    updateGeometry();
    emit imageLoaded();
}

void InlineImageViewer::HandleError(const QString &message) {
    qDebug() << "[IMAGE VIEWER] Image decoder error:" << message;
    image_label_->setText(translator_->Translate("ImageViewer.DecoderError", "⚠️ IMAGE DECODER ERROR..."));
    setMinimumSize(100, 50);
    updateGeometry();
}

void InlineImageViewer::HandleImageInfo(const int width, const int height, const bool has_alpha) {
    image_width_ = width;
    image_height_ = height;
    has_alpha_ = has_alpha;

    emit imageInfoReady(width, height, has_alpha);
}
