#include "animated_stacked_widget.h"

AnimatedStackedWidget::AnimatedStackedWidget(QWidget *parent)
    : QStackedWidget(parent),
      m_duration(500),
      m_animationType(Fade),
      m_animationGroup(new QParallelAnimationGroup(this)),
      m_animationRunning(false)
{
    connect(m_animationGroup, &QParallelAnimationGroup::finished, [this]() {
        m_animationRunning = false;
        setCurrentIndex(currentIndex() + 1 >= count() ? 0 : currentIndex() + 1);
        for (int i = 0; i < count(); ++i) {
            widget(i)->setGraphicsEffect(nullptr);
        }
    });
}

AnimatedStackedWidget::~AnimatedStackedWidget()
{
    delete m_animationGroup;
}

void AnimatedStackedWidget::slideToIndex(int index)
{
    if (m_animationRunning || index == currentIndex() || index < 0 || index >= count())
        return;

    m_animationRunning = true;

    switch (m_animationType) {
    case Fade:
        animateFade(index);
        break;
    case Slide:
        animateSlide(index);
        break;
    case SlideAndFade:
        animateSlideAndFade(index);
        break;
    case Push:
        animatePush(index);
        break;
    }

    m_animationGroup->start();
}

void AnimatedStackedWidget::slideToWidget(QWidget *widget)
{
    slideToIndex(indexOf(widget));
}

void AnimatedStackedWidget::animateFade(int nextIndex)
{
    QWidget *currentWidget = widget(currentIndex());
    QWidget *nextWidget = widget(nextIndex);

    // Ustaw następny widget na wierzchu
    nextWidget->setVisible(true);
    nextWidget->raise();
    currentWidget->raise();

    // Efekt przezroczystości dla bieżącego widgetu
    QGraphicsOpacityEffect *currentEffect = new QGraphicsOpacityEffect(currentWidget);
    currentWidget->setGraphicsEffect(currentEffect);

    // Efekt przezroczystości dla następnego widgetu
    QGraphicsOpacityEffect *nextEffect = new QGraphicsOpacityEffect(nextWidget);
    nextWidget->setGraphicsEffect(nextEffect);
    nextEffect->setOpacity(0);

    // Animacja zanikania bieżącego widgetu
    QPropertyAnimation *fadeOutAnimation = new QPropertyAnimation(currentEffect, "opacity");
    fadeOutAnimation->setDuration(m_duration);
    fadeOutAnimation->setStartValue(1.0);
    fadeOutAnimation->setEndValue(0.0);
    fadeOutAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja pojawiania się następnego widgetu
    QPropertyAnimation *fadeInAnimation = new QPropertyAnimation(nextEffect, "opacity");
    fadeInAnimation->setDuration(m_duration);
    fadeInAnimation->setStartValue(0.0);
    fadeInAnimation->setEndValue(1.0);
    fadeInAnimation->setEasingCurve(QEasingCurve::InCubic);

    m_animationGroup->clear();
    m_animationGroup->addAnimation(fadeOutAnimation);
    m_animationGroup->addAnimation(fadeInAnimation);
}

void AnimatedStackedWidget::animateSlide(int nextIndex)
{
    QWidget *currentWidget = widget(currentIndex());
    QWidget *nextWidget = widget(nextIndex);

    // Pozycje początkowe
    int width = currentWidget->width();
    nextWidget->setGeometry(width, 0, width, height());
    
    // Pokaż następny widget
    nextWidget->show();
    nextWidget->raise();

    // Animacja przesunięcia bieżącego widgetu
    QPropertyAnimation *currentAnimation = new QPropertyAnimation(currentWidget, "geometry");
    currentAnimation->setDuration(m_duration);
    currentAnimation->setStartValue(QRect(0, 0, width, height()));
    currentAnimation->setEndValue(QRect(-width, 0, width, height()));
    currentAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja przesunięcia następnego widgetu
    QPropertyAnimation *nextAnimation = new QPropertyAnimation(nextWidget, "geometry");
    nextAnimation->setDuration(m_duration);
    nextAnimation->setStartValue(QRect(width, 0, width, height()));
    nextAnimation->setEndValue(QRect(0, 0, width, height()));
    nextAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_animationGroup->clear();
    m_animationGroup->addAnimation(currentAnimation);
    m_animationGroup->addAnimation(nextAnimation);
}

