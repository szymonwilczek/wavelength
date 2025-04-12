#include "typing_test_layer.h"
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFontMetrics>

TypingTestLayer::TypingTestLayer(QWidget *parent) 
    : SecurityLayer(parent),
      m_currentPosition(0),
      m_testStarted(false),
      m_testCompleted(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(15);

    m_titleLabel = new QLabel("TYPING VERIFICATION TEST", this);
    m_titleLabel->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_instructionsLabel = new QLabel("Przepisz poniższy tekst, aby kontynuować", this);
    m_instructionsLabel->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    m_instructionsLabel->setAlignment(Qt::AlignCenter);

    // Panel z tekstem do przepisania
    QWidget* textPanel = new QWidget(this);
    textPanel->setFixedSize(600, 120);
    textPanel->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");
    
    QVBoxLayout* textLayout = new QVBoxLayout(textPanel);
    textLayout->setContentsMargins(20, 20, 20, 20);
    textLayout->setAlignment(Qt::AlignVCenter);

    m_displayTextLabel = new QLabel(textPanel);
    m_displayTextLabel->setStyleSheet("color: #bbbbbb; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
    m_displayTextLabel->setWordWrap(true);
    textLayout->addWidget(m_displayTextLabel);

    // Ukryte pole do wprowadzania tekstu
    m_hiddenInput = new QLineEdit(this);
    m_hiddenInput->setFixedWidth(0);
    m_hiddenInput->setFixedHeight(0);
    m_hiddenInput->setStyleSheet("background-color: transparent; border: none; color: transparent;");
    
    // Połączenie sygnału zmiany tekstu
    connect(m_hiddenInput, &QLineEdit::textChanged, this, &TypingTestLayer::onTextChanged);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_instructionsLabel);
    layout->addWidget(textPanel);
    layout->addWidget(m_hiddenInput);
    layout->addStretch();

    // Lista możliwych słów do losowania
    QStringList wordPool = {
        "system", "dostęp", "terminal", "hasło", "protokół", 
        "sieć", "serwer", "dane", "baza", "moduł", 
        "kanał", "interfejs", "komputer", "monitor", "klawiatura", 
        "bezpieczeństwo", "weryfikacja", "program", "algorytm", "proces",
        "analiza", "skanowanie", "transfer", "połączenie", "aplikacja"
    };

    // Inicjalizacja generatora liczb losowych
    QRandomGenerator::securelySeeded();

    // Generowanie losowych słów
    m_words.clear();
    int wordCount = QRandomGenerator::global()->bounded(10, 16); // 10-15 słów
    
    for (int i = 0; i < wordCount; i++) {
        int index = QRandomGenerator::global()->bounded(wordPool.size());
        m_words.append(wordPool.at(index));
    }

    // Tworzenie pełnego tekstu
    m_fullText = m_words.join(" ");
    
    // Ustawienie wyświetlanego tekstu
    updateDisplayText();
}

TypingTestLayer::~TypingTestLayer() {
}

void TypingTestLayer::initialize() {
    reset();
    m_hiddenInput->setFocus();
}

void TypingTestLayer::reset() {
    m_currentPosition = 0;
    m_testStarted = false;
    m_testCompleted = false;
    m_hiddenInput->clear();
    updateDisplayText();
    
    // Reset stylów
    m_displayTextLabel->setStyleSheet("color: #bbbbbb; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
}

void TypingTestLayer::generateWords() {
    // Lista możliwych słów do losowania
    QStringList wordPool = {
        "system", "dostęp", "terminal", "hasło", "protokół", 
        "sieć", "serwer", "dane", "baza", "moduł", 
        "kanał", "interfejs", "komputer", "monitor", "klawiatura", 
        "bezpieczeństwo", "weryfikacja", "program", "algorytm", "proces",
        "analiza", "skanowanie", "transfer", "połączenie", "aplikacja"
    };

    // Generowanie losowych słów
    m_words.clear();
    int wordCount = QRandomGenerator::global()->bounded(10, 16); // 10-15 słów
    
    for (int i = 0; i < wordCount; i++) {
        int index = QRandomGenerator::global()->bounded(wordPool.size());
        m_words.append(wordPool.at(index));
    }

    // Tworzenie pełnego tekstu
    m_fullText = m_words.join(" ");
}

void TypingTestLayer::updateDisplayText() {
    if (m_fullText.isEmpty()) {
        return;
    }

    // Tworzenie rich text z wyróżnieniem
    QString richText;
    
    if (m_currentPosition > 0) {
        // Już wpisany tekst - zielony
        richText += "<span style='color: #33ff33;'>" + 
                   m_fullText.left(m_currentPosition).toHtmlEscaped() + 
                   "</span>";
    }
    
    if (m_currentPosition < m_fullText.length()) {
        // Aktualny znak - czerwone tło
        if (m_currentPosition < m_fullText.length()) {
            richText += "<span style='background-color: #ff3333; color: #ffffff;'>" + 
                       QString(m_fullText.at(m_currentPosition)).toHtmlEscaped() + 
                       "</span>";
        }
        
        // Pozostały tekst - szary
        if (m_currentPosition + 1 < m_fullText.length()) {
            richText += "<span style='color: #bbbbbb;'>" + 
                       m_fullText.mid(m_currentPosition + 1).toHtmlEscaped() + 
                       "</span>";
        }
    }
    
    m_displayTextLabel->setText(richText);
}

void TypingTestLayer::onTextChanged(const QString& text) {
    if (m_testCompleted) {
        return;
    }
    
    m_testStarted = true;
    
    // Sprawdzenie poprawności wprowadzonego tekstu
    bool isCorrect = true;
    for (int i = 0; i < text.length() && i < m_fullText.length(); i++) {
        if (text.at(i) != m_fullText.at(i)) {
            isCorrect = false;
            break;
        }
    }
    
    if (isCorrect) {
        m_currentPosition = text.length();
        updateDisplayText();
        
        // Sprawdzenie czy użytkownik przepisał cały tekst
        if (m_currentPosition == m_fullText.length()) {
            m_testCompleted = true;
            updateHighlight();
            
            // Zmiana kolorów na zielony po pomyślnym zakończeniu
            m_displayTextLabel->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
            
            // Opóźnienie przed zakończeniem warstwy
            QTimer::singleShot(1000, this, [this]() {
                QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
                this->setGraphicsEffect(effect);

                QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
                animation->setDuration(500);
                animation->setStartValue(1.0);
                animation->setEndValue(0.0);
                animation->setEasingCurve(QEasingCurve::OutQuad);

                connect(animation, &QPropertyAnimation::finished, this, [this]() {
                    emit layerCompleted();
                });

                animation->start(QPropertyAnimation::DeleteWhenStopped);
            });
        }
    } else {
        // Niepoprawny tekst - cofamy edycję
        QSignalBlocker blocker(m_hiddenInput);
        QString correctText = text.left(m_currentPosition);
        m_hiddenInput->setText(correctText);
    }
}

void TypingTestLayer::updateHighlight() {
    // Aktualizacja wyróżnienia po zakończeniu
    if (m_testCompleted) {
        QString richText = "<span style='color: #33ff33;'>" + 
                          m_fullText.toHtmlEscaped() + 
                          "</span>";
        m_displayTextLabel->setText(richText);
    }
}