#include "animated_stacked_widget.h"

#include <QCoreApplication>
#include <QGraphicsOpacityEffect>

AnimatedStackedWidget::AnimatedStackedWidget(QWidget *parent)
    : QStackedWidget(parent),
      m_duration(500),
      m_animationType(Fade),
      m_animationGroup(new QParallelAnimationGroup(this)),
      m_animationRunning(false),
      m_targetIndex(-1),
      m_swooshSound(new QSoundEffect(this)) // Zainicjuj QSoundEffect
{
    // Ustaw źródło dźwięku (zakładając, że jest w zasobach Qt)
    m_swooshSound->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/swoosh1.wav"));
    // Opcjonalnie ustaw głośność (0.0 do 1.0)
    m_swooshSound->setVolume(0.5);


    connect(m_animationGroup, &QParallelAnimationGroup::finished, [this]() {
        m_animationRunning = false;

        // Ustaw właściwy widget docelowy
        if (m_targetIndex >= 0 && m_targetIndex < count()) {
            setCurrentIndex(m_targetIndex);  // To wyemituje currentChanged
        }

        // Czyszczenie efektów graficznych po animacji
        cleanupAfterAnimation();

        // Zresetuj docelowy indeks
        m_targetIndex = -1;
    });

    m_glTransitionWidget = new GLTransitionWidget(this);
    m_glTransitionWidget->hide();
    connect(m_glTransitionWidget, &GLTransitionWidget::transitionFinished,
            this, &AnimatedStackedWidget::onGLTransitionFinished);
}

AnimatedStackedWidget::~AnimatedStackedWidget()
{
    delete m_animationGroup;
    delete m_glTransitionWidget;
}

void AnimatedStackedWidget::slideToIndex(int index)
{
    // Sprawdź, czy animacja już trwa lub czy indeks jest nieprawidłowy
    if (m_animationRunning || index == currentIndex() || index < 0 || index >= count()) {
        return;
    }

    // Zatrzymaj poprzednią animację, jeśli istnieje
    if (m_animationGroup->state() == QAbstractAnimation::Running) {
        m_animationGroup->stop();
        cleanupAfterAnimation();
    }

    if (m_swooshSound->isLoaded()) {
        m_swooshSound->play();
    } else {
        qWarning("Nie można odtworzyć dźwięku swoosh - nie załadowano.");
    }


    m_animationRunning = true;
    m_targetIndex = index;

    // Przygotuj i uruchom animację
    prepareAnimation(index);
}

void AnimatedStackedWidget::slideToWidget(QWidget *widget)
{
    int index = indexOf(widget);
    if (index != -1) {
        slideToIndex(index);
    }
}

void AnimatedStackedWidget::slideToNextIndex()
{
    // Metoda do automatycznego przesuwania w karuzeli
    int nextIndex = currentIndex() + 1;
    if (nextIndex >= count()) {
        nextIndex = 0;
    }
    slideToIndex(nextIndex);
}

void AnimatedStackedWidget::prepareAnimation(int nextIndex)
{
    // Przygotowanie animacji w zależności od wybranego typu
    switch (m_animationType) {
    case Fade:
        animateFade(nextIndex);
        break;
    case Slide:
        if (m_glTransitionWidget) {
            // Pobieramy widgety
            QWidget *currentWidget = widget(currentIndex());
            QWidget *nextWidget = widget(nextIndex);

            // Upewnij się, że oba widgety mają prawidłową wielkość
            currentWidget->resize(size());
            nextWidget->resize(size());

            // Ustaw tekstury dla widgetu OpenGL
            m_glTransitionWidget->resize(size());
            m_glTransitionWidget->setWidgets(currentWidget, nextWidget);

            // Pokaż OpenGL widget
            m_glTransitionWidget->show();
            m_glTransitionWidget->raise();

            // Ukryj oryginalne widgety podczas animacji
            currentWidget->hide();
            nextWidget->hide();

            // Rozpocznij animację
            m_glTransitionWidget->startTransition(m_duration);
            return;
        }
        // Fallback do standardowej animacji
        animateSlide(nextIndex);
        break;
    case SlideAndFade:
        animateSlideAndFade(nextIndex);
        break;
    case Push:
        animatePush(nextIndex);
        break;
    }

    // Uruchom animację
    m_animationGroup->start();
}

void AnimatedStackedWidget::cleanupAfterAnimation()
{
    // Wyczyść efekty graficzne dla wszystkich widgetów
    for (int i = 0; i < count(); ++i) {
        if (widget(i)->graphicsEffect()) {
            widget(i)->setGraphicsEffect(nullptr);
        }

        // Przywróć pozycje widgetów
        if (i != currentIndex()) {
            widget(i)->setGeometry(0, 0, width(), height());
        }
    }
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

void AnimatedStackedWidget::onGLTransitionFinished()
{
    // Ukryj widget OpenGL
    m_glTransitionWidget->hide();

    // Aktualizuj bieżący indeks
    setCurrentIndex(m_targetIndex);

    // Pokaż właściwy widget
    widget(m_targetIndex)->show();

    // Resetuj flagi
    m_animationRunning = false;
    m_targetIndex = -1;

    // Emituj sygnał zakończenia animacji
    emit currentChanged(currentIndex());
}