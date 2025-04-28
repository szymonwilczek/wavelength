#ifndef INLINE_IMAGE_VIEWER_H
#define INLINE_IMAGE_VIEWER_H

#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <memory>
#include <QApplication>

#include "../decoder/image_decoder.h"

// Klasa wyświetlacza statycznych obrazów zintegrowanego z czatem
class InlineImageViewer final : public QFrame {
    Q_OBJECT

public:
    explicit InlineImageViewer(const QByteArray& imageData, QWidget* parent = nullptr);

    ~InlineImageViewer() override {
        releaseResources();
    }

    void releaseResources();

    // Zwraca oryginalny rozmiar obrazu jako wskazówkę
    QSize sizeHint() const override;

    // Usunięto eventFilter

private slots:
    void loadImage() const {
        if (!m_decoder) return;
        m_decoder->decode();
    }

    // Usunięto toggleZoom
    // Usunięto displayFullSizeImage

    void handleImageReady(const QImage& image);

    // Usunięto displayScaledImage

    void handleError(const QString& message);

    void handleImageInfo(int width, int height, bool hasAlpha);

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