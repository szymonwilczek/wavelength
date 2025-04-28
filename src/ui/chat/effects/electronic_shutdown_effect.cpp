#include "electronic_shutdown_effect.h"

#include <qmath.h>

ElectronicShutdownEffect::ElectronicShutdownEffect(QObject *parent): QGraphicsEffect(parent), m_progress(0.0), m_lastProgress(-1.0),
                                                                     m_resultCache(5), // Mała pamięć podręczna (tylko 5 klatek)
                                                                     m_randomSeed(QRandomGenerator::global()->generate()) {
}

void ElectronicShutdownEffect::setProgress(qreal progress) {
    m_progress = qBound(0.0, progress, 1.0);
    update();
}

void ElectronicShutdownEffect::draw(QPainter *painter) {
    if (m_progress < 0.01) {
        // Jeśli animacja się nie rozpoczęła, rysuj normalny obraz
        drawSource(painter);
        return;
    }

    if (m_progress > 0.99) {
        // Jeśli animacja się zakończyła, nie rysuj niczego
        return;
    }

    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::NoPad);
    if (pixmap.isNull())
        return;

    // Sprawdź czy mamy już ten efekt w cache
    QString cacheKey = QString("%1").arg(m_progress, 0, 'f', 2);
    QPixmap* cachedResult = m_resultCache.object(cacheKey);

    if (cachedResult == nullptr || qAbs(m_lastProgress - m_progress) > 0.05) {
        // Generowanie rezultatu tylko jeśli go nie ma w cache, lub progress
        // znacząco się zmienił (dla płynniejszej animacji)
        QPixmap result(pixmap.size());
        result.fill(Qt::transparent);

        // Używamy bloku try-catch aby upewnić się że painter zostanie zamknięty
        QPainter resultPainter(&result);
        try {
            resultPainter.setRenderHint(QPainter::Antialiasing);
            resultPainter.setRenderHint(QPainter::SmoothPixmapTransform);

            int w = pixmap.width();
            int h = pixmap.height();
            int centerY = h / 2;

            // Faza 1 (0.0-0.7): Zwężanie obrazu do poziomej linii
            // Faza 2 (0.7-1.0): Zanikanie poziomej linii

            if (m_progress < 0.7) {
                qreal normalizedProgress = m_progress / 0.7; // 0.0 - 1.0 dla pierwszej fazy

                // Wysokość zwężającego się obrazu - nieliniowe zmniejszanie dla lepszego efektu
                int newHeight = h * (1.0 - qPow(normalizedProgress, 1.8));

                // Zabezpieczenie przed ujemnymi wartościami
                newHeight = qMax(2, newHeight);

                // Dodajemy losowe zakłócenia wysokości z bezpiecznym sprawdzeniem zakresu
                if (normalizedProgress > 0.4) {
                    int glitchAmplitude = qMax(1, static_cast<int>(normalizedProgress * 15));
                    int glitchOffset = QRandomGenerator::global()->bounded(-glitchAmplitude, glitchAmplitude+1);
                    newHeight = qMax(2, newHeight + glitchOffset);
                }

                // Obliczamy obszar źródłowy i docelowy
                QRect sourceRect = pixmap.rect();
                QRect targetRect(0, centerY - newHeight / 2, w, newHeight);

                // Rysujemy zwężony obraz
                resultPainter.drawPixmap(targetRect, pixmap, sourceRect);

                // Dodajemy losowe przesunięcia poziomych linii (zakłócenia)
                if (normalizedProgress > 0.3) {
                    // Zabezpieczone parametry losowe
                    int numGlitches = qMin(5, qMax(1, static_cast<int>(normalizedProgress * 15)));

                    for (int i = 0; i < numGlitches; ++i) {
                        // Bezpieczne losowanie wartości
                        int min_y = targetRect.top();
                        int max_y = targetRect.bottom();

                        if (min_y >= max_y) {
                            // Zabezpieczenie przed nieprawidłowym zakresem
                            continue;
                        }

                        int glitchY = QRandomGenerator::global()->bounded(min_y, max_y + 1);

                        int minWidth = qMax(1, w / 4);
                        int maxWidth = qMax(minWidth + 1, w);
                        int glitchWidth = QRandomGenerator::global()->bounded(minWidth, maxWidth);

                        int glitchOffset = QRandomGenerator::global()->bounded(-10, 11); // -10 do 10

                        int glitchHeight = qMax(1, QRandomGenerator::global()->bounded(1, qMax(2, newHeight / 20) + 1));

                        if (glitchY + glitchHeight <= max_y) {
                            QRect glitchSource(0, glitchY, glitchWidth, glitchHeight);

                            // Przesuwamy fragment obrazu
                            resultPainter.drawPixmap(
                                QPoint(glitchOffset, glitchY),
                                result,
                                glitchSource
                            );
                        }
                    }
                }

                // Dodajemy linie CRT (scanlines)
                QPen scanlinePen(QColor(0, 0, 0, 40));
                resultPainter.setPen(scanlinePen);
                for (int y = targetRect.top(); y < targetRect.bottom(); y += 2) {
                    resultPainter.drawLine(0, y, w, y);
                }

                // Pulsujące bliknięcie (mrugnięcie)
                if (normalizedProgress > 0.5) {
                    qreal flickerChance = (normalizedProgress - 0.5) * 2.0; // 0.0 - 1.0
                    if (QRandomGenerator::global()->generateDouble() < flickerChance * 0.1) {
                        // Dodaj przezroczyste białe nakładki dla efektu migotania
                        resultPainter.setCompositionMode(QPainter::CompositionMode_Plus);
                        resultPainter.fillRect(targetRect, QColor(80, 80, 80, 30));
                    }
                }
            } else {
                // Druga faza - zanikanie linii
                qreal normalizedProgress = (m_progress - 0.7) / 0.3; // 0.0 - 1.0 dla drugiej fazy

                // Linia zaczyna znikać - staje się węższa i jaśniejsza
                int lineWidth = qMax(1, static_cast<int>(w * (1.0 - normalizedProgress)));
                int lineOffset = (w - lineWidth) / 2;

                // Efekt "jasności" - najjaśniejsza na końcu
                int brightness = 40 + qMin(215, static_cast<int>(normalizedProgress * 120));

                // Najpierw rysujemy poziomą linię (szczątkowy obraz)
                QRect sourceRect = pixmap.rect();
                QRect targetRect(lineOffset, centerY - 1, lineWidth, 2); // 2px wysokości

                // Sprawdzenie poprawności wymiarów lineImage
                if (lineWidth > 0 && targetRect.width() > 0 && targetRect.height() > 0) {
                    // Rysujemy linię z efektem jasności
                    QImage lineImage(lineWidth, 2, QImage::Format_ARGB32_Premultiplied);
                    lineImage.fill(Qt::transparent);

                    QPainter linePainter(&lineImage);
                    linePainter.setCompositionMode(QPainter::CompositionMode_Source);

                    // Sprawdzamy poprawność parametrów
                    if (lineOffset >= 0 && centerY >= 1 &&
                        lineOffset + lineWidth <= pixmap.width() &&
                        centerY + 1 <= pixmap.height()) {

                        // Pobierz piksele z oryginalnego obrazu
                        linePainter.drawPixmap(0, 0, pixmap, lineOffset, centerY - 1, lineWidth, 2);

                        // Zwiększ jasność
                        for (int x = 0; x < lineWidth; x++) {
                            for (int y = 0; y < 2; y++) {
                                QColor pixelColor = QColor(lineImage.pixel(x, y));
                                int r = qMin(255, pixelColor.red() + brightness);
                                int g = qMin(255, pixelColor.green() + brightness);
                                int b = qMin(255, pixelColor.blue() + brightness);
                                lineImage.setPixel(x, y, qRgba(r, g, b, 255));
                            }
                        }

                        // Narysuj przetworzoną linię
                        resultPainter.drawImage(targetRect, lineImage);

                        // Dodaj poświatę
                        QColor glowColor(100, 200, 255, qMax(0, 100 - static_cast<int>(normalizedProgress * 100)));
                        for (int i = 1; i <= 5; i++) {
                            QRect glowRect = targetRect.adjusted(-i, -i, i, i);
                            resultPainter.setPen(QPen(glowColor, 1));
                            resultPainter.drawRect(glowRect);
                            glowColor.setAlpha(glowColor.alpha() / 2);
                        }

                        // Kończymy linePainter przed dalszym przetwarzaniem
                        linePainter.end();

                        // Losowe "bliknięcia" linii pod koniec animacji
                        if (normalizedProgress > 0.7) {
                            if (QRandomGenerator::global()->generateDouble() < 0.2) {
                                // Nie rysuj linii w ogóle - efekt migotania
                            } else {
                                // Losowe przesunięcia poziome
                                if (QRandomGenerator::global()->generateDouble() < 0.3) {
                                    int shift = QRandomGenerator::global()->bounded(-15, 16);
                                    targetRect.moveLeft(qMax(0, targetRect.left() + shift));
                                    if (targetRect.right() < w) {
                                        resultPainter.drawImage(targetRect, lineImage);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Zawsze kończymy resultPainter przed wyjściem z funkcji
            resultPainter.end();

            // Zapisujemy wynik do cache
            cachedResult = new QPixmap(result);
            m_resultCache.insert(cacheKey, cachedResult);
            m_lastProgress = m_progress;
        }
        catch (...) {
            // Upewniamy się, że painter zostanie zamknięty nawet w przypadku wyjątku
            resultPainter.end();
            throw; // Przekazujemy wyjątek dalej
        }
    }

    // Rysujemy finalny efekt z cache, tylko jeśli jest prawidłowy
    if (cachedResult) {
        painter->save();
        painter->translate(offset);
        painter->drawPixmap(0, 0, *cachedResult);
        painter->restore();
    }
}
