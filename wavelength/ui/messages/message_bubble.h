#ifndef MESSAGE_BUBBLE_H
#define MESSAGE_BUBBLE_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QTextDocument>
#include <QStyle>
#include <QStyleOption>

class MessageBubble : public QFrame {
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(QPointF position READ position WRITE setPosition)

public:
    enum BubbleType {
        SentMessage,
        ReceivedMessage,
        SystemMessage
    };

    explicit MessageBubble(const QString& message, BubbleType type = ReceivedMessage, QWidget* parent = nullptr);
    ~MessageBubble();

    // Metody do animacji
    qreal scale() const { return m_scale; }
    void setScale(qreal scale);

    QPointF position() const { return m_position; }
    void setPosition(const QPointF& pos);

    // Metoda do animacji wejścia
    void startEntryAnimation();

    // Zaawansowana metoda animacji wejścia (z opcjonalnym opóźnieniem)
    void startAdvancedEntryAnimation(bool delayed = false);

    // Optymalne wymiary dla dymka
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    BubbleType m_type;
    QLabel* m_messageLabel;
    QGraphicsOpacityEffect* m_opacityEffect;
    qreal m_scale;
    QPointF m_position;

    // Do optymalizacji renderowania
    QPainterPath m_bubblePath;
    void updateBubblePath();
};

#endif // MESSAGE_BUBBLE_H