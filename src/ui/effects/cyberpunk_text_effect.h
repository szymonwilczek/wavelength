#ifndef CYBERPUNK_TEXT_EFFECT_H
#define CYBERPUNK_TEXT_EFFECT_H
#include <QLabel>
#include <QObject>
#include <QRandomGenerator>

class CyberpunkTextEffect : public QObject {
public:
    explicit CyberpunkTextEffect(QLabel *label, QWidget *parent = nullptr);

    void startAnimation();

private slots:
    void nextAnimationStep();

private:
    void animateScanning();

    void animateTyping();

    void animateGlitching();

    QLabel *m_label;
    QString m_originalText;
    QTimer *m_timer;
    int m_phase = 0;
    int m_charIndex = 0;
    int m_glitchCount = 0;
};

#endif //CYBERPUNK_TEXT_EFFECT_H
