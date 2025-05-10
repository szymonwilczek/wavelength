#include "image_decoder.h"

#include <QImage>

QImage ImageDecoder::Decode() {
    QImage image;

    if (!image.loadFromData(image_data_)) {
        emit error("[IMAGE DECODER] Unable to load image - invalid file format.");
        return QImage();
    }

    if (image.isNull()) {
        emit error("[IMAGE DECODER] Image decoding failed.");
        return QImage();
    }

    emit imageInfo(image.width(), image.height(), image.hasAlphaChannel());
    emit imageReady(image);

    return image;
}
