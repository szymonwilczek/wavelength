#ifndef CYBER_PUSH_BUTTON_H
#define CYBER_PUSH_BUTTON_H
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTimer>


class CyberPushButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberPushButton(const QString& text, QWidget* parent = nullptr);

    ~CyberPushButton() {
        if (m_pulseTimer) {
            m_pulseTimer->stop();
        }
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override {
        m_baseGlowIntensity = 0.8;
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        m_baseGlowIntensity = 0.5;
        QPushButton::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        m_baseGlowIntensity = 1.0;
        QPushButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        m_baseGlowIntensity = 0.8;
        QPushButton::mouseReleaseEvent(event);
    }

private:
    double m_glowIntensity;
    double m_baseGlowIntensity;
    QTimer* m_pulseTimer;
};



#endif //CYBER_PUSH_BUTTON_H
