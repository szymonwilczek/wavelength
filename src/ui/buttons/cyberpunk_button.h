#ifndef CYBERPUNK_BUTTON_H
#define CYBERPUNK_BUTTON_H

#include <QPushButton>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

#include "../../blob/core/dynamics/blob_event_handler.h"

class CyberpunkButton final : public QPushButton {
    Q_OBJECT

    // Nowa właściwość dla efektu propagacji światła
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberpunkButton(const QString &text, QWidget *parent = nullptr);
    ~CyberpunkButton() override;

    // Gettery i settery dla nowej animacji
    qreal GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(qreal intensity);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPropertyAnimation *animation_ = nullptr;
    QPropertyAnimation *glow_animation_;
    QGraphicsDropShadowEffect *glow_effect_;

    // Zmienne dla efektu propagacji światła
    qreal glow_intensity_;
    QColor frame_color_;
    QColor glow_color_;
    int frame_width_;
};

#endif // CYBERPUNK_BUTTON_H