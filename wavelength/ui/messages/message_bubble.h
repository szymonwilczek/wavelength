#ifndef MESSAGE_BUBBLE_H
#define MESSAGE_BUBBLE_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
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

    explicit MessageBubble(const QString& message, BubbleType type = ReceivedMessage, QWidget* parent = nullptr)
        : QFrame(parent)
        , m_type(type)
        , m_scale(0.95)
        , m_position(0, 0)
    {
        setObjectName("messageBubble");
        setAttribute(Qt::WA_TranslucentBackground);
        
        // Konfiguracja layoutu
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(13, 10, 13, 10);
        layout->setSpacing(3);
        
        // Etykieta wiadomości
        m_messageLabel = new QLabel(message, this);
        m_messageLabel->setTextFormat(Qt::RichText);
        m_messageLabel->setWordWrap(true);
        m_messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_messageLabel->setOpenExternalLinks(true);
        
        // Dostosowanie stylu w zależności od typu wiadomości
        switch(m_type) {
            case SentMessage:
                setStyleSheet("background-color: #0b93f6; border-radius: 18px;");
                m_messageLabel->setStyleSheet("color: white; background: transparent;");
                break;
            case ReceivedMessage:
                setStyleSheet("background-color: #262626; border-radius: 18px;");
                m_messageLabel->setStyleSheet("color: white; background: transparent;");
                break;
            case SystemMessage:
                setStyleSheet("background-color: #3a3a3a; border-radius: 14px;");
                m_messageLabel->setStyleSheet("color: #ffcc00; background: transparent;");
                break;
        }
        
        layout->addWidget(m_messageLabel);
        
        // Przygotowanie efektów dla animacji
        m_opacityEffect = new QGraphicsOpacityEffect(this);
        m_opacityEffect->setOpacity(0.0);
        setGraphicsEffect(m_opacityEffect);
    }
    
    // Metody do animacji
    qreal scale() const { return m_scale; }
    void setScale(qreal scale) {
        m_scale = scale;
        updateGeometry();
        update();
    }
    
    QPointF position() const { return m_position; }
    void setPosition(const QPointF& pos) {
        m_position = pos;
        move(pos.x(), pos.y());
    }

    QSize MessageBubble::sizeHint() const;
    void MessageBubble::startAdvancedEntryAnimation() ;
    
    void startEntryAnimation() {
        // 1. Animacja opacity
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity");
        opacityAnim->setDuration(280);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
        
        // 2. Animacja powiększania
        QPropertyAnimation* scaleAnim = new QPropertyAnimation(this, "scale");
        scaleAnim->setDuration(320);
        scaleAnim->setStartValue(0.95);
        scaleAnim->setEndValue(1.0);
        scaleAnim->setEasingCurve(QEasingCurve::OutBack);
        
        // 3. Animacja pojawienia się z boku (zależnie od typu)
        QPropertyAnimation* posAnim = new QPropertyAnimation(this, "position");
        posAnim->setDuration(280);
        QPointF startPos = pos();
        
        if (m_type == SentMessage) {
            // Wiadomości wysłane wjeżdżają z prawej
            posAnim->setStartValue(QPointF(startPos.x() + 20, startPos.y()));
        } else {
            // Wiadomości odebrane wjeżdżają z lewej
            posAnim->setStartValue(QPointF(startPos.x() - 20, startPos.y()));
        }
        
        posAnim->setEndValue(startPos);
        posAnim->setEasingCurve(QEasingCurve::OutCubic);
        
        // Grupa animacji
        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(opacityAnim);
        group->addAnimation(scaleAnim);
        group->addAnimation(posAnim);
        
        // Rozpocznij animację
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter painter(this);
        
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setOpacity(m_opacityEffect->opacity());
        
        // Ścieżka dla zaokrąglonego prostokąta
        QPainterPath path;
        path.addRoundedRect(rect(), 18, 18);
        
        // Zastosowanie transformacji skali
        QTransform transform;
        transform.translate(width() / 2, height() / 2);
        transform.scale(m_scale, m_scale);
        transform.translate(-width() / 2, -height() / 2);
        painter.setTransform(transform);
        
        // Rysowanie tła
        painter.fillPath(path, opt.palette.color(QPalette::Window));
        
        // Wywołanie rysowania standardowych elementów
        style()->drawPrimitive(QStyle::PE_Frame, &opt, &painter, this);
        
        // Wywołanie rysowania potomnych widgetów (label)
        QFrame::paintEvent(event);
    }
    
private:
    BubbleType m_type;
    QLabel* m_messageLabel;
    QGraphicsOpacityEffect* m_opacityEffect;
    qreal m_scale;
    QPointF m_position;
};

#endif // MESSAGE_BUBBLE_H