#include "animated_stacked_widget.h"

#include <QCoreApplication>
#include <QGraphicsOpacityEffect>

AnimatedStackedWidget::AnimatedStackedWidget(QWidget *parent)
    : QStackedWidget(parent),
      duration_(500),
      animation_type_(Fade),
      animation_group_(new QParallelAnimationGroup(this)),
      animation_running_(false),
      target_index_(-1),
      swoosh_sound_(new QSoundEffect(this)) // Zainicjuj QSoundEffect
{
    // Ustaw źródło dźwięku (zakładając, że jest w zasobach Qt)
    swoosh_sound_->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/swoosh1.wav"));
    // Opcjonalnie ustaw głośność (0.0 do 1.0)
    swoosh_sound_->setVolume(0.5);


    connect(animation_group_, &QParallelAnimationGroup::finished, [this]() {
        animation_running_ = false;

        // Ustaw właściwy widget docelowy
        if (target_index_ >= 0 && target_index_ < count()) {
            setCurrentIndex(target_index_);  // To wyemituje currentChanged
        }

        // Czyszczenie efektów graficznych po animacji
        CleanupAfterAnimation();

        // Zresetuj docelowy indeks
        target_index_ = -1;
    });

    gl_transition_widget_ = new GLTransitionWidget(this);
    gl_transition_widget_->hide();
    connect(gl_transition_widget_, &GLTransitionWidget::transitionFinished,
            this, &AnimatedStackedWidget::OnGLTransitionFinished);
}

AnimatedStackedWidget::~AnimatedStackedWidget()
{
    delete animation_group_;
    delete gl_transition_widget_;
}

void AnimatedStackedWidget::SlideToIndex(const int index)
{
    // Sprawdź, czy animacja już trwa lub czy indeks jest nieprawidłowy
    if (animation_running_ || index == currentIndex() || index < 0 || index >= count()) {
        return;
    }

    // Zatrzymaj poprzednią animację, jeśli istnieje
    if (animation_group_->state() == QAbstractAnimation::Running) {
        animation_group_->stop();
        CleanupAfterAnimation();
    }

    if (swoosh_sound_->isLoaded()) {
        swoosh_sound_->play();
    } else {
        qWarning("Nie można odtworzyć dźwięku swoosh - nie załadowano.");
    }


    animation_running_ = true;
    target_index_ = index;

    // Przygotuj i uruchom animację
    PrepareAnimation(index);
}

void AnimatedStackedWidget::SlideToWidget(QWidget *widget)
{
    const int index = indexOf(widget);
    if (index != -1) {
        SlideToIndex(index);
    }
}

void AnimatedStackedWidget::SlideToNextIndex()
{
    // Metoda do automatycznego przesuwania w karuzeli
    int next_index = currentIndex() + 1;
    if (next_index >= count()) {
        next_index = 0;
    }
    SlideToIndex(next_index);
}

void AnimatedStackedWidget::PrepareAnimation(const int next_index) const {
    // Przygotowanie animacji w zależności od wybranego typu
    switch (animation_type_) {
    case Fade:
        AnimateFade(next_index);
        break;
    case Slide:
        if (gl_transition_widget_) {
            // Pobieramy widgety
            QWidget *current_widget = widget(currentIndex());
            QWidget *next_widget = widget(next_index);

            // Upewnij się, że oba widgety mają prawidłową wielkość
            current_widget->resize(size());
            next_widget->resize(size());

            // Ustaw tekstury dla widgetu OpenGL
            gl_transition_widget_->resize(size());
            gl_transition_widget_->SetWidgets(current_widget, next_widget);

            // Pokaż OpenGL widget
            gl_transition_widget_->show();
            gl_transition_widget_->raise();

            // Ukryj oryginalne widgety podczas animacji
            current_widget->hide();
            next_widget->hide();

            // Rozpocznij animację
            gl_transition_widget_->StartTransition(duration_);
            return;
        }
        // Fallback do standardowej animacji
        AnimateSlide(next_index);
        break;
    case SlideAndFade:
        AnimateSlideAndFade(next_index);
        break;
    case Push:
        AnimatePush(next_index);
        break;
    }

    // Uruchom animację
    animation_group_->start();
}

