#ifndef TAB_BUTTON_H
#define TAB_BUTTON_H
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>


class TabButton final : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double underlineOffset READ underlineOffset WRITE setUnderlineOffset)

public:
    explicit TabButton(const QString &text, QWidget *parent = nullptr);

    double underlineOffset() const { return m_underlineOffset; }

    void setUnderlineOffset(double offset);

    void setActive(bool active);

protected:
    void enterEvent(QEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private:
    double m_underlineOffset;
    bool m_isActive;
};


#endif //TAB_BUTTON_H
