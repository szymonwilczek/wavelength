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

    double GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(double intensity);

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

private:
    double glow_intensity_;
};



#endif //CYBER_AUDIO_SLIDER_H
