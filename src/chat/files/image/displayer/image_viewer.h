#ifndef INLINE_IMAGE_VIEWER_H
#define INLINE_IMAGE_VIEWER_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <memory>
#include <QApplication>
#include <QDebug> // Dodano dla logowania

#include "../decoder/image_decoder.h"

// Klasa wyświetlacza statycznych obrazów zintegrowanego z czatem
class InlineImageViewer : public QFrame {
    Q_OBJECT

public:
    InlineImageViewer(const QByteArray& imageData, QWidget* parent = nullptr)
        : QFrame(parent), m_imageData(imageData) { // Usunięto m_isZoomed

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

    ~InlineImageViewer() {
        releaseResources();
    }

    void releaseResources() {
        if (m_decoder) {
            m_decoder->releaseResources();
            m_decoder.reset(); // Zwalniamy wskaźnik
        }
    }

    // Zwraca oryginalny rozmiar obrazu jako wskazówkę
    QSize sizeHint() const override {
        if (!m_originalImage.isNull()) {
            return m_originalImage.size();
        }
        return QFrame::sizeHint(); // Zwróć domyślny, jeśli obraz nie jest załadowany
    }

    // Usunięto eventFilter

private slots:
    void loadImage() {
        if (!m_decoder) return;
        m_decoder->decode();
    }

    // Usunięto toggleZoom
    // Usunięto displayFullSizeImage

    void handleImageReady(const QImage& image) {
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

    // Usunięto displayScaledImage

    void handleError(const QString& message) {
        qDebug() << "Image decoder error:" << message;
        m_imageLabel->setText("⚠️ " + message);
        // Można ustawić minimalny rozmiar, aby tekst błędu był widoczny
        setMinimumSize(100, 50);
        updateGeometry();
    }

    void handleImageInfo(int width, int height, bool hasAlpha) {
        m_imageWidth = width;
        m_imageHeight = height;
        m_hasAlpha = hasAlpha;

        qDebug() << "Image info - szerokość:" << width << "wysokość:" << height
                 << "kanał alfa:" << (hasAlpha ? "tak" : "nie");

        emit imageInfoReady(width, height, hasAlpha);
    }

signals:
    void imageInfoReady(int width, int height, bool hasAlpha);
    void imageLoaded(); // Nowy sygnał informujący o załadowaniu

private:
    QLabel* m_imageLabel;
    std::shared_ptr<ImageDecoder> m_decoder;
    QByteArray m_imageData;
    QImage m_originalImage;

    int m_imageWidth = 0;
    int m_imageHeight = 0;
    bool m_hasAlpha = false;
    // Usunięto m_isZoomed
};

#endif // INLINE_IMAGE_VIEWER_H