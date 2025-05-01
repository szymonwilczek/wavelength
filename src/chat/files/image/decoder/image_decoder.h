#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#include <QImage>
#include <QDebug>
#include <QObject>

class ImageDecoder final : public QObject {
    Q_OBJECT

public:
    explicit ImageDecoder(const QByteArray& image_data, QObject* parent = nullptr)
        : QObject(parent), image_data_(image_data) {
    }

    ~ImageDecoder() override {
        // Nie wymaga specjalnego zwalniania zasobów jak w FFmpeg
    }

    static void ReleaseResources() {
        // Nic do zwalniania w tej implementacji
    }

    QImage Decode();

    signals:
    void imageReady(const QImage& image);
    void error(const QString& message);
    void imageInfo(int width, int height, bool has_alpha);

private:
    QByteArray image_data_;
};

#endif // IMAGE_DECODER_H