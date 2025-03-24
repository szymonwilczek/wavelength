#ifndef SYSTEM_RADIO_MESSAGE_H
#define SYSTEM_RADIO_MESSAGE_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QGraphicsBlurEffect>
#include <QTimer>
#include <QVBoxLayout>

class SystemRadioMessage : public QWidget {
    Q_OBJECT

public:
    explicit SystemRadioMessage(const QString& message, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName("systemRadioMessage");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        
        // Transparentne tło
        setAttribute(Qt::WA_TranslucentBackground);
        
        // Tworzymy QLabel dla wiadomości
        m_messageLabel = new QLabel(message, this);
        m_messageLabel->setAlignment(Qt::AlignCenter);
        m_messageLabel->setWordWrap(true);
        m_messageLabel->setTextFormat(Qt::RichText);
        m_messageLabel->setStyleSheet(
            "color: #ffcc00; "
            "font-family: 'Courier New', monospace; "
            "font-weight: bold; "
            "background: transparent; "
            "padding: 10px;"
        );
        
        // Efekty dla animacji
        m_opacityEffect = new QGraphicsOpacityEffect(this);
        m_opacityEffect->setOpacity(0.0);
        m_messageLabel->setGraphicsEffect(m_opacityEffect);
        
        // Układ
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 5, 0, 5);
        layout->addWidget(m_messageLabel, 0, Qt::AlignCenter);
    }
    
    void startEntryAnimation() {
        // Animacja pojawiania
        QPropertyAnimation* opacityAnim1 = new QPropertyAnimation(m_opacityEffect, "opacity");
        opacityAnim1->setDuration(600);
        opacityAnim1->setStartValue(0.0);
        opacityAnim1->setEndValue(1.0);
        opacityAnim1->setEasingCurve(QEasingCurve::OutCubic);
        
        // Animacja znikania po 3 sekundach
        QPropertyAnimation* opacityAnim2 = new QPropertyAnimation(m_opacityEffect, "opacity");
        opacityAnim2->setDuration(800);
        opacityAnim2->setStartValue(1.0);
        opacityAnim2->setEndValue(0.7);
        opacityAnim2->setEasingCurve(QEasingCurve::InCubic);
        
        // Sekwencja animacji
        QSequentialAnimationGroup* sequence = new QSequentialAnimationGroup(this);
        sequence->addAnimation(opacityAnim1);
        sequence->addPause(3000); // Pauza 3 sekundy
        sequence->addAnimation(opacityAnim2);
        sequence->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
    QSize sizeHint() const override {
        return m_messageLabel->sizeHint();
    }

private:
    QLabel* m_messageLabel;
    QGraphicsOpacityEffect* m_opacityEffect;
};

#endif // SYSTEM_RADIO_MESSAGE_H