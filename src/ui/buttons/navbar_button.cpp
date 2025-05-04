#include "navbar_button.h"
#include <QResizeEvent>
#include <QHBoxLayout>
#include <QStyleOption>
#include <QPainter>

CyberpunkButton::CyberpunkButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent) {
    animation_ = nullptr;

    // Inicjalizacja zmiennych
    glow_intensity_ = 0.0;
    frame_width_ = 2;
    frame_color_ = QColor(0, 229, 255, 120); // Kolor ramki z mniejszą intensywnością
    glow_color_ = QColor(0, 255, 255, 200); // Kolor poświaty

    // Styl bazowego przycisku (bez underline)
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

    // Sprawienie żeby przycisk zawsze zajmował właściwą wielkość dla tekstu
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumWidth(this->fontMetrics().horizontalAdvance(text) + 32);
    setMinimumHeight(36); // Zwiększona wysokość przycisku z 36 na 48

    // Efekt poświaty dla tekstu
    glow_effect_ = new QGraphicsDropShadowEffect(this);
    glow_effect_->setBlurRadius(10);
    glow_effect_->setColor(QColor(0, 255, 255, 160));
    glow_effect_->setOffset(0, 0);
    setGraphicsEffect(glow_effect_);


    // Animacja propagacji światła
    glow_animation_ = new QPropertyAnimation(this, "glowIntensity");
    glow_animation_->setDuration(600); // Dłuższa animacja dla płynniejszego efektu
    glow_animation_->setEasingCurve(QEasingCurve::OutQuad);
}

CyberpunkButton::~CyberpunkButton() {
    delete animation_;
    delete glow_animation_;
}

void CyberpunkButton::paintEvent(QPaintEvent *event) {
    // Najpierw wywołujemy domyślny paintEvent
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysujemy ramkę neonową - jej jasność jest kontrolowana przez m_glowIntensity
    QPen frame_pen;

    // Kolor ramki z jasnością zależną od intensywności
    QColor border_color;

    // Bazowy kolor to niebieski neon
    int r = 0;
    int g = 229;
    int b = 255;

    // Alpha (przezroczystość) i grubość zależne od intensywności
    int base_alpha = 120;
    int max_alpha = 200;
    int alpha = base_alpha + (max_alpha - base_alpha) * glow_intensity_;

    // Zwiększamy grubość ramki w zależności od intensywności
    float base_width = 2.0;
    float max_width_increase = 0.5; // Maksymalny przyrost grubości
    float pen_width = base_width + (glow_intensity_ * max_width_increase);

    border_color = QColor(r, g, b, alpha);
    frame_pen.setColor(border_color);
    frame_pen.setWidthF(pen_width);

    painter.setPen(frame_pen);

    // Rysujemy prostokąt z zaokrąglonymi rogami dla ramki
    QRectF frame_rect = rect().adjusted(pen_width/2, pen_width/2, -pen_width/2, -pen_width/2);
    painter.drawRoundedRect(frame_rect, 5, 5);

    // Dodajemy efekt poświaty dla samej ramki - opcjonalnie
    if (glow_intensity_ > 0.3) { // Tylko przy większej intensywności
        QPen glow_pen;
        auto glow_color = QColor(r, g, b, static_cast<int>(70 * glow_intensity_));
        glow_pen.setColor(glow_color);
        glow_pen.setWidthF(pen_width + 0.6);
        painter.setPen(glow_pen);

        // Rysujemy drugą, cieńszą ramkę na zewnątrz dla efektu poświaty
        QRectF outer_rect = frame_rect.adjusted(-0.5, -0.5, 0.5, 0.5);
        painter.drawRoundedRect(outer_rect, 5, 5);
    }
}

void CyberpunkButton::SetGlowIntensity(const qreal intensity) {
    glow_intensity_ = intensity;

    // Aktualizacja efektu poświaty dla tekstu
    if (glow_effect_) {
        // Zwiększenie blasku i promienia poświaty dla tekstu
        constexpr int base_blur = 7;
        constexpr int delta_blur = 2;
        glow_effect_->setBlurRadius(base_blur + delta_blur * intensity);

        // Zwiększenie jasności poświaty tekstu
        const auto glow_color = QColor(0, 255, 255, 160 + 95 * intensity);
        glow_effect_->setColor(glow_color);
    }

    update(); // Wymusza przerysowanie przycisku
}

void CyberpunkButton::enterEvent(QEvent *event) {
    QPushButton::enterEvent(event);

    // Animacja rozjaśnienia i propagacji światła
    glow_animation_->setStartValue(0.0);
    glow_animation_->setEndValue(1.0);
    glow_animation_->start();
}

void CyberpunkButton::leaveEvent(QEvent *event) {
    QPushButton::leaveEvent(event);
    // Animacja wygaszenia propagacji światła
    glow_animation_->setStartValue(glow_intensity_);
    glow_animation_->setEndValue(0.0);
    glow_animation_->start();
}

void CyberpunkButton::resizeEvent(QResizeEvent *event) {
    QPushButton::resizeEvent(event);
}