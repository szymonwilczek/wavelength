#ifndef ANIMATED_STACKED_WIDGET_H
#define ANIMATED_STACKED_WIDGET_H

#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>

class AnimatedStackedWidget : public QStackedWidget
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
    ~AnimatedStackedWidget();

    // Ustawienia
    void setDuration(int duration) { m_duration = duration; }
    int duration() const { return m_duration; }
    void setAnimationType(AnimationType type) { m_animationType = type; }
    AnimationType animationType() const { return m_animationType; }
    bool isAnimating() const { return m_animationRunning; }

    public slots:
        void slideToIndex(int index);
    void slideToWidget(QWidget *widget);
    void slideToNextIndex();  // Nowa metoda dla karuzelowego przewijania

protected:
    void animateFade(int nextIndex);
    void animateSlide(int nextIndex);
    void animateSlideAndFade(int nextIndex);
    void animatePush(int nextIndex);

private:
    void prepareAnimation(int nextIndex);
    void cleanupAfterAnimation();

    int m_duration;
    AnimationType m_animationType;
    QParallelAnimationGroup *m_animationGroup;
    bool m_animationRunning;
    int m_targetIndex;  // Nowe pole dla przechowywanego indeksu docelowego
};

#endif // ANIMATED_STACKED_WIDGET_H