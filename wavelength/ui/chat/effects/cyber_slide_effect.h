#ifndef CYBER_SLIDE_EFFECT_H
#define CYBER_SLIDE_EFFECT_H

#include <QGraphicsEffect>
#include <QPainter>
#include <QDateTime>
#include <QRandomGenerator>
#include <QCache>

class CyberSlideEffect : public QGraphicsEffect {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    Q_PROPERTY(SlideDirection direction READ direction WRITE setDirection)

public:
    enum SlideDirection {
        SlideLeft,
        SlideRight
    };

    CyberSlideEffect(QObject* parent = nullptr)
        : QGraphicsEffect(parent), m_progress(0.0), m_direction(SlideLeft), 
          m_resultCache(*"slide_cache"), m_lastProgress(-1.0)
    {
        // Limitujemy cache do 5 klatek - więcej nie potrzeba
        m_resultCache.setMaxCost(5);
    }
    
    qreal progress() const { return m_progress; }
    void setProgress(qreal progress) {
        m_progress = qBound(0.0, progress, 1.0);
        update(); // Wymusza odświeżenie efektu
    }
    
    SlideDirection direction() const { return m_direction; }
    void setDirection(SlideDirection dir) { m_direction = dir; }

protected:
    void draw(QPainter* painter) override {
        if (m_progress <= 0.001) {
            drawSource(painter);
            return;
        }
        
        if (m_progress >= 0.999) {
            return; // Nie rysuj nic gdy element zniknął
        }
        
        QPoint offset;
        QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::NoPad);
        if (pixmap.isNull())
            return;
            
        // Używamy cache tylko gdy różnica postępu jest znacząca
        QString cacheKey = QString("%1_%2").arg(m_direction).arg(m_progress, 0, 'f', 2);
        QPixmap* cachedResult = m_resultCache.object(cacheKey);
        
        bool needsRedraw = !cachedResult || qAbs(m_lastProgress - m_progress) > 0.05;
        
        if (needsRedraw) {
            QPixmap result(pixmap.size());
            result.fill(Qt::transparent);
            QPainter resultPainter(&result);
            
            // Kierunek przesunięcia
            int slideDistance = (m_direction == SlideLeft) ? 
                                -pixmap.width() : pixmap.width();
            
            // Obliczamy przesunięcie na podstawie postępu
            int offsetX = slideDistance * m_progress;
            
            // Rysujemy oryginalny obraz z przesunięciem
            resultPainter.drawPixmap(offsetX, 0, pixmap);
            
            // Dodajemy cyfrowe efekty
            if (m_progress > 0.05 && m_progress < 0.95) {
                addCyberEffects(resultPainter, result.rect(), m_progress);
            }
            
            // Zakończ rysowanie
            resultPainter.end();
            
            // Dodaj do cache
            cachedResult = new QPixmap(result);
            m_resultCache.insert(cacheKey, cachedResult, 1);
            m_lastProgress = m_progress;
        }
        
        // Rysujemy z cache
        if (cachedResult) {
            painter->drawPixmap(offset, *cachedResult);
        }
    }

private:
    // Dodaje cyberpunkowe efekty do animacji
    // Zamień tę implementację w klasie CyberSlideEffect
