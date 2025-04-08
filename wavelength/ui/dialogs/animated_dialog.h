#ifndef ANIMATED_DIALOG_H
#define ANIMATED_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QShowEvent>
#include <QCloseEvent>
#include <QPropertyAnimation>
#include <QRandomGenerator>

// Klasa dla przyciemnienia tła
class OverlayWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit OverlayWidget(QWidget *parent = nullptr);
    void updateGeometry(const QRect& rect);
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

private:
    QPixmap m_buffer;
    bool m_bufferDirty;
    qreal m_opacity;
    QRect m_excludeRect; // Obszar do wykluczenia (nawigacja)

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    // void resizeEvent(QResizeEvent *event) override;
};

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
    ~AnimatedDialog();

    void setAnimationDuration(int duration) { m_duration = duration; }
    int animationDuration() const { return m_duration; }

    signals:
        void showAnimationFinished(); // Nowy sygnał informujący o zakończeniu animacji



protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void initScanlineBuffer();
    double m_digitalizationProgress = 0.0;
    double m_cornerGlowProgress = 0.0;
    double m_glitchIntensity = 0.0;
    QList<int> m_glitchLines;

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