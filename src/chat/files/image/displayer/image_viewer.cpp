#include "image_viewer.h"

#include <QVBoxLayout>

#include "../../../../app/managers/translation_manager.h"

ImageViewer::ImageViewer(const QByteArray &image_data, QWidget *parent): QFrame(parent),
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

    connect(decoder_.get(), &ImageDecoder::imageReady, this, &ImageViewer::HandleImageReady);
    connect(decoder_.get(), &ImageDecoder::error, this, &ImageViewer::HandleError);
    connect(decoder_.get(), &ImageDecoder::imageInfo, this, &ImageViewer::HandleImageInfo);

    connect(qApp, &QApplication::aboutToQuit, this, &ImageViewer::ReleaseResources);

    QTimer::singleShot(0, this, &ImageViewer::LoadImage);
}

void ImageViewer::ReleaseResources() {
    if (decoder_) {
        decoder_->ReleaseResources();
        decoder_.reset();
    }
}

QSize ImageViewer::sizeHint() const {
    if (!original_image_.isNull()) {
        return original_image_.size();
    }
    return QFrame::sizeHint();
}

void ImageViewer::HandleImageReady(const QImage &image) {
    if (image.isNull()) {
        HandleError(translator_->Translate("ImageViewer.EmptyImage", "⚠️ Viewer got an empty image from decoder."));
        return;
    }
    original_image_ = image;
    image_label_->setPixmap(QPixmap::fromImage(original_image_));

    updateGeometry();
    emit imageLoaded();
}

void ImageViewer::HandleError(const QString &message) {
    qDebug() << "[IMAGE VIEWER] Image decoder error:" << message;
    image_label_->setText(translator_->Translate("ImageViewer.DecoderError", "⚠️ IMAGE DECODER ERROR..."));
    setMinimumSize(100, 50);
    updateGeometry();
}

void ImageViewer::HandleImageInfo(const int width, const int height, const bool has_alpha) {
    image_width_ = width;
    image_height_ = height;
    has_alpha_ = has_alpha;

    emit imageInfoReady(width, height, has_alpha);
}
