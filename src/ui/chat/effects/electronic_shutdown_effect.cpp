#include "electronic_shutdown_effect.h"

#include <qmath.h>

ElectronicShutdownEffect::ElectronicShutdownEffect(QObject *parent): QGraphicsEffect(parent), progress_(0.0),
                                                                     last_progress_(-1.0),
                                                                     result_cache_(5) {
}

void ElectronicShutdownEffect::SetProgress(const qreal progress) {
    progress_ = qBound(0.0, progress, 1.0);
    update();
}

void ElectronicShutdownEffect::draw(QPainter *painter) {
    if (progress_ < 0.01) {
        drawSource(painter);
        return;
    }

    if (progress_ > 0.99) {
        return;
    }

    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, NoPad);
    if (pixmap.isNull())
        return;

    QString cache_key = QString("%1").arg(progress_, 0, 'f', 2);
    QPixmap *cached_result = result_cache_.object(cache_key);

    if (cached_result == nullptr || qAbs(last_progress_ - progress_) > 0.05) {
        QPixmap result(pixmap.size());
        result.fill(Qt::transparent);

        QPainter result_painter(&result);
        try {
            result_painter.setRenderHint(QPainter::Antialiasing);
            result_painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int w = pixmap.width();
            int h = pixmap.height();
            int center_y = h / 2;

            // phase 1 (0.0-0.7): Narrowing the image to a horizontal line
            if (progress_ < 0.7) {
                qreal normalized_progress = progress_ / 0.7; // 0.0-1.0 for first phase

                int new_height = h * (1.0 - qPow(normalized_progress, 1.8));
                new_height = qMax(2, new_height);

                if (normalized_progress > 0.4) {
                    int glitch_amplitude = qMax(1, static_cast<int>(normalized_progress * 15));
                    int glitch_offset = QRandomGenerator::global()->bounded(-glitch_amplitude, glitch_amplitude + 1);
                    new_height = qMax(2, new_height + glitch_offset);
                }

                QRect source_rect = pixmap.rect();
                QRect target_rect(0, center_y - new_height / 2, w, new_height);

                result_painter.drawPixmap(target_rect, pixmap, source_rect);

                if (normalized_progress > 0.3) {
                    int num_of_glitches = qMin(5, qMax(1, static_cast<int>(normalized_progress * 15)));

                    for (int i = 0; i < num_of_glitches; ++i) {
                        int min_y = target_rect.top();
                        int max_y = target_rect.bottom();

                        if (min_y >= max_y) continue;

                        int glitch_y = QRandomGenerator::global()->bounded(min_y, max_y + 1);

                        int min_width = qMax(1, w / 4);
                        int max_width = qMax(min_width + 1, w);
                        int glitch_width = QRandomGenerator::global()->bounded(min_width, max_width);

                        int glitch_offset = QRandomGenerator::global()->bounded(-10, 11);

                        int glitch_height = qMax(
                            1, QRandomGenerator::global()->bounded(1, qMax(2, new_height / 20) + 1));

                        if (glitch_y + glitch_height <= max_y) {
                            QRect glitch_source(0, glitch_y, glitch_width, glitch_height);

                            result_painter.drawPixmap(
                                QPoint(glitch_offset, glitch_y),
                                result,
                                glitch_source
                            );
                        }
                    }
                }

                // crt lines (scanlines)
                QPen scanline_pen(QColor(0, 0, 0, 40));
                result_painter.setPen(scanline_pen);
                for (int y = target_rect.top(); y < target_rect.bottom(); y += 2) {
                    result_painter.drawLine(0, y, w, y);
                }

                // pulse blink (twitch)
                if (normalized_progress > 0.5) {
                    qreal flicker_chance = (normalized_progress - 0.5) * 2.0; // 0.0 - 1.0
                    if (QRandomGenerator::global()->generateDouble() < flicker_chance * 0.1) {
                        result_painter.setCompositionMode(QPainter::CompositionMode_Plus);
                        result_painter.fillRect(target_rect, QColor(80, 80, 80, 30));
                    }
                }
            } else {
                // phase 2 (0.7-1.0): Fading of the horizontal line
                qreal normalized_progress = (progress_ - 0.7) / 0.3;

                int line_width = qMax(1, static_cast<int>(w * (1.0 - normalized_progress)));
                int line_offset = (w - line_width) / 2;

                int brightness = 40 + qMin(215, static_cast<int>(normalized_progress * 120));

                QRect target_rect(line_offset, center_y - 1, line_width, 2);

                if (line_width > 0 && target_rect.width() > 0 && target_rect.height() > 0) {
                    QImage line_image(line_width, 2, QImage::Format_ARGB32_Premultiplied);
                    line_image.fill(Qt::transparent);

                    QPainter line_painter(&line_image);
                    line_painter.setCompositionMode(QPainter::CompositionMode_Source);

                    if (line_offset >= 0 && center_y >= 1 &&
                        line_offset + line_width <= pixmap.width() &&
                        center_y + 1 <= pixmap.height()) {
                        line_painter.drawPixmap(0, 0, pixmap, line_offset, center_y - 1, line_width, 2);

                        for (int x = 0; x < line_width; x++) {
                            for (int y = 0; y < 2; y++) {
                                auto pixelColor = QColor(line_image.pixel(x, y));
                                int r = qMin(255, pixelColor.red() + brightness);
                                int g = qMin(255, pixelColor.green() + brightness);
                                int b = qMin(255, pixelColor.blue() + brightness);
                                line_image.setPixel(x, y, qRgba(r, g, b, 255));
                            }
                        }

                        result_painter.drawImage(target_rect, line_image);

                        QColor glow_color(100, 200, 255, qMax(0, 100 - static_cast<int>(normalized_progress * 100)));
                        for (int i = 1; i <= 5; i++) {
                            QRect glowRect = target_rect.adjusted(-i, -i, i, i);
                            result_painter.setPen(QPen(glow_color, 1));
                            result_painter.drawRect(glowRect);
                            glow_color.setAlpha(glow_color.alpha() / 2);
                        }

                        line_painter.end();

                        if (normalized_progress > 0.7) {
                            if (!QRandomGenerator::global()->generateDouble() < 0.2) {
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

            result_painter.end();

            cached_result = new QPixmap(result);
            result_cache_.insert(cache_key, cached_result);
            last_progress_ = progress_;
        } catch (...) {
            result_painter.end();
            throw;
        }
    }

    painter->save();
    painter->translate(offset);
    painter->drawPixmap(0, 0, *cached_result);
    painter->restore();
}
