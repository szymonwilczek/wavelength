#include "cyber_checkbox.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

CyberCheckBox::CyberCheckBox(const QString &text, QWidget *parent): QCheckBox(text, parent), glow_intensity_(0.5) {
    setStyleSheet(
        "QCheckBox { "
        "spacing: 8px; "
        "background-color: transparent; "
        "color: #00ccff; "
        "font-family: Consolas; "
        "font-size: 9pt; "
        "margin-top: 4px; "
        "margin-bottom: 4px; "
        "}");

    setMinimumHeight(24);
}

QSize CyberCheckBox::sizeHint() const {
    QSize size = QCheckBox::sizeHint();
    size.setHeight(qMax(size.height(), 24));
    return size;
}

void CyberCheckBox::SetGlowIntensity(const double intensity) {
    glow_intensity_ = intensity;
    update();
}

void CyberCheckBox::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QStyleOptionButton option_button;
    option_button.initFrom(this);

    QColor background_color(0, 30, 40);
    QColor border_color(0, 200, 255);
    QColor check_color(0, 220, 255);
    QColor text_color(0, 200, 255);

    constexpr int checkbox_size = 16;
    constexpr int x = 0;
    const int y = (height() - checkbox_size) / 2;

    QPainterPath path;
    int clip_size = 3;

    path.moveTo(x + clip_size, y);
    path.lineTo(x + checkbox_size - clip_size, y);
    path.lineTo(x + checkbox_size, y + clip_size);
    path.lineTo(x + checkbox_size, y + checkbox_size - clip_size);
    path.lineTo(x + checkbox_size - clip_size, y + checkbox_size);
    path.lineTo(x + clip_size, y + checkbox_size);
    path.lineTo(x, y + checkbox_size - clip_size);
    path.lineTo(x, y + clip_size);
    path.closeSubpath();

    // checkbox background
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawPath(path);

    // checkbox border
    painter.setPen(QPen(border_color, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    if (glow_intensity_ > 0.1) {
        QColor glow_color = border_color;
        glow_color.setAlpha(80 * glow_intensity_);

        painter.setPen(QPen(glow_color, 2.0));
        painter.drawPath(path);
    }

    if (isChecked()) {
        painter.setPen(QPen(check_color, 2.0));
        int margin = 3;
        painter.drawLine(x + margin, y + margin, x + checkbox_size - margin, y + checkbox_size - margin);
        painter.drawLine(x + checkbox_size - margin, y + margin, x + margin, y + checkbox_size - margin);
    }

    QFont font = painter.font();
    font.setFamily("Blender Pro Book");
    font.setPointSize(9);
    painter.setFont(font);

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
