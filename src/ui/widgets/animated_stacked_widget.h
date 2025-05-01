#ifndef ANIMATED_STACKED_WIDGET_H
#define ANIMATED_STACKED_WIDGET_H

#include <QStackedWidget>
#include <QParallelAnimationGroup>
#include <QSoundEffect>

#include "gl_transition_widget.h"

class AnimatedStackedWidget final : public QStackedWidget
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration)

public:
    enum AnimationType {
        Fade,
        Slide,
        SlideAndFade,
        Push
    };

    explicit AnimatedStackedWidget(QWidget *parent = nullptr);
    ~AnimatedStackedWidget() override;

    // Ustawienia
    void SetDuration(const int duration) { duration_ = duration; }
    int GetDuration() const { return duration_; }
    void SetAnimationType(const AnimationType type) { animation_type_ = type; }
    AnimationType GetAnimationType() const { return animation_type_; }
    bool IsAnimating() const { return animation_running_; }

public slots:
    void SlideToIndex(int index);
    void SlideToWidget(QWidget *widget);
    void SlideToNextIndex();  // Nowa metoda dla karuzelowego przewijania

protected:
    void AnimateFade(int next_index) const;
    void AnimateSlide(int next_index) const;
    void AnimateSlideAndFade(int next_index) const;
    void AnimatePush(int next_index) const;

private:
    void PrepareAnimation(int next_index) const;
    void CleanupAfterAnimation() const;
    void OnGLTransitionFinished();

    int duration_;
    AnimationType animation_type_;
    QParallelAnimationGroup *animation_group_;
    bool animation_running_;
    int target_index_;  // Nowe pole dla przechowywanego indeksu docelowego
    GLTransitionWidget *gl_transition_widget_ = nullptr;
    QSoundEffect *swoosh_sound_; // Dodaj pole dla efektu dźwiękowego
};

#endif // ANIMATED_STACKED_WIDGET_H