#ifndef CYBER_INPUT_H
#define CYBER_INPUT_H
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>

class CyberLineEdit final : public QLineEdit {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    explicit CyberLineEdit(QWidget* parent = nullptr);

    ~CyberLineEdit() override;

    QSize sizeHint() const override;

    double GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(double intensity);

    QRect CyberCursorRect() const;

protected:
    void paintEvent(QPaintEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

private:
    double glow_intensity_;
    QTimer* cursor_blink_timer_;
    bool cursor_visible_;
};


#endif //CYBER_INPUT_H
