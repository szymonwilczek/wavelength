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

#include "../decoder/image_decoder.h"

// Klasa wyświetlacza statycznych obrazów zintegrowanego z czatem
class InlineImageViewer : public QFrame {
    Q_OBJECT

public:
    InlineImageViewer(const QByteArray& imageData, QWidget* parent = nullptr)
        : QFrame(parent), m_imageData(imageData), m_isZoomed(false) {
        
        setFrameStyle(QFrame::StyledPanel);
        setStyleSheet("background-color: #1a1a1a; border: 1px solid #444;");
        
        // Początkowy rozmiar będzie dostosowany po otrzymaniu obrazu
        setMaximumSize(600, 600);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Obszar wyświetlania obrazu
        m_imageLabel = new QLabel(this);
        m_imageLabel->setAlignment(Qt::AlignCenter);
        m_imageLabel->setStyleSheet("background-color: #1a1a1a; color: #ffffff;");
        m_imageLabel->setText("Ładowanie obrazu...");
        layout->addWidget(m_imageLabel);

        // Dekoder obrazu
        m_decoder = std::make_shared<ImageDecoder>(imageData, this);

        // Połącz sygnały
        connect(m_decoder.get(), &ImageDecoder::imageReady, this, &InlineImageViewer::handleImageReady);
        connect(m_decoder.get(), &ImageDecoder::error, this, &InlineImageViewer::handleError);
        connect(m_decoder.get(), &ImageDecoder::imageInfo, this, &InlineImageViewer::handleImageInfo);
        
        // Instalacja filtra zdarzeń dla obsługi kliknięć
        m_imageLabel->installEventFilter(this);
        m_imageLabel->setCursor(Qt::PointingHandCursor);
        
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
        }
    }

    // Obsługa kliknięcia w obraz (powiększenie/pomniejszenie)
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_imageLabel && event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                toggleZoom();
                return true;
            }
        }
        return QFrame::eventFilter(watched, event);
    }

private slots:
    void loadImage() {
        if (!m_decoder) return;
        
        // Dekodowanie obrazu
        m_decoder->decode();
        // Wynik zostanie odebrany przez sygnał imageReady
    }

    void toggleZoom() {
        m_isZoomed = !m_isZoomed;
        
        if (m_isZoomed) {
            // Pokaż pełny rozmiar obrazu (lub maksymalny dozwolony)
            displayFullSizeImage();
        } else {
            // Pokaż skalowany obraz
            displayScaledImage(m_originalImage);
        }
    }

    void handleImageReady(const QImage& image) {
        m_originalImage = image;
        
        // Domyślnie wyświetl skalowany obraz
        displayScaledImage(image);
    }

    void displayScaledImage(const QImage& image) {
        // Obliczamy rozmiar skalowanego obrazu z zachowaniem proporcji
        QSize targetSize = image.size();
        int maxWidth = std::min(image.width(), 600);  // Maksymalna szerokość obrazu
        int maxHeight = std::min(image.height(), 600);  // Maksymalna wysokość obrazu
        
        targetSize.scale(maxWidth, maxHeight, Qt::KeepAspectRatio);

        // Skalujemy oryginalny obraz tylko jeśli trzeba
        QImage scaledImage;
        if (image.width() > maxWidth || image.height() > maxHeight) {
            scaledImage = image.scaled(targetSize.width(), targetSize.height(),
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            scaledImage = image;  // Użyj oryginalnego rozmiaru
        }

        // Aktualizujemy rozmiar widgetu do rozmiaru obrazu
        setFixedSize(scaledImage.width(), scaledImage.height());

        // Wyświetlamy obraz
        m_imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
        m_imageLabel->setFixedSize(scaledImage.size());
    }

    void displayFullSizeImage() {
        // W przypadku dużych obrazów, ograniczamy maksymalny rozmiar widoku
        QSize originalSize = m_originalImage.size();
        int maxViewWidth = std::min(originalSize.width(), 1200);  // Max szerokość powiększonego widoku
        int maxViewHeight = std::min(originalSize.height(), 800); // Max wysokość powiększonego widoku
        
        QSize viewSize = originalSize;
        if (originalSize.width() > maxViewWidth || originalSize.height() > maxViewHeight) {
            viewSize.scale(maxViewWidth, maxViewHeight, Qt::KeepAspectRatio);
        }
        
        QImage scaledForView;
        if (viewSize != originalSize) {
            scaledForView = m_originalImage.scaled(viewSize.width(), viewSize.height(),
                                            Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            scaledForView = m_originalImage;
        }
        
        // Aktualizujemy rozmiar widgetu
        setFixedSize(viewSize.width(), viewSize.height());
        
        // Wyświetlamy obraz
        m_imageLabel->setPixmap(QPixmap::fromImage(scaledForView));
        m_imageLabel->setFixedSize(viewSize);
    }

    void handleError(const QString& message) {
        qDebug() << "Image decoder error:" << message;
        m_imageLabel->setText("⚠️ " + message);
    }

    void handleImageInfo(int width, int height, bool hasAlpha) {
        m_imageWidth = width;
        m_imageHeight = height;
        m_hasAlpha = hasAlpha;
        
        qDebug() << "Image info - szerokość:" << width << "wysokość:" << height 
                 << "kanał alfa:" << (hasAlpha ? "tak" : "nie");
        
        // Możemy tutaj wysłać dodatkowy sygnał z informacjami o obrazie
        emit imageInfoReady(width, height, hasAlpha);
    }

signals:
    void imageInfoReady(int width, int height, bool hasAlpha);

private:
    QLabel* m_imageLabel;
    std::shared_ptr<ImageDecoder> m_decoder;
    QByteArray m_imageData;
    QImage m_originalImage;
    
    int m_imageWidth = 0;
    int m_imageHeight = 0;
    bool m_hasAlpha = false;
    bool m_isZoomed = false;
};

#endif // INLINE_IMAGE_VIEWER_H