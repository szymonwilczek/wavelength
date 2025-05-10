#include "tab_button.h"

#include <cmath>
#include <QPainter>
#include <QPropertyAnimation>

TabButton::TabButton(const QString &text, QWidget *parent): QPushButton(text, parent), underline_offset_(0),
                                                            is_active_(false) {
    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    setCheckable(true);

    setStyleSheet(
        "TabButton {"
        "  color: #00ccff;"
        "  background-color: transparent;"
        "  border: none;"
        "  font-family: 'Consolas';"
        "  font-size: 11pt;"
        "  padding: 5px 15px;"
        "  margin: 0px 10px;"
        "  text-align: center;"
        "}"
        "TabButton:hover {"
        "  color: #00eeff;"
        "}"
        "TabButton:checked {"
        "  color: #ffffff;"
        "}"
    );
}

void TabButton::SetUnderlineOffset(const double offset) {
    underline_offset_ = offset;
    update();
}

void TabButton::SetActive(const bool active) {
    is_active_ = active;
    update();
}

void TabButton::enterEvent(QEvent *event) {
    if (!is_active_) {
        const auto animation = new QPropertyAnimation(this, "underlineOffset");
        animation->setDuration(300);
        animation->setStartValue(0.0);
        animation->setEndValue(5.0);
        animation->setEasingCurve(QEasingCurve::InOutQuad);
        animation->start(QPropertyAnimation::DeleteWhenStopped);
    }
    QPushButton::enterEvent(event);
}

void TabButton::leaveEvent(QEvent *event) {
    if (!is_active_) {
        const auto animation = new QPropertyAnimation(this, "underlineOffset");
        animation->setDuration(300);
        animation->setStartValue(underline_offset_);
        animation->setEndValue(0.0);
        animation->setEasingCurve(QEasingCurve::InOutQuad);
        animation->start(QPropertyAnimation::DeleteWhenStopped);
    }
    QPushButton::leaveEvent(event);
}

void TabButton::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor underline_color = is_active_ ? QColor(0, 220, 255) : QColor(0, 180, 220, 180);

    // underline
    const int line_y = height() - 5;

    if (is_active_) {
        painter.setPen(QPen(underline_color, 2));
        painter.drawLine(5, line_y, width() - 5, line_y);
    } else if (underline_offset_ > 0) {
        painter.setPen(QPen(underline_color, 1.5));

        const double offset = sin(underline_offset_) * 2.0;
        const int center_x = width() / 2;
        const int line_width = width() * 0.6 * (underline_offset_ / 5.0);

        painter.drawLine(center_x - line_width / 2 + offset, line_y,
                         center_x + line_width / 2 + offset, line_y);
    }
}
