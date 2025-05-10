#include "mask_overlay_effect.h"

#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>

MaskOverlayEffect::MaskOverlayEffect(QWidget *parent): QWidget(parent), reveal_percentage_(0), scanline_y_(0) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);

    scan_timer_ = new QTimer(this);
    connect(scan_timer_, &QTimer::timeout, this, &MaskOverlayEffect::UpdateScanLine);
    scan_timer_->start(30);
}

void MaskOverlayEffect::SetRevealProgress(const int percentage) {
    reveal_percentage_ = qBound(0, percentage, 100);
    update();
}

void MaskOverlayEffect::StartScanning() {
    reveal_percentage_ = 0;
    scanline_y_ = 0;
    if (!scan_timer_->isActive()) {
        scan_timer_->start();
    }
    setVisible(true);
    update();
}

void MaskOverlayEffect::StopScanning() {
    scan_timer_->stop();
    setVisible(false);
}

void MaskOverlayEffect::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    int reveal_height = static_cast<int>(h * (reveal_percentage_ / 100.0));

    QRectF mask_rect(0, reveal_height, w, h - reveal_height);

    if (mask_rect.height() > 0) {
        auto mask_background_color = QColor(5, 15, 25, 230);
        painter.fillRect(mask_rect, mask_background_color);

        painter.setPen(QColor(0, 50, 70, 150));
        for (int i = 0; i < 50; ++i) {
            int start_x = QRandomGenerator::global()->bounded(w);
            int start_y = reveal_height + QRandomGenerator::global()->bounded(static_cast<int>(mask_rect.height()));
            int len = QRandomGenerator::global()->bounded(5, 20);
            painter.drawLine(start_x, start_y, start_x + len, start_y);
        }
        painter.setPen(QColor(0, 100, 120, 80));
        for (int i = 0; i < 30; ++i) {
            int start_x = QRandomGenerator::global()->bounded(w);
            int start_y = reveal_height + QRandomGenerator::global()->bounded(static_cast<int>(mask_rect.height()));
            int len = QRandomGenerator::global()->bounded(1, 5);
            painter.drawLine(start_x, start_y, start_x, start_y + len);
        }
    }

    auto scanline_color = QColor(0, 255, 255, 180);
    painter.setPen(QPen(scanline_color, 1.5));
    painter.drawLine(0, scanline_y_, w, scanline_y_);

    QLinearGradient glow(0, scanline_y_ - 3, 0, scanline_y_ + 3);
    glow.setColorAt(0.0, QColor(0, 255, 255, 0));
    glow.setColorAt(0.5, QColor(0, 255, 255, 100));
    glow.setColorAt(1.0, QColor(0, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawRect(0, scanline_y_ - 3, w, 6);

    if (reveal_height > 0 && scanline_y_ < reveal_height + 5 && scanline_y_ > reveal_height - 5) {
        QLinearGradient reveal_effect(0, reveal_height - 5, 0, reveal_height);
        reveal_effect.setColorAt(0.0, QColor(200, 255, 255, 0));
        reveal_effect.setColorAt(1.0, QColor(200, 255, 255, 40));
        painter.setBrush(reveal_effect);
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, reveal_height - 5, w, 5);
    }
}

void MaskOverlayEffect::UpdateScanLine() {
    scanline_y_ += 4;
    if (scanline_y_ > height()) {
        scanline_y_ = 0;
    }
    update();
}
