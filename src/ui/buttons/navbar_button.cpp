#include "navbar_button.h"

#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPropertyAnimation>

CyberpunkButton::CyberpunkButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent) {
    glow_intensity_ = 0.0;
    frame_width_ = 2;
    frame_color_ = QColor(0, 229, 255, 120);
    glow_color_ = QColor(0, 255, 255, 200);

    setStyleSheet(
        "CyberpunkButton {"
        "  background-color: transparent;"
        "  color: #00e5ff;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  font-family: 'Rajdhani', 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  text-transform: uppercase;"
        "  letter-spacing: 1px;"
        "}"
        "CyberpunkButton:hover {"
        "  color: #00ffff;"
        "}"
        "CyberpunkButton:pressed {"
        "  color: #80ffff;"
        "}"
    );

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumWidth(this->fontMetrics().horizontalAdvance(text) + 32);
    setMinimumHeight(36);

    glow_effect_ = new QGraphicsDropShadowEffect(this);
    glow_effect_->setBlurRadius(10);
    glow_effect_->setColor(QColor(0, 255, 255, 160));
    glow_effect_->setOffset(0, 0);
    setGraphicsEffect(glow_effect_);

    glow_animation_ = new QPropertyAnimation(this, "glowIntensity");
    glow_animation_->setDuration(600); // Dłuższa animacja dla płynniejszego efektu
    glow_animation_->setEasingCurve(QEasingCurve::OutQuad);
}

CyberpunkButton::~CyberpunkButton() {
    delete glow_animation_;
}

void CyberpunkButton::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen frame_pen;
    QColor border_color;

    int r = 0;
    int g = 229;
    int b = 255;

    int base_alpha = 120;
    int max_alpha = 200;
    int alpha = base_alpha + (max_alpha - base_alpha) * glow_intensity_;

    float base_width = 2.0;
    float max_width_increase = 0.5;
    float pen_width = base_width + glow_intensity_ * max_width_increase;

    border_color = QColor(r, g, b, alpha);
    frame_pen.setColor(border_color);
    frame_pen.setWidthF(pen_width);

    painter.setPen(frame_pen);

    QRectF frame_rect = rect().adjusted(pen_width / 2, pen_width / 2, -pen_width / 2, -pen_width / 2);
    painter.drawRoundedRect(frame_rect, 5, 5);

    if (glow_intensity_ > 0.3) {
        QPen glow_pen;
        auto glow_color = QColor(r, g, b, static_cast<int>(70 * glow_intensity_));
        glow_pen.setColor(glow_color);
        glow_pen.setWidthF(pen_width + 0.6);
        painter.setPen(glow_pen);

        QRectF outer_rect = frame_rect.adjusted(-0.5, -0.5, 0.5, 0.5);
        painter.drawRoundedRect(outer_rect, 5, 5);
    }
}

void CyberpunkButton::SetGlowIntensity(const qreal intensity) {
    glow_intensity_ = intensity;

    if (glow_effect_) {
        constexpr int base_blur = 7;
        constexpr int delta_blur = 2;
        glow_effect_->setBlurRadius(base_blur + delta_blur * intensity);

        const auto glow_color = QColor(0, 255, 255, 160 + 95 * intensity);
        glow_effect_->setColor(glow_color);
    }

    update();
}

void CyberpunkButton::enterEvent(QEvent *event) {
    QPushButton::enterEvent(event);

    glow_animation_->setStartValue(0.0);
    glow_animation_->setEndValue(1.0);
    glow_animation_->start();
}

void CyberpunkButton::leaveEvent(QEvent *event) {
    QPushButton::leaveEvent(event);
    glow_animation_->setStartValue(glow_intensity_);
    glow_animation_->setEndValue(0.0);
    glow_animation_->start();
}

void CyberpunkButton::resizeEvent(QResizeEvent *event) {
    QPushButton::resizeEvent(event);
}
