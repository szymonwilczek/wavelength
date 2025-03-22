#ifndef ANIMATED_DIALOG_H
#define ANIMATED_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QShowEvent>
#include <QCloseEvent>

// Klasa dla przyciemnienia tła
class OverlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit OverlayWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

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

    void setAnimationDuration(int duration) { m_duration = duration; }
    int animationDuration() const { return m_duration; }

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void animateShow();
    void animateClose();

    AnimationType m_animationType;
    int m_duration;
    bool m_closing;
    OverlayWidget *m_overlay;
};

#endif // ANIMATED_DIALOG_H