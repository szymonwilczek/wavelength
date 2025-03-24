#ifndef DISINTEGRATION_EFFECT_H
#define DISINTEGRATION_EFFECT_H
#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QCache>

class DisintegrationEffect : public QGraphicsEffect {
public:
    DisintegrationEffect(QObject* parent = nullptr)
        : QGraphicsEffect(parent), m_progress(0.0),
          m_particleCache(100) // Cache na 100 elementów
    {
        // Pregeneruj losowe punkty do użycia w animacji
        m_precomputedOffsets.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            m_precomputedOffsets.append(QPoint(
                QRandomGenerator::global()->bounded(-100, 100),
                QRandomGenerator::global()->bounded(-100, 100)
            ));
        }
    }

    void setProgress(qreal progress) {
        m_progress = qBound(0.0, progress, 1.0);
        update();
    }

    qreal progress() const { return m_progress; }

protected:
    void draw(QPainter* painter) override {
        if (m_progress < 0.01) {
            drawSource(painter);
            return;
        }

        QPoint offset;
        QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
        if (pixmap.isNull())
            return;

        // Sprawdź, czy mamy już ten efekt w cache
        QString cacheKey = QString("%1").arg(m_progress, 0, 'f', 2);
        QPixmap* cachedResult = m_particleCache.object(cacheKey);

        if (!cachedResult) {
            // Generuj nowy efekt tylko jeśli go nie ma w cache
            QPixmap result(pixmap.size());
            result.fill(Qt::transparent);

            QPainter resultPainter(&result);
            resultPainter.setRenderHint(QPainter::Antialiasing);
            resultPainter.setRenderHint(QPainter::SmoothPixmapTransform);

            int w = pixmap.width();
            int h = pixmap.height();

            // Zmniejszamy liczbę cząstek dla lepszej wydajności
            int particleSize = 4 + m_progress * 8.0; // Większe cząstki
            int particlesX = w / particleSize;
            int particlesY = h / particleSize;

            // Maksymalna liczba cząstek
            const int maxParticles = 800;
            int totalParticles = particlesX * particlesY;
            double particleProbability = qMin(1.0, double(maxParticles) / totalParticles);

            // Współczynnik rozproszenia
            int spread = qRound(m_progress * 80.0);

            // Używamy pregenerowanych punktów przesunięcia zamiast losowania w czasie rzeczywistym
            int offsetIndex = 0;

            for (int y = 0; y < h; y += particleSize) {
                for (int x = 0; x < w; x += particleSize) {
                    // Dodatkowy warunek prawdopodobieństwa, aby zmniejszyć liczbę cząstek
                    if (QRandomGenerator::global()->generateDouble() < particleProbability * (1.0 - m_progress * 0.5)) {
                        // Użyj pregenerowanego offsetu
                        QPoint pregeneratedOffset = m_precomputedOffsets[offsetIndex % m_precomputedOffsets.size()];
                        offsetIndex++;

                        // Skaluj offset zgodnie z postępem
                        int dx = pregeneratedOffset.x() * m_progress * spread / 100;
                        int dy = pregeneratedOffset.y() * m_progress * spread / 100;

                        // Zmniejszamy nieprzezroczystość w miarę postępu
                        int alpha = qRound(255 * (1.0 - m_progress));

                        // Rysujemy tylko wierzchy cząsteczek
                        if (x % (particleSize * 2) == 0 && y % (particleSize * 2) == 0) {
                            // Wytnij kawałek oryginalnego obrazu
                            QRect sourceRect(x, y, particleSize, particleSize);
                            if (sourceRect.right() >= w) sourceRect.setRight(w-1);
                            if (sourceRect.bottom() >= h) sourceRect.setBottom(h-1);

                            // Narysuj go z przesunięciem
                            QRectF targetRect(x + dx, y + dy, particleSize, particleSize);
                            resultPainter.setOpacity(alpha / 255.0);
                            resultPainter.drawPixmap(targetRect, pixmap, sourceRect);
                        }
                    }
                }
            }

            resultPainter.end();

            // Zapisz wynik w cache
            cachedResult = new QPixmap(result);
            m_particleCache.insert(cacheKey, cachedResult);
        }

        // Rysuj z cache
        painter->save();
        painter->translate(offset);
        painter->drawPixmap(0, 0, *cachedResult);
        painter->restore();
    }

private:
    qreal m_progress;
    QCache<QString, QPixmap> m_particleCache;
    QVector<QPoint> m_precomputedOffsets;
};

#endif //DISINTEGRATION_EFFECT_H