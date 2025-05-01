#ifndef CYBER_PUSH_BUTTON_H
#define CYBER_PUSH_BUTTON_H
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTimer>


class CyberPushButton final : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberPushButton(const QString& text, QWidget* parent = nullptr);

    ~CyberPushButton() override {
        if (pulse_timer_) {
            pulse_timer_->stop();
        }
    }

    double GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(const double intensity) {
        glow_intensity_ = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override {
        base_glow_intensity_ = 0.8;
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        base_glow_intensity_ = 0.5;
        QPushButton::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        base_glow_intensity_ = 1.0;
        QPushButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        base_glow_intensity_ = 0.8;
        QPushButton::mouseReleaseEvent(event);
    }

private:
    double glow_intensity_;
    double base_glow_intensity_;
    QTimer* pulse_timer_;
};



#endif //CYBER_PUSH_BUTTON_H
