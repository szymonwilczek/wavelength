//
// Created by szymo on 6.04.2025.
//

#ifndef CYBER_BUTTON_H
#define CYBER_BUTTON_H
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>


class CyberButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberButton(const QString& text, QWidget* parent = nullptr, bool isPrimary = true);

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity);

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    double m_glowIntensity;
    bool m_isPrimary;
};



#endif //CYBER_BUTTON_H
