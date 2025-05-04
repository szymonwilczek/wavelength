#include "cyber_push_button.h"

#include <cmath>

CyberPushButton::CyberPushButton(const QString &text, QWidget *parent): QPushButton(text, parent), glow_intensity_(0.5) {
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("background-color: transparent; border: none;");

    base_glow_intensity_ = 0.5;
}

void CyberPushButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paleta kolorów
    QColor background_color(0, 40, 60);        // ciemny tył
    QColor border_color(0, 200, 255);  // neonowy niebieski brzeg
    QColor text_color(0, 220, 255);    // neonowy tekst

    // Tworzymy ścieżkę przycisku - ścięte rogi dla cyberpunkowego stylu
    QPainterPath path;
    int clip_size = 5; // rozmiar ścięcia

    path.moveTo(clip_size, 0);
    path.lineTo(width() - clip_size, 0);
    path.lineTo(width(), clip_size);
    path.lineTo(width(), height() - clip_size);
    path.lineTo(width() - clip_size, height());
    path.lineTo(clip_size, height());
    path.lineTo(0, height() - clip_size);
    path.lineTo(0, clip_size);
    path.closeSubpath();

    // Tło przycisku
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(border_color, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Tekst przycisku
    painter.setPen(QPen(text_color, 1));
    painter.setFont(QFont("Consolas", 10, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
