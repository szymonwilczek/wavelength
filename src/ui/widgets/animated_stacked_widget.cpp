#include "animated_stacked_widget.h"

#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSoundEffect>

#include "gl_transition_widget.h"

AnimatedStackedWidget::AnimatedStackedWidget(QWidget *parent)
    : QStackedWidget(parent),
      duration_(500),
      animation_type_(Fade),
      animation_group_(new QParallelAnimationGroup(this)),
      animation_running_(false),
      target_index_(-1),
      swoosh_sound_(new QSoundEffect(this)) {
    swoosh_sound_->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/swoosh1.wav"));
    swoosh_sound_->setVolume(0.5);

    connect(animation_group_, &QParallelAnimationGroup::finished, [this] {
        animation_running_ = false;

        if (target_index_ >= 0 && target_index_ < count()) {
            setCurrentIndex(target_index_);
        }

        CleanupAfterAnimation();
        target_index_ = -1;
    });

    gl_transition_widget_ = new GLTransitionWidget(this);
    gl_transition_widget_->hide();
    connect(gl_transition_widget_, &GLTransitionWidget::transitionFinished,
            this, &AnimatedStackedWidget::OnGLTransitionFinished);
}

AnimatedStackedWidget::~AnimatedStackedWidget() {
    delete animation_group_;
    delete gl_transition_widget_;
}

void AnimatedStackedWidget::SlideToIndex(const int index) {
    if (animation_running_ || index == currentIndex() || index < 0 || index >= count()) {
        return;
    }

    if (animation_group_->state() == QAbstractAnimation::Running) {
        animation_group_->stop();
        CleanupAfterAnimation();
    }

    if (swoosh_sound_->isLoaded()) {
        swoosh_sound_->play();
    } else {
        qWarning("[ANIMATED STACK WIDGET] Unable to play swoosh sound - not loaded.");
    }

    animation_running_ = true;
    target_index_ = index;

    PrepareAnimation(index);
}

void AnimatedStackedWidget::SlideToWidget(QWidget *widget) {
    const int index = indexOf(widget);
    if (index != -1) {
        SlideToIndex(index);
    }
}

void AnimatedStackedWidget::SlideToNextIndex() {
    int next_index = currentIndex() + 1;
    if (next_index >= count()) {
        next_index = 0;
    }
    SlideToIndex(next_index);
}

void AnimatedStackedWidget::PrepareAnimation(const int next_index) const {
    switch (animation_type_) {
        case Fade:
            AnimateFade(next_index);
            break;
        case Slide:
            if (gl_transition_widget_) {
                QWidget *current_widget = widget(currentIndex());
                QWidget *next_widget = widget(next_index);

                current_widget->resize(size());
                next_widget->resize(size());

                gl_transition_widget_->resize(size());
                gl_transition_widget_->SetWidgets(current_widget, next_widget);

                gl_transition_widget_->show();
                gl_transition_widget_->raise();

                current_widget->hide();
                next_widget->hide();

                gl_transition_widget_->StartTransition(duration_);
                return;
            }
            // fallback to standard animation
            AnimateSlide(next_index);
            break;
        case SlideAndFade:
            AnimateSlideAndFade(next_index);
            break;
        case Push:
            AnimatePush(next_index);
            break;
    }

    animation_group_->start();
}

void AnimatedStackedWidget::CleanupAfterAnimation() const {
    for (int i = 0; i < count(); ++i) {
        if (widget(i)->graphicsEffect()) {
            widget(i)->setGraphicsEffect(nullptr);
        }

        if (i != currentIndex()) {
            widget(i)->setGeometry(0, 0, width(), height());
        }
    }
}

void AnimatedStackedWidget::AnimateFade(const int next_index) const {
    QWidget *current_widget = widget(currentIndex());
    QWidget *next_widget = widget(next_index);

    next_widget->setVisible(true);
    next_widget->raise();
    current_widget->raise();

    const auto current_effect = new QGraphicsOpacityEffect(current_widget);
    current_widget->setGraphicsEffect(current_effect);

    const auto next_effect = new QGraphicsOpacityEffect(next_widget);
    next_widget->setGraphicsEffect(next_effect);
    next_effect->setOpacity(0);

    const auto fade_out_animation = new QPropertyAnimation(current_effect, "opacity");
    fade_out_animation->setDuration(duration_);
    fade_out_animation->setStartValue(1.0);
    fade_out_animation->setEndValue(0.0);
    fade_out_animation->setEasingCurve(QEasingCurve::OutCubic);

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

    const int width = current_widget->width();
    next_widget->setGeometry(width, 0, width, height());

    next_widget->show();
    next_widget->raise();

    const auto current_animation = new QPropertyAnimation(current_widget, "geometry");
    current_animation->setDuration(duration_);
    current_animation->setStartValue(QRect(0, 0, width, height()));
    current_animation->setEndValue(QRect(-width, 0, width, height()));
    current_animation->setEasingCurve(QEasingCurve::OutCubic);

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

    const int width = current_widget->width();
    next_widget->setGeometry(width, 0, width, height());

    next_widget->show();
    next_widget->raise();

    const auto current_effect = new QGraphicsOpacityEffect(current_widget);
    current_widget->setGraphicsEffect(current_effect);
    const auto next_effect = new QGraphicsOpacityEffect(next_widget);
    next_widget->setGraphicsEffect(next_effect);
    next_effect->setOpacity(0.3);

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

    const int width = current_widget->width();
    next_widget->setGeometry(width, 0, width, height());

    next_widget->show();
    next_widget->raise();

    const auto current_animation = new QPropertyAnimation(current_widget, "geometry");
    current_animation->setDuration(duration_);
    current_animation->setStartValue(QRect(0, 0, width, height()));
    current_animation->setEndValue(QRect(-width / 2, 0, width, height()));
    current_animation->setEasingCurve(QEasingCurve::OutCubic);

    const auto current_effect = new QGraphicsOpacityEffect(current_widget);
    current_widget->setGraphicsEffect(current_effect);
    current_effect->setOpacity(1.0);

    const auto fade_out_animation = new QPropertyAnimation(current_effect, "opacity");
    fade_out_animation->setDuration(duration_);
    fade_out_animation->setStartValue(1.0);
    fade_out_animation->setEndValue(0.0);
    fade_out_animation->setEasingCurve(QEasingCurve::OutCubic);

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

void AnimatedStackedWidget::OnGLTransitionFinished() {
    gl_transition_widget_->hide();
    setCurrentIndex(target_index_);
    widget(target_index_)->show();
    animation_running_ = false;
    target_index_ = -1;

    emit currentChanged(currentIndex());
}
