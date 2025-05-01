#include "cyberpunk_text_effect.h"

#include <QTimer>

CyberpunkTextEffect::CyberpunkTextEffect(QLabel *label, QWidget *parent): QObject(parent), label_(label), original_text_(label->text()) {
    // Ukryj etykietę na początku
    label_->setText("");
    label_->show();

    // Utwórz timer dla efektów animacji
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &CyberpunkTextEffect::NextAnimationStep);
}

void CyberpunkTextEffect::StartAnimation() {
    phase_ = 0;
    char_index_ = 0;
    glitch_count_ = 0;
    timer_->start(50); // 50ms pomiędzy krokami animacji
}

void CyberpunkTextEffect::AnimateScanning() {
    // Symulacja skanowania - pionowa linia
    if (char_index_ < original_text_.length() * 2) {
        const int scanPos = char_index_ / 2;

        // Przygotuj tekst ze spacjami i znakiem |
        QString scan_text;
        for (int i = 0; i < original_text_.length(); i++) {
            if (i == scanPos) {
                scan_text += "|";
            } else {
                scan_text += " ";
            }
        }

        label_->setText(scan_text);
        char_index_++;
    } else {
        phase_ = 1;
        char_index_ = 0;
        label_->setText("");
    }
}

void CyberpunkTextEffect::AnimateTyping() {
    // Pisanie tekstu znak po znaku z glitchem
    if (char_index_ < original_text_.length()) {
        QString display_text = original_text_.left(char_index_ + 1);

        // Dodaj losowy glitch
        if (QRandomGenerator::global()->bounded(100) < 30) {
            const auto glitchChar = QString(QChar(QRandomGenerator::global()->bounded(33, 126)));
            display_text += glitchChar;
        }

        label_->setText(display_text);
        char_index_++;
    } else {
        phase_ = 2;
        glitch_count_ = 0;
    }
}

void CyberpunkTextEffect::AnimateGlitching() {
    // Końcowe glitche
    if (glitch_count_ < 5) {
        if (glitch_count_ % 2 == 0) {
            // Dodaj losowe znaki glitchujące
            QString glitch_text = original_text_;
            for (int i = 0; i < 3; i++) {
                const int pos = QRandomGenerator::global()->bounded(glitch_text.length());
                glitch_text.insert(pos, QString(QChar(QRandomGenerator::global()->bounded(33, 126))));
            }
            label_->setText(glitch_text);
        } else {
            label_->setText(original_text_);
        }
        glitch_count_++;
    } else {
        phase_ = 3;
    }
}


void CyberpunkTextEffect::NextAnimationStep() {
    switch (phase_) {
        case 0: // Faza skanowania - pionowa linia przesuwająca się
            AnimateScanning();
            break;
        case 1: // Faza wypełniania tekstu
            AnimateTyping();
            break;
        case 2: // Faza glitchowania ostateczna
            AnimateGlitching();
            break;
        case 3: // Koniec animacji
            timer_->stop();
            label_->setText(original_text_);
            break;
    }
}

