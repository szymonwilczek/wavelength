#include "image_decoder.h"

QImage ImageDecoder::Decode() {
    QImage image;

    // Próbujemy bezpośrednio załadować obraz z danych binarnych
    if (!image.loadFromData(image_data_)) {
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
