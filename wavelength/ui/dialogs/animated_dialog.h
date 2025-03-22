#ifndef ANIMATED_DIALOG_H
#define ANIMATED_DIALOG_H

#include <QDialog>
#include <QPropertyAnimation>
#include <QPoint>
#include <QShowEvent>
#include <QCloseEvent>

class AnimatedDialog : public QDialog {
    Q_OBJECT

public:
    enum AnimationType {
        SlideFromBottom,
        FadeIn,
        ScaleFromCenter
    };

    explicit AnimatedDialog(QWidget *parent = nullptr, AnimationType type = SlideFromBottom);
    ~AnimatedDialog();

    void setAnimationType(AnimationType type) { m_animationType = type; }
    void setAnimationDuration(int duration) { m_duration = duration; }

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void animateShow();
    void animateClose();
    
    AnimationType m_animationType;
    int m_duration;
    bool m_closing;
};

#endif // ANIMATED_DIALOG_H