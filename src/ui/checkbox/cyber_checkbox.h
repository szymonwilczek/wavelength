#ifndef CYBER_CHECKBOX_H
#define CYBER_CHECKBOX_H
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionButton>


class CyberCheckBox final : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberCheckBox(const QString& text, QWidget* parent = nullptr);

    QSize sizeHint() const override;

    double GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(double intensity);

protected:
    void paintEvent(QPaintEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

private:
    double glow_intensity_;
};



#endif //CYBER_CHECKBOX_H
