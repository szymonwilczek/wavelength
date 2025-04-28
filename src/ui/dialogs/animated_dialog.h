#ifndef ANIMATED_DIALOG_H
#define ANIMATED_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QCloseEvent>
#include <QRandomGenerator>


class OverlayWidget;

class AnimatedDialog : public QDialog {
    Q_OBJECT

public:
    enum AnimationType {
        SlideFromBottom,
        FadeIn,
        ScaleFromCenter,
        DigitalMaterialization  // Nowy typ animacji
    };

    explicit AnimatedDialog(QWidget *parent = nullptr, AnimationType type = SlideFromBottom);
    ~AnimatedDialog() override;

    void setAnimationDuration(const int duration) { m_duration = duration; }
    int animationDuration() const { return m_duration; }

    signals:
        void showAnimationFinished(); // Nowy sygnał informujący o zakończeniu animacji



protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    double m_digitalizationProgress = 0.0;
    double m_cornerGlowProgress = 0.0;

private:
    void animateShow();
    void animateClose();

    AnimationType m_animationType;
    int m_duration;
    bool m_closing;
    OverlayWidget *m_overlay;

    QPixmap m_scanlineBuffer;
    QLinearGradient m_scanGradient;
    bool m_scanlineInitialized = false;


};

#endif // ANIMATED_DIALOG_H