void addCyberEffects(QPainter& painter, const QRect& rect, qreal progress) {
    // Intensywność efektów zależy od postępu
    // Maksymalna w środku animacji
    qreal intensity = 1.0 - qAbs(progress - 0.5) * 2.0;
    intensity = qMin(intensity * 2.0, 1.0);

    // Skanowane linie - efekt przesuwających się pasków
    int scanCount = 3 + qRound(intensity * 5);
    for (int i = 0; i < scanCount; i++) {
        // Pozycja linii zależna od czasu i indeksu
        qreal phase = (QDateTime::currentMSecsSinceEpoch() % 1000) / 1000.0;
        qreal offset = (phase + static_cast<qreal>(i) / scanCount);
        offset -= floor(offset);

        int y = rect.top() + offset * rect.height();

        // Rysujemy linię skanowania
        QColor scanColor(0, 200, 255, 120 * intensity);
        painter.setPen(QPen(scanColor, 2));
        painter.drawLine(rect.left(), y, rect.right(), y);
    }

    // Glitche poziome - prostokąty z przesunięciem kolorów RGB
    int glitchCount = qRound(intensity * 8);
    for (int i = 0; i < glitchCount; i++) {
        int glitchHeight = 2 + QRandomGenerator::global()->bounded(8);
        int glitchY = QRandomGenerator::global()->bounded(rect.height() - glitchHeight);
        int glitchX = QRandomGenerator::global()->bounded(rect.width());
        int glitchWidth = 30 + QRandomGenerator::global()->bounded(100);

        if (glitchX + glitchWidth > rect.width())
            glitchWidth = rect.width() - glitchX;

        // Zamiast kopiowania pikseli, które wymaga toImage(),
        // rysujemy kolorowe prostokąty z przesunięciem, co tworzy efekt zakłóceń RGB
        painter.setCompositionMode(QPainter::CompositionMode_Screen);

        // Czerwony kanał
        QColor redShift(255, 0, 0, 60);
        painter.fillRect(QRect(glitchX, glitchY + rect.top(), glitchWidth, glitchHeight), redShift);

        // Zielony kanał (lekko przesunięty)
        QColor greenShift(0, 255, 0, 60);
        int greenOffset = QRandomGenerator::global()->bounded(-10, 10);
        painter.fillRect(QRect(glitchX + greenOffset, glitchY + rect.top(), glitchWidth, glitchHeight), greenShift);

        // Niebieski kanał (bardziej przesunięty)
        QColor blueShift(0, 0, 255, 60);
        int blueOffset = QRandomGenerator::global()->bounded(-15, 15);
        painter.fillRect(QRect(glitchX + blueOffset, glitchY + rect.top(), glitchWidth, glitchHeight), blueShift);
    }

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Pionowe linie - cyfrowy "szum"
    int lineCount = qRound(intensity * 15);
    for (int i = 0; i < lineCount; i++) {
        int x = rect.left() + QRandomGenerator::global()->bounded(rect.width());
        int height = 5 + QRandomGenerator::global()->bounded(30);
        int y = rect.top() + QRandomGenerator::global()->bounded(rect.height() - height);

        // Kolor linii - cyberpunkowa paleta
        QColor lineColor;
        int colorType = QRandomGenerator::global()->bounded(3);
        switch (colorType) {
            case 0: lineColor = QColor(0, 200, 255, 100); break;  // niebieski
            case 1: lineColor = QColor(255, 50, 200, 100); break; // różowy
            case 2: lineColor = QColor(0, 255, 200, 100); break;  // cyjanowy
        }

        painter.setPen(QPen(lineColor, 1));
        painter.drawLine(x, y, x, y + height);
    }

    // Efekt "pikselizacji" - losowe kwadraty
    int pixelateCount = qRound(intensity * 10);
    for (int i = 0; i < pixelateCount; i++) {
        int size = 4 + QRandomGenerator::global()->bounded(8);
        int x = rect.left() + QRandomGenerator::global()->bounded(rect.width() - size);
        int y = rect.top() + QRandomGenerator::global()->bounded(rect.height() - size);

        QColor pixelColor(
            100 + QRandomGenerator::global()->bounded(100),
            150 + QRandomGenerator::global()->bounded(100),
            200 + QRandomGenerator::global()->bounded(55),
            40 + QRandomGenerator::global()->bounded(30)
        );

        painter.setPen(Qt::NoPen);
        painter.setBrush(pixelColor);
        painter.drawRect(x, y, size, size);
    }
}

    qreal m_progress;
    SlideDirection m_direction;
    QCache<QString, QPixmap> m_resultCache;
    qreal m_lastProgress;
};

#endif // CYBER_SLIDE_EFFECT_H