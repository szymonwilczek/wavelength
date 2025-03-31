#ifndef CYBERPUNK_TEXT_EFFECT_H
#define CYBERPUNK_TEXT_EFFECT_H
#include <QLabel>
#include <QObject>
#include <QRandomGenerator>
#include <QTimer>

class CyberpunkTextEffect : public QObject {
public:
    CyberpunkTextEffect(QLabel *label, QWidget *parent = nullptr)
        : QObject(parent), m_label(label), m_originalText(label->text()) {
        // Ukryj etykietę na początku
        m_label->setText("");
        m_label->show();

        // Utwórz timer dla efektów animacji
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &CyberpunkTextEffect::nextAnimationStep);
    }

    void startAnimation() {
        m_phase = 0;
        m_charIndex = 0;
        m_glitchCount = 0;
        m_timer->start(50); // 50ms pomiędzy krokami animacji
    }

private slots:
    void nextAnimationStep() {
        switch (m_phase) {
            case 0: // Faza skanowania - pionowa linia przesuwająca się
                animateScanning();
                break;
            case 1: // Faza wypełniania tekstu
                animateTyping();
                break;
            case 2: // Faza glitchowania ostateczna
                animateGlitching();
                break;
            case 3: // Koniec animacji
                m_timer->stop();
                m_label->setText(m_originalText);
                break;
        }
    }

private:
    void animateScanning() {
        // Symulacja skanowania - pionowa linia
        if (m_charIndex < m_originalText.length() * 2) {
            int scanPos = m_charIndex / 2;

            // Przygotuj tekst ze spacjami i znakiem |
            QString scanText;
            for (int i = 0; i < m_originalText.length(); i++) {
                if (i == scanPos) {
                    scanText += "|";
                } else {
                    scanText += " ";
                }
            }

            m_label->setText(scanText);
            m_charIndex++;
        } else {
            m_phase = 1;
            m_charIndex = 0;
            m_label->setText("");
        }
    }

    void animateTyping() {
        // Pisanie tekstu znak po znaku z glitchem
        if (m_charIndex < m_originalText.length()) {
            QString displayText = m_originalText.left(m_charIndex + 1);

            // Dodaj losowy glitch
            if (QRandomGenerator::global()->bounded(100) < 30) {
                QString glitchChar = QString(QChar(QRandomGenerator::global()->bounded(33, 126)));
                displayText += glitchChar;
            }

            m_label->setText(displayText);
            m_charIndex++;
        } else {
            m_phase = 2;
            m_glitchCount = 0;
        }
    }

    void animateGlitching() {
        // Końcowe glitche
        if (m_glitchCount < 5) {
            if (m_glitchCount % 2 == 0) {
                // Dodaj losowe znaki glitchujące
                QString glitchText = m_originalText;
                for (int i = 0; i < 3; i++) {
                    int pos = QRandomGenerator::global()->bounded(glitchText.length());
                    glitchText.insert(pos, QString(QChar(QRandomGenerator::global()->bounded(33, 126))));
                }
                m_label->setText(glitchText);
            } else {
                m_label->setText(m_originalText);
            }
            m_glitchCount++;
        } else {
            m_phase = 3;
        }
    }

    QLabel *m_label;
    QString m_originalText;
    QTimer *m_timer;
    int m_phase = 0;
    int m_charIndex = 0;
    int m_glitchCount = 0;
};

#endif //CYBERPUNK_TEXT_EFFECT_H
