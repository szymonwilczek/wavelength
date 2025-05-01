#include "mask_overlay.h"

MaskOverlay::MaskOverlay(QWidget *parent): QWidget(parent), reveal_percentage_(0), scanline_y_(0) {
    // Kluczowe: Przezroczystość dla zdarzeń myszy, aby kliki przechodziły do widgetu pod spodem
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // Potrzebne, aby paintEvent był wywoływany poprawnie przy przezroczystości
    setAttribute(Qt::WA_NoSystemBackground);

    // Timer do animacji linii skanującej
    scan_timer_ = new QTimer(this);
    connect(scan_timer_, &QTimer::timeout, this, &MaskOverlay::UpdateScanLine);
    scan_timer_->start(30); // Szybkość aktualizacji linii skanującej
}

void MaskOverlay::SetRevealProgress(const int percentage) {
    reveal_percentage_ = qBound(0, percentage, 100);
    update(); // Przerenderuj widget z nowym postępem
}

void MaskOverlay::StartScanning() {
    reveal_percentage_ = 0;
    scanline_y_ = 0;
    if (!scan_timer_->isActive()) {
        scan_timer_->start();
    }
    setVisible(true);
    update();
}

void MaskOverlay::StopScanning() {
    scan_timer_->stop();
    setVisible(false);
}

void MaskOverlay::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    // Oblicz wysokość odkrytego obszaru
    int reveal_height = static_cast<int>(h * (reveal_percentage_ / 100.0));

    // --- Rysowanie Maski ---
    // Obszar, który nadal jest zamaskowany
    QRectF mask_rect(0, reveal_height, w, h - reveal_height);

    if (mask_rect.height() > 0) {
        // Wypełnienie tła maski (półprzezroczyste)
        auto mask_background_color = QColor(5, 15, 25, 230); // Ciemny, prawie nieprzezroczysty
        painter.fillRect(mask_rect, mask_background_color);

        // Dodanie wzoru/tekstury do maski (przykład: losowe linie/glitche)
        painter.setPen(QColor(0, 50, 70, 150)); // Ciemnoniebieski glitch
        for (int i = 0; i < 50; ++i) { // Zmniejszona liczba dla wydajności
            int start_x = QRandomGenerator::global()->bounded(w);
            int start_y = reveal_height + QRandomGenerator::global()->bounded(static_cast<int>(mask_rect.height()));
            int len = QRandomGenerator::global()->bounded(5, 20);
            painter.drawLine(start_x, start_y, start_x + len, start_y);
        }
        painter.setPen(QColor(0, 100, 120, 80)); // Jaśniejszy glitch
        for (int i = 0; i < 30; ++i) {
            int start_x = QRandomGenerator::global()->bounded(w);
            int start_y = reveal_height + QRandomGenerator::global()->bounded(static_cast<int>(mask_rect.height()));
            int len = QRandomGenerator::global()->bounded(1, 5);
            painter.drawLine(start_x, start_y, start_x, start_y + len);
        }
    }

    // --- Rysowanie Linii Skanującej ---
    // Linia jest rysowana na całej wysokości, ale może mieć inny wygląd w zależności od postępu
    auto scanline_color = QColor(0, 255, 255, 180); // Jasny cyjan
    painter.setPen(QPen(scanline_color, 1.5));
    painter.drawLine(0, scanline_y_, w, scanline_y_);

    // Dodatkowy efekt "blasku" dla linii skanującej
    QLinearGradient glow(0, scanline_y_ - 3, 0, scanline_y_ + 3);
    glow.setColorAt(0.0, QColor(0, 255, 255, 0));
    glow.setColorAt(0.5, QColor(0, 255, 255, 100));
    glow.setColorAt(1.0, QColor(0, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawRect(0, scanline_y_ - 3, w, 6);

    // --- Rysowanie efektu "odsłaniania" przez linię ---
    // Można dodać efekt np. lekkiego rozjaśnienia tuż nad linią skanującą w odkrytym obszarze
    if (reveal_height > 0 && scanline_y_ < reveal_height + 5 && scanline_y_ > reveal_height - 5) {
        QLinearGradient reveal_effect(0, reveal_height - 5, 0, reveal_height);
        reveal_effect.setColorAt(0.0, QColor(200, 255, 255, 0));
        reveal_effect.setColorAt(1.0, QColor(200, 255, 255, 40)); // Lekki biało-cyjanowy blask
        painter.setBrush(reveal_effect);
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, reveal_height - 5, w, 5);
    }
}

void MaskOverlay::UpdateScanLine() {
    // Przesuwaj linię skanującą w dół
    scanline_y_ += 4; // Prędkość skanowania
    if (scanline_y_ > height()) {
        scanline_y_ = 0; // Wróć na górę
    }
    // Aktualizuj tylko obszar wokół linii skanującej dla optymalizacji
    // update(0, m_scanLineY - 5, width(), 10); // Może powodować artefakty, lepiej update() całości
    update();
}
