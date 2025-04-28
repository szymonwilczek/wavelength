#ifndef CYBERPUNK_BUTTON_H
#define CYBERPUNK_BUTTON_H

#include <QPushButton>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

#include "../../blob/core/dynamics/blob_event_handler.h"

class CyberpunkButton : public QPushButton {
    Q_OBJECT

    // Nowa właściwość dla efektu propagacji światła
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberpunkButton(const QString &text, QWidget *parent = nullptr);
    ~CyberpunkButton() override;

    // Gettery i settery dla nowej animacji
    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPropertyAnimation *m_animation = nullptr;
    QPropertyAnimation *m_glowAnimation;
    QGraphicsDropShadowEffect *m_glowEffect;

    // Zmienne dla efektu propagacji światła
    qreal m_glowIntensity;
    QColor m_frameColor;
    QColor m_glowColor;
    int m_frameWidth;
};

#endif // CYBERPUNK_BUTTON_H