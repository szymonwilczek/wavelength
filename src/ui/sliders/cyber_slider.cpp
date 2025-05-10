#include "cyber_slider.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QStyle>

CyberSlider::CyberSlider(const Qt::Orientation orientation, QWidget *parent): QSlider(orientation, parent),
                                                                              glow_intensity_(0.5) {
    setStyleSheet("background: transparent; border: none;");
}

void CyberSlider::SetGlowIntensity(const double intensity) {
    glow_intensity_ = intensity;
    update();
}

void CyberSlider::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor track_color(0, 60, 80);
    QColor progress_color(0, 200, 255);
    QColor handle_color(0, 240, 255);

    constexpr int handle_width = 14;
    constexpr int handle_height = 20;
    constexpr int track_height = 4;

    QRect track_rect = rect().adjusted(5, (height() - track_height) / 2, -5, -(height() - track_height) / 2);

    QPainterPath track_path;
    track_path.addRoundedRect(track_rect, 2, 2);

    painter.setPen(Qt::NoPen);
    painter.setBrush(track_color);
    painter.drawPath(track_path);

    int handle_position = QStyle::sliderPositionFromValue(minimum(), maximum(), value(), width() - handle_width);

    if (value() > minimum()) {
        QRect progress_rect = track_rect;
        progress_rect.setWidth(handle_position + handle_width / 2);

        QPainterPath progress_path;
        progress_path.addRoundedRect(progress_rect, 2, 2);

        painter.setBrush(progress_color);
        painter.drawPath(progress_path);

        painter.setOpacity(0.4);
        painter.setPen(QPen(QColor(255, 255, 255, 40), 1));

        for (int i = 0; i < progress_rect.width(); i += 15) {
            painter.drawLine(i, progress_rect.top(), i, progress_rect.bottom());
        }
        painter.setOpacity(1.0);
    }

    QRect handle_rect(handle_position, (height() - handle_height) / 2, handle_width, handle_height);

    QPainterPath handle_path;
    handle_path.addRoundedRect(handle_rect, 3, 3);

    painter.setPen(QPen(handle_color, 1));
    painter.setBrush(QColor(0, 50, 70));
    painter.drawPath(handle_path);

    painter.setPen(QPen(handle_color.lighter(), 1));
    painter.drawLine(handle_rect.left() + 4, handle_rect.top() + 4,
                     handle_rect.right() - 4, handle_rect.top() + 4);
    painter.drawLine(handle_rect.left() + 4, handle_rect.bottom() - 4,
                     handle_rect.right() - 4, handle_rect.bottom() - 4);
}

void CyberSlider::enterEvent(QEvent *event) {
    QSlider::enterEvent(event);
}

void CyberSlider::leaveEvent(QEvent *event) {
    QSlider::leaveEvent(event);
}
