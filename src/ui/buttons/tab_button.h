//
// Created by szymo on 18.04.2025.
//

#ifndef TAB_BUTTON_H
#define TAB_BUTTON_H
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>


class TabButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double underlineOffset READ underlineOffset WRITE setUnderlineOffset)

public:
    explicit TabButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent), m_underlineOffset(0), m_isActive(false) {

        setFlat(true);
        setCursor(Qt::PointingHandCursor);
        setCheckable(true);

        // Styl przycisku zakładki
        setStyleSheet(
            "TabButton {"
            "  color: #00ccff;"
            "  background-color: transparent;"
            "  border: none;"
            "  font-family: 'Consolas';"
            "  font-size: 11pt;"
            "  padding: 5px 15px;"
            "  margin: 0px 10px;"
            "  text-align: center;"
            "}"
            "TabButton:hover {"
            "  color: #00eeff;"
            "}"
            "TabButton:checked {"
            "  color: #ffffff;"
            "}"
        );
    }

    double underlineOffset() const { return m_underlineOffset; }

    void setUnderlineOffset(double offset) {
        m_underlineOffset = offset;
        update();
    }

    void setActive(bool active) {
        m_isActive = active;
        update();
    }


protected:
    void enterEvent(QEvent *event) override {
        if (!m_isActive) {
            QPropertyAnimation *anim = new QPropertyAnimation(this, "underlineOffset");
            anim->setDuration(300);
            anim->setStartValue(0.0);
            anim->setEndValue(5.0);
            anim->setEasingCurve(QEasingCurve::InOutQuad);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        if (!m_isActive) {
            QPropertyAnimation *anim = new QPropertyAnimation(this, "underlineOffset");
            anim->setDuration(300);
            anim->setStartValue(m_underlineOffset);
            anim->setEndValue(0.0);
            anim->setEasingCurve(QEasingCurve::InOutQuad);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
        QPushButton::leaveEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        QPushButton::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor underlineColor = m_isActive ? QColor(0, 220, 255) : QColor(0, 180, 220, 180);

        // Rysowanie podkreślenia
        int lineY = height() - 5;

        if (m_isActive) {
            // Aktywna zakładka ma pełne podkreślenie
            painter.setPen(QPen(underlineColor, 2));
            painter.drawLine(5, lineY, width() - 5, lineY);
        } else if (m_underlineOffset > 0) {
            // Zakładka z animującym się podkreśleniem przy najechaniu
            painter.setPen(QPen(underlineColor, 1.5));

            // Animowane podkreślenie porusza się lekko w poziomie
            double offset = sin(m_underlineOffset) * 2.0;
            int centerX = width() / 2;
            int lineWidth = width() * 0.6 * (m_underlineOffset / 5.0);

            painter.drawLine(centerX - lineWidth/2 + offset, lineY,
                            centerX + lineWidth/2 + offset, lineY);
        }
    }

private:
    double m_underlineOffset;
    bool m_isActive;
};


#endif //TAB_BUTTON_H
