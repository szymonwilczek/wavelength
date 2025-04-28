#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#include <QImage>
#include <QDebug>
#include <QBuffer>
#include <QObject>

class ImageDecoder : public QObject {
    Q_OBJECT

public:
    ImageDecoder(const QByteArray& imageData, QObject* parent = nullptr)
        : QObject(parent), m_imageData(imageData) {
    }

    ~ImageDecoder() {
        // Nie wymaga specjalnego zwalniania zasobów jak w FFmpeg
    }

    void releaseResources() {
        // Nic do zwalniania w tej implementacji
    }

    QImage decode();

signals:
        void imageReady(const QImage& image);
    void error(const QString& message);
    void imageInfo(int width, int height, bool hasAlpha);

private:
    QByteArray m_imageData;
};

#endif // IMAGE_DECODER_H