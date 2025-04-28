//
// Created by szymo on 17.03.2025.
//

#include "image_viewer.h"

InlineImageViewer::InlineImageViewer(const QByteArray &imageData, QWidget *parent): QFrame(parent), m_imageData(imageData) { // Usunięto m_isZoomed

    // Usunięto setFrameStyle i setStyleSheet - styl będzie dziedziczony lub ustawiany z zewnątrz
    // Usunięto setMaximumSize

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Obszar wyświetlania obrazu
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: transparent; color: #ffffff;"); // Ustawiamy przezroczyste tło
    m_imageLabel->setText("Ładowanie obrazu...");
    m_imageLabel->setScaledContents(true); // Kluczowa zmiana: Włącz skalowanie zawartości QLabel
    layout->addWidget(m_imageLabel);

    // Dekoder obrazu
    m_decoder = std::make_shared<ImageDecoder>(imageData, this);

    // Połącz sygnały
    connect(m_decoder.get(), &ImageDecoder::imageReady, this, &InlineImageViewer::handleImageReady);
    connect(m_decoder.get(), &ImageDecoder::error, this, &InlineImageViewer::handleError);
    connect(m_decoder.get(), &ImageDecoder::imageInfo, this, &InlineImageViewer::handleImageInfo);

    // Usunięto instalację filtra zdarzeń dla zoomu
    // Usunięto setCursor

    // Obsługa zamykania aplikacji
    connect(qApp, &QApplication::aboutToQuit, this, &InlineImageViewer::releaseResources);

    // Dekoduj obraz
    QTimer::singleShot(0, this, &InlineImageViewer::loadImage);
}

void InlineImageViewer::releaseResources() {
    if (m_decoder) {
        m_decoder->releaseResources();
        m_decoder.reset(); // Zwalniamy wskaźnik
    }
}

QSize InlineImageViewer::sizeHint() const {
    if (!m_originalImage.isNull()) {
        return m_originalImage.size();
    }
    return QFrame::sizeHint(); // Zwróć domyślny, jeśli obraz nie jest załadowany
}

void InlineImageViewer::handleImageReady(const QImage &image) {
    if (image.isNull()) {
        handleError("Otrzymano pusty obraz z dekodera.");
        return;
    }
    m_originalImage = image;
    qDebug() << "InlineImageViewer::handleImageReady - Oryginalny rozmiar obrazu:" << m_originalImage.size();

    // Ustaw pixmapę na QLabel - skalowanie załatwi setScaledContents(true)
    m_imageLabel->setPixmap(QPixmap::fromImage(m_originalImage));

    // Nie ustawiamy już setFixedSize - pozwalamy layoutowi zarządzać rozmiarem
    // m_imageLabel->setFixedSize(m_originalImage.size()); // Usunięto
    // setFixedSize(m_originalImage.size()); // Usunięto

    updateGeometry(); // Informujemy layout o potencjalnej zmianie rozmiaru
    emit imageLoaded(); // Sygnalizujemy załadowanie obrazu
}

void InlineImageViewer::handleError(const QString &message) {
    qDebug() << "Image decoder error:" << message;
    m_imageLabel->setText("⚠️ " + message);
    // Można ustawić minimalny rozmiar, aby tekst błędu był widoczny
    setMinimumSize(100, 50);
    updateGeometry();
}

void InlineImageViewer::handleImageInfo(int width, int height, bool hasAlpha) {
    m_imageWidth = width;
    m_imageHeight = height;
    m_hasAlpha = hasAlpha;

    qDebug() << "Image info - szerokość:" << width << "wysokość:" << height
            << "kanał alfa:" << (hasAlpha ? "tak" : "nie");

    emit imageInfoReady(width, height, hasAlpha);
}
