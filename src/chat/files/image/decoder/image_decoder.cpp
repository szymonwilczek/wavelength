//
// Created by szymo on 17.03.2025.
//

#include "image_decoder.h"

QImage ImageDecoder::decode() {
    QImage image;

    // Próbujemy bezpośrednio załadować obraz z danych binarnych
    if (!image.loadFromData(m_imageData)) {
        emit error("Nie można załadować obrazu - nieprawidłowy format pliku");
        return QImage();
    }

    // Sprawdzenie czy obraz został poprawnie załadowany
    if (image.isNull()) {
        emit error("Dekodowanie obrazu nie powiodło się");
        return QImage();
    }

    // Emitujemy sygnał z informacjami o obrazie
    emit imageInfo(image.width(), image.height(), image.hasAlphaChannel());
    emit imageReady(image);

    return image;
}
