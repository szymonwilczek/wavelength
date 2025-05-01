#include "electronic_shutdown_effect.h"

#include <qmath.h>

ElectronicShutdownEffect::ElectronicShutdownEffect(QObject *parent): QGraphicsEffect(parent), progress_(0.0), last_progress_(-1.0),
                                                                     result_cache_(5),
                                                                     random_seed_(QRandomGenerator::global()->generate()) {
}

void ElectronicShutdownEffect::SetProgress(const qreal progress) {
    progress_ = qBound(0.0, progress, 1.0);
    update();
}

void ElectronicShutdownEffect::draw(QPainter *painter) {
    if (progress_ < 0.01) {
        // Jeśli animacja się nie rozpoczęła, rysuj normalny obraz
        drawSource(painter);
        return;
    }

    if (progress_ > 0.99) {
        // Jeśli animacja się zakończyła, nie rysuj niczego
        return;
    }

    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::NoPad);
    if (pixmap.isNull())
        return;

    // Sprawdź czy mamy już ten efekt w cache
    QString cache_key = QString("%1").arg(progress_, 0, 'f', 2);
    QPixmap* cached_result = result_cache_.object(cache_key);

    if (cached_result == nullptr || qAbs(last_progress_ - progress_) > 0.05) {
        // Generowanie rezultatu tylko jeśli go nie ma w cache, lub progress
        // znacząco się zmienił (dla płynniejszej animacji)
        QPixmap result(pixmap.size());
        result.fill(Qt::transparent);

        // Używamy bloku try-catch aby upewnić się że painter zostanie zamknięty
        QPainter result_painter(&result);
        try {
            result_painter.setRenderHint(QPainter::Antialiasing);
            result_painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int w = pixmap.width();
            int h = pixmap.height();
            int center_y = h / 2;

            // Faza 1 (0.0-0.7): Zwężanie obrazu do poziomej linii
            // Faza 2 (0.7-1.0): Zanikanie poziomej linii

            if (progress_ < 0.7) {
                qreal normalized_progress = progress_ / 0.7; // 0.0 - 1.0 dla pierwszej fazy

                // Wysokość zwężającego się obrazu - nieliniowe zmniejszanie dla lepszego efektu
                int new_height = h * (1.0 - qPow(normalized_progress, 1.8));

                // Zabezpieczenie przed ujemnymi wartościami
                new_height = qMax(2, new_height);

                // Dodajemy losowe zakłócenia wysokości z bezpiecznym sprawdzeniem zakresu
                if (normalized_progress > 0.4) {
                    int glitch_amplitude = qMax(1, static_cast<int>(normalized_progress * 15));
                    int glitch_offset = QRandomGenerator::global()->bounded(-glitch_amplitude, glitch_amplitude+1);
                    new_height = qMax(2, new_height + glitch_offset);
                }

                // Obliczamy obszar źródłowy i docelowy
                QRect source_rect = pixmap.rect();
                QRect target_rect(0, center_y - new_height / 2, w, new_height);

                // Rysujemy zwężony obraz
                result_painter.drawPixmap(target_rect, pixmap, source_rect);

                // Dodajemy losowe przesunięcia poziomych linii (zakłócenia)
                if (normalized_progress > 0.3) {
                    // Zabezpieczone parametry losowe
                    int num_of_glitches = qMin(5, qMax(1, static_cast<int>(normalized_progress * 15)));

                    for (int i = 0; i < num_of_glitches; ++i) {
                        // Bezpieczne losowanie wartości
                        int min_y = target_rect.top();
                        int max_y = target_rect.bottom();

                        if (min_y >= max_y) {
                            // Zabezpieczenie przed nieprawidłowym zakresem
                            continue;
                        }

                        int glitch_y = QRandomGenerator::global()->bounded(min_y, max_y + 1);

                        int min_width = qMax(1, w / 4);
                        int max_width = qMax(min_width + 1, w);
                        int glitch_width = QRandomGenerator::global()->bounded(min_width, max_width);

                        int glitch_offset = QRandomGenerator::global()->bounded(-10, 11); // -10 do 10

                        int glitch_height = qMax(1, QRandomGenerator::global()->bounded(1, qMax(2, new_height / 20) + 1));

                        if (glitch_y + glitch_height <= max_y) {
                            QRect glitch_source(0, glitch_y, glitch_width, glitch_height);

                            // Przesuwamy fragment obrazu
                            result_painter.drawPixmap(
                                QPoint(glitch_offset, glitch_y),
                                result,
                                glitch_source
                            );
                        }
                    }
                }

                // Dodajemy linie CRT (scanlines)
                QPen scanline_pen(QColor(0, 0, 0, 40));
                result_painter.setPen(scanline_pen);
                for (int y = target_rect.top(); y < target_rect.bottom(); y += 2) {
                    result_painter.drawLine(0, y, w, y);
                }

                // Pulsujące bliknięcie (mrugnięcie)
                if (normalized_progress > 0.5) {
                    qreal flicker_chance = (normalized_progress - 0.5) * 2.0; // 0.0 - 1.0
                    if (QRandomGenerator::global()->generateDouble() < flicker_chance * 0.1) {
                        // Dodaj przezroczyste białe nakładki dla efektu migotania
                        result_painter.setCompositionMode(QPainter::CompositionMode_Plus);
                        result_painter.fillRect(target_rect, QColor(80, 80, 80, 30));
                    }
                }
            } else {
                // Druga faza - zanikanie linii
                qreal normalized_progress = (progress_ - 0.7) / 0.3; // 0.0 - 1.0 dla drugiej fazy

                // Linia zaczyna znikać - staje się węższa i jaśniejsza
                int line_width = qMax(1, static_cast<int>(w * (1.0 - normalized_progress)));
                int line_offset = (w - line_width) / 2;

                // Efekt "jasności" - najjaśniejsza na końcu
                int brightness = 40 + qMin(215, static_cast<int>(normalized_progress * 120));

                QRect target_rect(line_offset, center_y - 1, line_width, 2); // 2px wysokości

                // Sprawdzenie poprawności wymiarów lineImage
                if (line_width > 0 && target_rect.width() > 0 && target_rect.height() > 0) {
                    // Rysujemy linię z efektem jasności
                    QImage line_image(line_width, 2, QImage::Format_ARGB32_Premultiplied);
                    line_image.fill(Qt::transparent);

                    QPainter line_painter(&line_image);
                    line_painter.setCompositionMode(QPainter::CompositionMode_Source);

                    // Sprawdzamy poprawność parametrów
                    if (line_offset >= 0 && center_y >= 1 &&
                        line_offset + line_width <= pixmap.width() &&
                        center_y + 1 <= pixmap.height()) {

                        // Pobierz piksele z oryginalnego obrazu
                        line_painter.drawPixmap(0, 0, pixmap, line_offset, center_y - 1, line_width, 2);

                        // Zwiększ jasność
                        for (int x = 0; x < line_width; x++) {
                            for (int y = 0; y < 2; y++) {
                                auto pixelColor = QColor(line_image.pixel(x, y));
                                int r = qMin(255, pixelColor.red() + brightness);
                                int g = qMin(255, pixelColor.green() + brightness);
                                int b = qMin(255, pixelColor.blue() + brightness);
                                line_image.setPixel(x, y, qRgba(r, g, b, 255));
                            }
                        }

                        // Narysuj przetworzoną linię
                        result_painter.drawImage(target_rect, line_image);

                        // Dodaj poświatę
                        QColor glow_color(100, 200, 255, qMax(0, 100 - static_cast<int>(normalized_progress * 100)));
                        for (int i = 1; i <= 5; i++) {
                            QRect glowRect = target_rect.adjusted(-i, -i, i, i);
                            result_painter.setPen(QPen(glow_color, 1));
                            result_painter.drawRect(glowRect);
                            glow_color.setAlpha(glow_color.alpha() / 2);
                        }

                        // Kończymy linePainter przed dalszym przetwarzaniem
                        line_painter.end();

                        // Losowe "bliknięcia" linii pod koniec animacji
                        if (normalized_progress > 0.7) {
                            if (QRandomGenerator::global()->generateDouble() < 0.2) {
                                // Nie rysuj linii w ogóle - efekt migotania
                            } else {
                                // Losowe przesunięcia poziome
                                if (QRandomGenerator::global()->generateDouble() < 0.3) {
                                    int shift = QRandomGenerator::global()->bounded(-15, 16);
                                    target_rect.moveLeft(qMax(0, target_rect.left() + shift));
                                    if (target_rect.right() < w) {
                                        result_painter.drawImage(target_rect, line_image);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Zawsze kończymy resultPainter przed wyjściem z funkcji
            result_painter.end();

            // Zapisujemy wynik do cache
            cached_result = new QPixmap(result);
            result_cache_.insert(cache_key, cached_result);
            last_progress_ = progress_;
        }
        catch (...) {
            // Upewniamy się, że painter zostanie zamknięty nawet w przypadku wyjątku
            result_painter.end();
            throw; // Przekazujemy wyjątek dalej
        }
    }

        painter->save();
        painter->translate(offset);
        painter->drawPixmap(0, 0, *cached_result);
        painter->restore();

}
