#ifndef CYBER_INPUT_H
#define CYBER_INPUT_H
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>


// Cyberpunkowe pole tekstowe
class CyberLineEdit : public QLineEdit {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberLineEdit(QWidget* parent = nullptr);

    ~CyberLineEdit() override;

    QSize sizeHint() const override;

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity);

    QRect cursorRect() const;

protected:
    void paintEvent(QPaintEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

    // Obsługa zdarzeń klawiatury - zresetuj kursor
    void keyPressEvent(QKeyEvent* event) override;

private:
    double m_glowIntensity;
    QTimer* m_cursorBlinkTimer;
    bool m_cursorVisible;
};


#endif //CYBER_INPUT_H
