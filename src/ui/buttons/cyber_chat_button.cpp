#include "cyber_chat_button.h"

CyberChatButton::CyberChatButton(const QString &text, QWidget *parent): QPushButton(text, parent) {
    setCursor(Qt::PointingHandCursor);
}

void CyberChatButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paleta kolorów
    QColor bgColor(0, 40, 60); // ciemne tło
    QColor borderColor(0, 170, 255); // neonowy niebieski
    QColor textColor(0, 220, 255); // jasny neonowy tekst

    // Ścieżka przycisku ze ściętymi rogami
    QPainterPath path;
    int clipSize = 5;

    path.moveTo(clipSize, 0);
    path.lineTo(width() - clipSize, 0);
    path.lineTo(width(), clipSize);
    path.lineTo(width(), height() - clipSize);
    path.lineTo(width() - clipSize, height());
    path.lineTo(clipSize, height());
    path.lineTo(0, height() - clipSize);
    path.lineTo(0, clipSize);
    path.closeSubpath();

    // Tło przycisku
    if (isDown()) {
        bgColor = QColor(0, 30, 45);
    } else if (underMouse()) {
        bgColor = QColor(0, 50, 75);
        borderColor = QColor(0, 200, 255);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(borderColor, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Tekst przycisku
    painter.setPen(QPen(textColor, 1));
    painter.setFont(QFont("Consolas", 10, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
