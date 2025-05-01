#include "disintegration_effect.h"

DisintegrationEffect::DisintegrationEffect(QObject *parent): QGraphicsEffect(parent), progress_(0.0),
                                                             particle_cache_(100) {
    // Pregeneruj losowe punkty do użycia w animacji
    precomputed_offsets_.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        precomputed_offsets_.append(QPoint(
            QRandomGenerator::global()->bounded(-100, 100),
            QRandomGenerator::global()->bounded(-100, 100)
        ));
    }
}

void DisintegrationEffect::SetProgress(const qreal progress) {
    progress_ = qBound(0.0, progress, 1.0);
    update();
}

void DisintegrationEffect::draw(QPainter *painter) {
    if (progress_ < 0.01) {
        drawSource(painter);
        return;
    }

    QPoint offset;
    const QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
    if (pixmap.isNull())
        return;

    // Sprawdź, czy mamy już ten efekt w cache
    const QString cache_key = QString("%1").arg(progress_, 0, 'f', 2);
    QPixmap* cached_result = particle_cache_.object(cache_key);

    if (!cached_result) {
        // Generuj nowy efekt tylko jeśli go nie ma w cache
        QPixmap result(pixmap.size());
        result.fill(Qt::transparent);

        QPainter result_painter(&result);
        result_painter.setRenderHint(QPainter::Antialiasing);
        result_painter.setRenderHint(QPainter::SmoothPixmapTransform);

        const int w = pixmap.width();
        const int h = pixmap.height();

        // Zmniejszamy liczbę cząstek dla lepszej wydajności
        const int particle_size = 4 + progress_ * 8.0; // Większe cząstki
        const int particles_x = w / particle_size;
        const int particles_y = h / particle_size;

        // Maksymalna liczba cząstek
        constexpr int max_particles = 800;
        const int total_particles = particles_x * particles_y;
        const double particle_probability = qMin(1.0, static_cast<double>(max_particles) / total_particles);

        // Współczynnik rozproszenia
        const int spread = qRound(progress_ * 80.0);

        // Używamy pregenerowanych punktów przesunięcia zamiast losowania w czasie rzeczywistym
        int offset_index = 0;

        for (int y = 0; y < h; y += particle_size) {
            for (int x = 0; x < w; x += particle_size) {
                // Dodatkowy warunek prawdopodobieństwa, aby zmniejszyć liczbę cząstek
                if (QRandomGenerator::global()->generateDouble() < particle_probability * (1.0 - progress_ * 0.5)) {
                    // Użyj pregenerowanego offsetu
                    QPoint pregenerated_offset = precomputed_offsets_[offset_index % precomputed_offsets_.size()];
                    offset_index++;

                    // Skaluj offset zgodnie z postępem
                    const int dx = pregenerated_offset.x() * progress_ * spread / 100;
                    const int dy = pregenerated_offset.y() * progress_ * spread / 100;

                    // Zmniejszamy nieprzezroczystość w miarę postępu
                    const int alpha = qRound(255 * (1.0 - progress_));

                    // Rysujemy tylko wierzchy cząsteczek
                    if (x % (particle_size * 2) == 0 && y % (particle_size * 2) == 0) {
                        // Wytnij kawałek oryginalnego obrazu
                        QRect source_rect(x, y, particle_size, particle_size);
                        if (source_rect.right() >= w) source_rect.setRight(w-1);
                        if (source_rect.bottom() >= h) source_rect.setBottom(h-1);

                        // Narysuj go z przesunięciem
                        QRectF target_rect(x + dx, y + dy, particle_size, particle_size);
                        result_painter.setOpacity(alpha / 255.0);
                        result_painter.drawPixmap(target_rect, pixmap, source_rect);
                    }
                }
            }
        }

        result_painter.end();

        // Zapisz wynik w cache
        cached_result = new QPixmap(result);
        particle_cache_.insert(cache_key, cached_result);
    }

    // Rysuj z cache
    painter->save();
    painter->translate(offset);
    painter->drawPixmap(0, 0, *cached_result);
    painter->restore();
}
