#ifndef CYBER_SLIDER_H
#define CYBER_SLIDER_H
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QSlider>


class CyberSlider : public QSlider {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity);

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

private:
    double m_glowIntensity;
};



#endif //CYBER_SLIDER_H
