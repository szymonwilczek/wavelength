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
    explicit InlineImageViewer(const QByteArray& image_data, QWidget* parent = nullptr);

    ~InlineImageViewer() override {
        ReleaseResources();
    }

    void ReleaseResources();

    // Zwraca oryginalny rozmiar obrazu jako wskazówkę
    QSize sizeHint() const override;

private slots:
    void LoadImage() const {
        if (!decoder_) return;
        decoder_->Decode();
    }

    void HandleImageReady(const QImage& image);

    void HandleError(const QString& message);

    void HandleImageInfo(int width, int height, bool has_alpha);

signals:
    void imageInfoReady(int width, int height, bool has_alpha);
    void imageLoaded();

private:
    QLabel* image_label_;
    std::shared_ptr<ImageDecoder> decoder_;
    QByteArray image_data_;
    QImage original_image_;

    int image_width_ = 0;
    int image_height_ = 0;
    bool has_alpha_ = false;
};

#endif // INLINE_IMAGE_VIEWER_H