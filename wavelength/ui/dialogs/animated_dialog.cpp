#include "animated_dialog.h"
#include <QPropertyAnimation>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>

AnimatedDialog::AnimatedDialog(QWidget *parent, AnimationType type)
    : QDialog(parent), 
      m_animationType(type),
      m_duration(300),
      m_closing(false)
{
    // Niezbędne dla animacji
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

AnimatedDialog::~AnimatedDialog()
{
}

void AnimatedDialog::showEvent(QShowEvent *event)
{
    event->accept();
    animateShow();
}

void AnimatedDialog::closeEvent(QCloseEvent *event)
{
    if (!m_closing) {
        event->ignore();
        m_closing = true;
        animateClose();
    } else {
        event->accept();
    }
}

void AnimatedDialog::animateShow()
{
    QPropertyAnimation *animation = nullptr;
    
    // Przechowujemy początkową geometrię
    QRect geometry = this->geometry();
    
    switch (m_animationType) {
        case SlideFromBottom: {
            // Rozpoczynamy poza ekranem na dole
            QRect startGeometry = geometry;
            startGeometry.moveBottom(QGuiApplication::screenAt(geometry.center())->geometry().bottom() + 50);
            this->setGeometry(startGeometry);
            
            // Animacja przesunięcia
            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(startGeometry);
            animation->setEndValue(geometry);
            animation->setEasingCurve(QEasingCurve::OutQuint);
            break;
        }
        
        case FadeIn: {
            // Efekt przezroczystości
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
            this->setGraphicsEffect(effect);
            effect->setOpacity(0);
            
            animation = new QPropertyAnimation(effect, "opacity");
            animation->setStartValue(0.0);
            animation->setEndValue(1.0);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            break;
        }
        
        case ScaleFromCenter: {
            // Rozpoczynamy z małego punktu na środku
            QPoint center = geometry.center();
            QRect startGeometry(center.x(), center.y(), 0, 0);
            startGeometry.moveCenter(center);
            this->setGeometry(startGeometry);
            
            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(startGeometry);
            animation->setEndValue(geometry);
            animation->setEasingCurve(QEasingCurve::OutBack);
            break;
        }
    }
    
    if (animation) {
        animation->setDuration(m_duration);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void AnimatedDialog::animateClose()
{
    QPropertyAnimation *animation = nullptr;
    
    // Przechowujemy końcową geometrię (poza ekranem)
    QRect endGeometry = this->geometry();
    
    switch (m_animationType) {
        case SlideFromBottom: {
            // Przesuwamy poza ekran na dole
            endGeometry.moveTop(QGuiApplication::screenAt(endGeometry.center())->geometry().bottom());
            
            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(this->geometry());
            animation->setEndValue(endGeometry);
            animation->setEasingCurve(QEasingCurve::InQuint);
            break;
        }
        
        case FadeIn: {
            // Efekt przezroczystości powinien już być ustawiony z animateShow()
            QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect());
            if (!effect) {
                effect = new QGraphicsOpacityEffect(this);
                this->setGraphicsEffect(effect);
            }
            
            animation = new QPropertyAnimation(effect, "opacity");
            animation->setStartValue(1.0);
            animation->setEndValue(0.0);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            break;
        }
        
        case ScaleFromCenter: {
            // Kurczymy do punktu na środku
            QPoint center = this->geometry().center();
            QRect startGeometry = this->geometry();
            QRect endGeometry(0, 0, 0, 0);
            endGeometry.moveCenter(center);
            
            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(startGeometry);
            animation->setEndValue(endGeometry);
            animation->setEasingCurve(QEasingCurve::InBack);
            break;
        }
    }
    
    if (animation) {
        animation->setDuration(m_duration);
        connect(animation, &QPropertyAnimation::finished, this, [this]() {
            QDialog::close();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}