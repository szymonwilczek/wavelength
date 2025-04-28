#ifndef CYBER_CHAT_BUTTON_H
#define CYBER_CHAT_BUTTON_H
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>


class CyberChatButton : public QPushButton {
    Q_OBJECT

public:
    CyberChatButton(const QString &text, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};



#endif //CYBER_CHAT_BUTTON_H
