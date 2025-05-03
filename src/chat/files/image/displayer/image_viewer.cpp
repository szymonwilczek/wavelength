#include "image_viewer.h"

#include <QVBoxLayout>

InlineImageViewer::InlineImageViewer(const QByteArray &image_data, QWidget *parent): QFrame(parent), image_data_(image_data) { // Usunięto m_isZoomed

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Obszar wyświetlania obrazu
    image_label_ = new QLabel(this);
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setStyleSheet("background-color: transparent; color: #ffffff;"); // Ustawiamy przezroczyste tło
    image_label_->setText("Ładowanie obrazu...");
    image_label_->setScaledContents(true); // Kluczowa zmiana: Włącz skalowanie zawartości QLabel
    layout->addWidget(image_label_);

    // Dekoder obrazu
    decoder_ = std::make_shared<ImageDecoder>(image_data, this);

    // Połącz sygnały
    connect(decoder_.get(), &ImageDecoder::imageReady, this, &InlineImageViewer::HandleImageReady);
    connect(decoder_.get(), &ImageDecoder::error, this, &InlineImageViewer::HandleError);
    connect(decoder_.get(), &ImageDecoder::imageInfo, this, &InlineImageViewer::HandleImageInfo);

    // Usunięto instalację filtra zdarzeń dla zoomu
    // Usunięto setCursor

    // Obsługa zamykania aplikacji
    connect(qApp, &QApplication::aboutToQuit, this, &InlineImageViewer::ReleaseResources);

    // Dekoduj obraz
    QTimer::singleShot(0, this, &InlineImageViewer::LoadImage);
}

void InlineImageViewer::ReleaseResources() {
    if (decoder_) {
        decoder_->ReleaseResources();
        decoder_.reset(); // Zwalniamy wskaźnik
    }
}

QSize InlineImageViewer::sizeHint() const {
    if (!original_image_.isNull()) {
        return original_image_.size();
    }
    return QFrame::sizeHint(); // Zwróć domyślny, jeśli obraz nie jest załadowany
}

void InlineImageViewer::HandleImageReady(const QImage &image) {
    if (image.isNull()) {
        HandleError("Otrzymano pusty obraz z dekodera.");
        return;
    }
    original_image_ = image;

    // Ustaw pixmapę na QLabel - skalowanie załatwi setScaledContents(true)
    image_label_->setPixmap(QPixmap::fromImage(original_image_));

    // Nie ustawiamy już setFixedSize - pozwalamy layoutowi zarządzać rozmiarem
    // m_imageLabel->setFixedSize(m_originalImage.size()); // Usunięto
    // setFixedSize(m_originalImage.size()); // Usunięto

    updateGeometry(); // Informujemy layout o potencjalnej zmianie rozmiaru
    emit imageLoaded(); // Sygnalizujemy załadowanie obrazu
}

void InlineImageViewer::HandleError(const QString &message) {
    qDebug() << "Image decoder error:" << message;
    image_label_->setText("⚠️ " + message);
    // Można ustawić minimalny rozmiar, aby tekst błędu był widoczny
    setMinimumSize(100, 50);
    updateGeometry();
}

void InlineImageViewer::HandleImageInfo(const int width, const int height, const bool has_alpha) {
    image_width_ = width;
    image_height_ = height;
    has_alpha_ = has_alpha;

    emit imageInfoReady(width, height, has_alpha);
}
