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
    void setDuration(const int duration) { m_duration = duration; }
    int duration() const { return m_duration; }
    void setAnimationType(const AnimationType type) { m_animationType = type; }
    AnimationType animationType() const { return m_animationType; }
    bool isAnimating() const { return m_animationRunning; }

public slots:
    void slideToIndex(int index);
    void slideToWidget(QWidget *widget);
    void slideToNextIndex();  // Nowa metoda dla karuzelowego przewijania

protected:
    void animateFade(int nextIndex) const;
    void animateSlide(int nextIndex) const;
    void animateSlideAndFade(int nextIndex) const;
    void animatePush(int nextIndex) const;

private:
    void prepareAnimation(int nextIndex) const;
    void cleanupAfterAnimation() const;
    void onGLTransitionFinished();

    int m_duration;
    AnimationType m_animationType;
    QParallelAnimationGroup *m_animationGroup;
    bool m_animationRunning;
    int m_targetIndex;  // Nowe pole dla przechowywanego indeksu docelowego
    GLTransitionWidget *m_glTransitionWidget = nullptr;
    QSoundEffect *m_swooshSound; // Dodaj pole dla efektu dźwiękowego
};

#endif // ANIMATED_STACKED_WIDGET_H