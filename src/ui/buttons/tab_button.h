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

    double UnderlineOffset() const { return underline_offset_; }

    void SetUnderlineOffset(double offset);

    void SetActive(bool active);

protected:
    void enterEvent(QEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private:
    double underline_offset_;
    bool is_active_;
};


#endif //TAB_BUTTON_H
