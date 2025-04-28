#ifndef MASK_OVERLAY_H
#define MASK_OVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QLinearGradient>

class MaskOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MaskOverlay(QWidget* parent = nullptr)
        : QWidget(parent), m_revealPercentage(0), m_scanLineY(0)
    {
        // Kluczowe: Przezroczystość dla zdarzeń myszy, aby kliki przechodziły do widgetu pod spodem
        setAttribute(Qt::WA_TransparentForMouseEvents);
        // Potrzebne, aby paintEvent był wywoływany poprawnie przy przezroczystości
        setAttribute(Qt::WA_NoSystemBackground);

        // Timer do animacji linii skanującej
        m_scanTimer = new QTimer(this);
        connect(m_scanTimer, &QTimer::timeout, this, &MaskOverlay::updateScanLine);
        m_scanTimer->start(30); // Szybkość aktualizacji linii skanującej
    }

public slots:
    void setRevealProgress(int percentage) {
        m_revealPercentage = qBound(0, percentage, 100);
        update(); // Przerenderuj widget z nowym postępem
    }

    void startScanning() {
        m_revealPercentage = 0;
        m_scanLineY = 0;
        if (!m_scanTimer->isActive()) {
            m_scanTimer->start();
        }
        setVisible(true);
        update();
    }

    void stopScanning() {
        m_scanTimer->stop();
        setVisible(false);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int w = width();
        int h = height();
        if (w <= 0 || h <= 0) return;

        // Oblicz wysokość odkrytego obszaru
        int revealHeight = static_cast<int>(h * (m_revealPercentage / 100.0));

        // --- Rysowanie Maski ---
        // Obszar, który nadal jest zamaskowany
        QRectF maskRect(0, revealHeight, w, h - revealHeight);

        if (maskRect.height() > 0) {
            // Wypełnienie tła maski (półprzezroczyste)
            QColor maskBgColor = QColor(5, 15, 25, 230); // Ciemny, prawie nieprzezroczysty
            painter.fillRect(maskRect, maskBgColor);

            // Dodanie wzoru/tekstury do maski (przykład: losowe linie/glitche)
            painter.setPen(QColor(0, 50, 70, 150)); // Ciemnoniebieski glitch
            for (int i = 0; i < 50; ++i) { // Zmniejszona liczba dla wydajności
                int startX = QRandomGenerator::global()->bounded(w);
                int startY = revealHeight + QRandomGenerator::global()->bounded(static_cast<int>(maskRect.height()));
                int len = QRandomGenerator::global()->bounded(5, 20);
                painter.drawLine(startX, startY, startX + len, startY);
            }
             painter.setPen(QColor(0, 100, 120, 80)); // Jaśniejszy glitch
             for (int i = 0; i < 30; ++i) {
                 int startX = QRandomGenerator::global()->bounded(w);
                 int startY = revealHeight + QRandomGenerator::global()->bounded(static_cast<int>(maskRect.height()));
                 int len = QRandomGenerator::global()->bounded(1, 5);
                 painter.drawLine(startX, startY, startX, startY + len);
             }
        }

        // --- Rysowanie Linii Skanującej ---
        // Linia jest rysowana na całej wysokości, ale może mieć inny wygląd w zależności od postępu
        QColor scanLineColor = QColor(0, 255, 255, 180); // Jasny cyjan
        painter.setPen(QPen(scanLineColor, 1.5));
        painter.drawLine(0, m_scanLineY, w, m_scanLineY);

        // Dodatkowy efekt "blasku" dla linii skanującej
        QLinearGradient glow(0, m_scanLineY - 3, 0, m_scanLineY + 3);
        glow.setColorAt(0.0, QColor(0, 255, 255, 0));
        glow.setColorAt(0.5, QColor(0, 255, 255, 100));
        glow.setColorAt(1.0, QColor(0, 255, 255, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawRect(0, m_scanLineY - 3, w, 6);

        // --- Rysowanie efektu "odsłaniania" przez linię ---
        // Można dodać efekt np. lekkiego rozjaśnienia tuż nad linią skanującą w odkrytym obszarze
        if (revealHeight > 0 && m_scanLineY < revealHeight + 5 && m_scanLineY > revealHeight - 5) {
             QLinearGradient revealEffect(0, revealHeight - 5, 0, revealHeight);
             revealEffect.setColorAt(0.0, QColor(200, 255, 255, 0));
             revealEffect.setColorAt(1.0, QColor(200, 255, 255, 40)); // Lekki biało-cyjanowy blask
             painter.setBrush(revealEffect);
             painter.setPen(Qt::NoPen);
             painter.drawRect(0, revealHeight - 5, w, 5);
        }
    }

private slots:
    void updateScanLine() {
        // Przesuwaj linię skanującą w dół
        m_scanLineY += 4; // Prędkość skanowania
        if (m_scanLineY > height()) {
            m_scanLineY = 0; // Wróć na górę
        }
        // Aktualizuj tylko obszar wokół linii skanującej dla optymalizacji
        // update(0, m_scanLineY - 5, width(), 10); // Może powodować artefakty, lepiej update() całości
        update();
    }

private:
    int m_revealPercentage;
    int m_scanLineY;
    QTimer* m_scanTimer;
};

#endif // MASK_OVERLAY_H