#ifndef CYBER_AUDIO_SLIDER_H
#define CYBER_AUDIO_SLIDER_H
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QSlider>
#include <QStyle>


class CyberAudioSlider final : public QSlider {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberAudioSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity);

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

private:
    double m_glowIntensity;
};



#endif //CYBER_AUDIO_SLIDER_H
