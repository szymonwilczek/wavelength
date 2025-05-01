#ifndef CYBER_BUTTON_H
#define CYBER_BUTTON_H

#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>


class CyberButton final : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberButton(const QString& text, QWidget* parent = nullptr, bool isPrimary = true);

    double GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(double intensity);

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    double glow_intensity_;
    bool is_primary_;
};



#endif //CYBER_BUTTON_H
