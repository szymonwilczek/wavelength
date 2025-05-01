#include "cyber_chat_button.h"

CyberChatButton::CyberChatButton(const QString &text, QWidget *parent): QPushButton(text, parent) {
    setCursor(Qt::PointingHandCursor);
}

void CyberChatButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paleta kolorów
    QColor background_color(0, 40, 60); // ciemne tło
    QColor border_color(0, 170, 255); // neonowy niebieski
    QColor text_color(0, 220, 255); // jasny neonowy tekst

    // Ścieżka przycisku ze ściętymi rogami
    QPainterPath path;
    int clip_size = 5;

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
    if (isDown()) {
        background_color = QColor(0, 30, 45);
    } else if (underMouse()) {
        background_color = QColor(0, 50, 75);
        border_color = QColor(0, 200, 255);
    }

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
