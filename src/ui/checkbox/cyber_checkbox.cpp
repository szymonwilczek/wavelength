#include "cyber_checkbox.h"

#include <QPropertyAnimation>

CyberCheckBox::CyberCheckBox(const QString &text, QWidget *parent): QCheckBox(text, parent), glow_intensity_(0.5) {
    // Dodaj marginesy, aby zapewnić poprawny rozmiar
    setStyleSheet("QCheckBox { spacing: 8px; background-color: transparent; color: #00ccff; font-family: Consolas; font-size: 9pt; margin-top: 4px; margin-bottom: 4px; }");

    // Ustaw minimalną wysokość dla checkboxa
    setMinimumHeight(24);
}

QSize CyberCheckBox::sizeHint() const {
    QSize size = QCheckBox::sizeHint();
    size.setHeight(qMax(size.height(), 24)); // Minimum 24px wysokości
    return size;
}

void CyberCheckBox::SetGlowIntensity(const double intensity) {
    glow_intensity_ = intensity;
    update();
}

void CyberCheckBox::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Nie rysujemy standardowego wyglądu
    QStyleOptionButton option_button;
    option_button.initFrom(this);

    // Kolory
    QColor background_color(0, 30, 40);
    QColor border_color(0, 200, 255);
    QColor check_color(0, 220, 255);
    QColor text_color(0, 200, 255);

    // Rysowanie pola wyboru (kwadrat ze ściętymi rogami)
    constexpr int checkbox_size = 16;
    constexpr int x = 0;
    const int y = (height() - checkbox_size) / 2;

    // Ścieżka dla pola wyboru ze ściętymi rogami
    QPainterPath path;
    int clip_size = 3; // rozmiar ścięcia

    path.moveTo(x + clip_size, y);
    path.lineTo(x + checkbox_size - clip_size, y);
    path.lineTo(x + checkbox_size, y + clip_size);
    path.lineTo(x + checkbox_size, y + checkbox_size - clip_size);
    path.lineTo(x + checkbox_size - clip_size, y + checkbox_size);
    path.lineTo(x + clip_size, y + checkbox_size);
    path.lineTo(x, y + checkbox_size - clip_size);
    path.lineTo(x, y + clip_size);
    path.closeSubpath();

    // Tło pola
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(border_color, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Efekt poświaty
    if (glow_intensity_ > 0.1) {
        QColor glow_color = border_color;
        glow_color.setAlpha(80 * glow_intensity_);

        painter.setPen(QPen(glow_color, 2.0));
        painter.drawPath(path);
    }

    // Rysowanie znacznika (jeśli zaznaczony)
    if (isChecked()) {
        // Znacznik w postaci "X" w stylu technologicznym
        painter.setPen(QPen(check_color, 2.0));
        int margin = 3;
        painter.drawLine(x + margin, y + margin, x + checkbox_size - margin, y + checkbox_size - margin);
        painter.drawLine(x + checkbox_size - margin, y + margin, x + margin, y + checkbox_size - margin);
    }

    // Skalowanie tekstu
    QFont font = painter.font();
    font.setFamily("Blender Pro Book");
    font.setPointSize(9);
    painter.setFont(font);

    // Rysowanie tekstu
    QRect text_rect(x + checkbox_size + 5, 0, width() - checkbox_size - 5, height());
    painter.setPen(text_color);
    painter.drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, text());
}

void CyberCheckBox::enterEvent(QEvent *event) {
    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(200);
    animation->setStartValue(glow_intensity_);
    animation->setEndValue(0.9);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    QCheckBox::enterEvent(event);
}

void CyberCheckBox::leaveEvent(QEvent *event) {
    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(200);
    animation->setStartValue(glow_intensity_);
    animation->setEndValue(0.5);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    QCheckBox::leaveEvent(event);
}
