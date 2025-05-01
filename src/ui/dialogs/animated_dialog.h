#ifndef ANIMATED_DIALOG_H
#define ANIMATED_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QCloseEvent>

class OverlayWidget;

class AnimatedDialog : public QDialog {
    Q_OBJECT

public:
    enum AnimationType {
        kSlideFromBottom,
        kFadeIn,
        kScaleFromCenter,
        kDigitalMaterialization
    };

    explicit AnimatedDialog(QWidget *parent = nullptr, AnimationType type = kSlideFromBottom);
    ~AnimatedDialog() override;

    void SetAnimationDuration(const int duration) { duration_ = duration; }
    int GetAnimationDuration() const { return duration_; }

    signals:
        void showAnimationFinished();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    double digitalization_progress_ = 0.0;
    double corner_glow_progress_ = 0.0;

private:
    void AnimateShow();
    void AnimateClose();

    AnimationType animation_type_;
    int duration_;
    bool closing_;
    OverlayWidget *overlay_;

    QPixmap scanline_buffer_;
    QLinearGradient scan_gradient_;
    bool scanline_initialized_ = false;


};

#endif // ANIMATED_DIALOG_H