void AnimatedStackedWidget::CleanupAfterAnimation() const {
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

void AnimatedStackedWidget::AnimateFade(const int next_index) const {
    QWidget *current_widget = widget(currentIndex());
    QWidget *next_widget = widget(next_index);

    // Ustaw następny widget na wierzchu
    next_widget->setVisible(true);
    next_widget->raise();
    current_widget->raise();

    // Efekt przezroczystości dla bieżącego widgetu
    const auto current_effect = new QGraphicsOpacityEffect(current_widget);
    current_widget->setGraphicsEffect(current_effect);

    // Efekt przezroczystości dla następnego widgetu
    const auto next_effect = new QGraphicsOpacityEffect(next_widget);
    next_widget->setGraphicsEffect(next_effect);
    next_effect->setOpacity(0);

    // Animacja zanikania bieżącego widgetu
    const auto fade_out_animation = new QPropertyAnimation(current_effect, "opacity");
    fade_out_animation->setDuration(duration_);
    fade_out_animation->setStartValue(1.0);
    fade_out_animation->setEndValue(0.0);
    fade_out_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja pojawiania się następnego widgetu
    const auto fade_in_animation = new QPropertyAnimation(next_effect, "opacity");
    fade_in_animation->setDuration(duration_);
    fade_in_animation->setStartValue(0.0);
    fade_in_animation->setEndValue(1.0);
    fade_in_animation->setEasingCurve(QEasingCurve::InCubic);

    animation_group_->clear();
    animation_group_->addAnimation(fade_out_animation);
    animation_group_->addAnimation(fade_in_animation);
}

void AnimatedStackedWidget::AnimateSlide(const int next_index) const {
    QWidget *current_widget = widget(currentIndex());
    QWidget *next_widget = widget(next_index);

    // Pozycje początkowe
    const int width = current_widget->width();
    next_widget->setGeometry(width, 0, width, height());
    
    // Pokaż następny widget
    next_widget->show();
    next_widget->raise();

    // Animacja przesunięcia bieżącego widgetu
    const auto current_animation = new QPropertyAnimation(current_widget, "geometry");
    current_animation->setDuration(duration_);
    current_animation->setStartValue(QRect(0, 0, width, height()));
    current_animation->setEndValue(QRect(-width, 0, width, height()));
    current_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja przesunięcia następnego widgetu
    const auto next_animation = new QPropertyAnimation(next_widget, "geometry");
    next_animation->setDuration(duration_);
    next_animation->setStartValue(QRect(width, 0, width, height()));
    next_animation->setEndValue(QRect(0, 0, width, height()));
    next_animation->setEasingCurve(QEasingCurve::OutCubic);

    animation_group_->clear();
    animation_group_->addAnimation(current_animation);
    animation_group_->addAnimation(next_animation);
}

void AnimatedStackedWidget::AnimateSlideAndFade(const int next_index) const {
    QWidget *current_widget = widget(currentIndex());
    QWidget *next_widget = widget(next_index);

    // Pozycje początkowe
    const int width = current_widget->width();
    next_widget->setGeometry(width, 0, width, height());
    
    // Pokaż następny widget
    next_widget->show();
    next_widget->raise();

    // Efekty przezroczystości
    const auto current_effect = new QGraphicsOpacityEffect(current_widget);
    current_widget->setGraphicsEffect(current_effect);
    const auto next_effect = new QGraphicsOpacityEffect(next_widget);
    next_widget->setGraphicsEffect(next_effect);
    next_effect->setOpacity(0.3);

    // Animacja przesunięcia i zanikania bieżącego widgetu
    const auto current_move_animation = new QPropertyAnimation(current_widget, "geometry");
    current_move_animation->setDuration(duration_);
    current_move_animation->setStartValue(QRect(0, 0, width, height()));
    current_move_animation->setEndValue(QRect(-width, 0, width, height()));
    current_move_animation->setEasingCurve(QEasingCurve::OutCubic);

    const auto current_fade_animation = new QPropertyAnimation(current_effect, "opacity");
    current_fade_animation->setDuration(duration_);
    current_fade_animation->setStartValue(1.0);
    current_fade_animation->setEndValue(0.0);
    current_fade_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja przesunięcia i pojawiania się następnego widgetu
    const auto next_move_animation = new QPropertyAnimation(next_widget, "geometry");
    next_move_animation->setDuration(duration_);
    next_move_animation->setStartValue(QRect(width, 0, width, height()));
    next_move_animation->setEndValue(QRect(0, 0, width, height()));
    next_move_animation->setEasingCurve(QEasingCurve::OutCubic);

    const auto next_fade_animation = new QPropertyAnimation(next_effect, "opacity");
    next_fade_animation->setDuration(duration_);
    next_fade_animation->setStartValue(0.3);
    next_fade_animation->setEndValue(1.0);
    next_fade_animation->setEasingCurve(QEasingCurve::InCubic);

    animation_group_->clear();
    animation_group_->addAnimation(current_move_animation);
    animation_group_->addAnimation(current_fade_animation);
    animation_group_->addAnimation(next_move_animation);
    animation_group_->addAnimation(next_fade_animation);
}

void AnimatedStackedWidget::AnimatePush(const int next_index) const {
    QWidget *current_widget = widget(currentIndex());
    QWidget *next_widget = widget(next_index);

    // Pozycje początkowe
    const int width = current_widget->width();
    next_widget->setGeometry(width, 0, width, height());
    
    // Pokaż następny widget
    next_widget->show();
    next_widget->raise();

    // Animacja przesunięcia bieżącego widgetu
    const auto current_animation = new QPropertyAnimation(current_widget, "geometry");
    current_animation->setDuration(duration_);
    current_animation->setStartValue(QRect(0, 0, width, height()));
    current_animation->setEndValue(QRect(-width/2, 0, width, height()));
    current_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Efekt przezroczystości dla bieżącego widgetu
    const auto current_effect = new QGraphicsOpacityEffect(current_widget);
    current_widget->setGraphicsEffect(current_effect);
    current_effect->setOpacity(1.0);

    const auto fade_out_animation = new QPropertyAnimation(current_effect, "opacity");
    fade_out_animation->setDuration(duration_);
    fade_out_animation->setStartValue(1.0);
    fade_out_animation->setEndValue(0.0);
    fade_out_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Animacja przesunięcia następnego widgetu
    const auto next_animation = new QPropertyAnimation(next_widget, "geometry");
    next_animation->setDuration(duration_);
    next_animation->setStartValue(QRect(width, 0, width, height()));
    next_animation->setEndValue(QRect(0, 0, width, height()));
    next_animation->setEasingCurve(QEasingCurve::OutCubic);

    animation_group_->clear();
    animation_group_->addAnimation(current_animation);
    animation_group_->addAnimation(fade_out_animation);
    animation_group_->addAnimation(next_animation);
}

void AnimatedStackedWidget::OnGLTransitionFinished()
{
    // Ukryj widget OpenGL
    gl_transition_widget_->hide();

    // Aktualizuj bieżący indeks
    setCurrentIndex(target_index_);

    // Pokaż właściwy widget
    widget(target_index_)->show();

    // Resetuj flagi
    animation_running_ = false;
    target_index_ = -1;

    // Emituj sygnał zakończenia animacji
    emit currentChanged(currentIndex());
}