void AnimatedStackedWidget::animateSlideAndFade(int nextIndex)
{
    QWidget *currentWidget = widget(currentIndex());
    QWidget *nextWidget = widget(nextIndex);

    // Pozycje początkowe
    int width = currentWidget->width();
    nextWidget->setGeometry(width, 0, width, height());
    
    // Pokaż następny widget
    nextWidget->show();
    nextWidget->raise();

    // Efekty przezroczystości
    QGraphicsOpacityEffect *currentEffect = new QGraphicsOpacityEffect(currentWidget);
    currentWidget->setGraphicsEffect(currentEffect);
    QGraphicsOpacityEffect *nextEffect = new QGraphicsOpacityEffect(nextWidget);
    nextWidget->setGraphicsEffect(nextEffect);
    nextEffect->setOpacity(0.3);

    // Animacja przesunięcia i zanikania bieżącego widgetu
    QPropertyAnimation *currentMoveAnimation = new QPropertyAnimation(currentWidget, "geometry");
    currentMoveAnimation->setDuration(m_duration);
    currentMoveAnimation->setStartValue(QRect(0, 0, width, height()));
    currentMoveAnimation->setEndValue(QRect(-width, 0, width, height()));
    currentMoveAnimation->setEasingCurve(QEasingCurve::OutCubic);

    QPropertyAnimation *currentFadeAnimation = new QPropertyAnimation(currentEffect, "opacity");
    currentFadeAnimation->setDuration(m_duration);
    currentFadeAnimation->setStartValue(1.0);
    currentFadeAnimation->setEndValue(0.0);
    currentFadeAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja przesunięcia i pojawiania się następnego widgetu
    QPropertyAnimation *nextMoveAnimation = new QPropertyAnimation(nextWidget, "geometry");
    nextMoveAnimation->setDuration(m_duration);
    nextMoveAnimation->setStartValue(QRect(width, 0, width, height()));
    nextMoveAnimation->setEndValue(QRect(0, 0, width, height()));
    nextMoveAnimation->setEasingCurve(QEasingCurve::OutCubic);

    QPropertyAnimation *nextFadeAnimation = new QPropertyAnimation(nextEffect, "opacity");
    nextFadeAnimation->setDuration(m_duration);
    nextFadeAnimation->setStartValue(0.3);
    nextFadeAnimation->setEndValue(1.0);
    nextFadeAnimation->setEasingCurve(QEasingCurve::InCubic);

    m_animationGroup->clear();
    m_animationGroup->addAnimation(currentMoveAnimation);
    m_animationGroup->addAnimation(currentFadeAnimation);
    m_animationGroup->addAnimation(nextMoveAnimation);
    m_animationGroup->addAnimation(nextFadeAnimation);
}

void AnimatedStackedWidget::animatePush(int nextIndex)
{
    QWidget *currentWidget = widget(currentIndex());
    QWidget *nextWidget = widget(nextIndex);

    // Pozycje początkowe
    int width = currentWidget->width();
    nextWidget->setGeometry(width, 0, width, height());
    
    // Pokaż następny widget
    nextWidget->show();
    nextWidget->raise();

    // Animacja przesunięcia bieżącego widgetu
    QPropertyAnimation *currentAnimation = new QPropertyAnimation(currentWidget, "geometry");
    currentAnimation->setDuration(m_duration);
    currentAnimation->setStartValue(QRect(0, 0, width, height()));
    currentAnimation->setEndValue(QRect(-width/2, 0, width, height()));
    currentAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Efekt przezroczystości dla bieżącego widgetu
    QGraphicsOpacityEffect *currentEffect = new QGraphicsOpacityEffect(currentWidget);
    currentWidget->setGraphicsEffect(currentEffect);
    currentEffect->setOpacity(1.0);

    QPropertyAnimation *fadeOutAnimation = new QPropertyAnimation(currentEffect, "opacity");
    fadeOutAnimation->setDuration(m_duration);
    fadeOutAnimation->setStartValue(1.0);
    fadeOutAnimation->setEndValue(0.0);
    fadeOutAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja przesunięcia następnego widgetu
    QPropertyAnimation *nextAnimation = new QPropertyAnimation(nextWidget, "geometry");
    nextAnimation->setDuration(m_duration);
    nextAnimation->setStartValue(QRect(width, 0, width, height()));
    nextAnimation->setEndValue(QRect(0, 0, width, height()));
    nextAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_animationGroup->clear();
    m_animationGroup->addAnimation(currentAnimation);
    m_animationGroup->addAnimation(fadeOutAnimation);
    m_animationGroup->addAnimation(nextAnimation);
}