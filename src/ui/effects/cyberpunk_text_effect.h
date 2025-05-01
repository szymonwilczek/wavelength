#ifndef CYBERPUNK_TEXT_EFFECT_H
#define CYBERPUNK_TEXT_EFFECT_H
#include <QLabel>
#include <QObject>
#include <QRandomGenerator>

class CyberpunkTextEffect final : public QObject {
public:
    explicit CyberpunkTextEffect(QLabel *label, QWidget *parent = nullptr);

    void StartAnimation();

private slots:
    void NextAnimationStep();

private:
    void AnimateScanning();

    void AnimateTyping();

    void AnimateGlitching();

    QLabel *label_;
    QString original_text_;
    QTimer *timer_;
    int phase_ = 0;
    int char_index_ = 0;
    int glitch_count_ = 0;
};

#endif //CYBERPUNK_TEXT_EFFECT_H
