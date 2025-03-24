#ifndef DISINTEGRATION_EFFECT_H
#define DISINTEGRATION_EFFECT_H
#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>


class DisintegrationEffect : public QGraphicsEffect {
public:
    DisintegrationEffect(QObject* parent = nullptr)
        : QGraphicsEffect(parent), m_progress(0.0) {}

    void setProgress(qreal progress) {
        m_progress = qBound(0.0, progress, 1.0);
        updateBoundingRect();
    }

    qreal progress() const { return m_progress; }

protected:
    void draw(QPainter* painter) override {
        if (m_progress < 0.01) {
            drawSource(painter);
            return;
        }

        QPoint offset;
        QPixmap pixmap;

        // Pobieramy oryginalny obraz
        pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
        if (pixmap.isNull())
            return;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);

        // Przesuwamy do prawidłowego offsetu
        painter->translate(offset);

        int w = pixmap.width();
        int h = pixmap.height();

        // Rysujemy efekt rozpadu
        QImage image = pixmap.toImage();

        // Współczynnik rozproszenia
        int particleSize = 2 + m_progress * 5.0;
        int spread = qRound(m_progress * 50.0);

        // Rysujemy "rozpadające się" cząsteczki
        for (int y = 0; y < h; y += particleSize) {
            for (int x = 0; x < w; x += particleSize) {
                // Wyższy próg oznacza mniej cząsteczek
                double threshold = 0.2 + m_progress * 0.8;

                // Decydujemy czy narysować tę cząsteczkę
                if (QRandomGenerator::global()->generateDouble() > threshold) {
                    // Pozycja docelowa (z przesunięciem)
                    int dx = QRandomGenerator::global()->bounded(-spread, spread + 1);
                    int dy = QRandomGenerator::global()->bounded(-spread, spread + 1);

                    // Pobieramy kolor z oryginalnego piksela
                    QColor color = image.pixelColor(x, y);

                    // Zmniejszamy nieprzezroczystość w miarę postępu
                    color.setAlpha(color.alpha() * (1.0 - m_progress));

                    // Rysujemy cząsteczkę
                    painter->fillRect(x + dx, y + dy, particleSize, particleSize, color);
                }
            }
        }

        painter->restore();
    }

private:
    qreal m_progress;
};



#endif //DISINTEGRATION_EFFECT_H
