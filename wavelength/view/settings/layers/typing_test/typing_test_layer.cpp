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

    m_instructionsLabel = new QLabel("Type the following text to continue", this);
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

    // Generowanie tekstu do testu pisania
    generateWords();

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

    // Generuj nowy zestaw słów
    generateWords();
    updateDisplayText();

    // Reset stylów
    m_displayTextLabel->setStyleSheet("color: #bbbbbb; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
}

void TypingTestLayer::generateWords() {
    // Lista angielskich słów do losowania
    QStringList wordPool = {
        "system", "access", "terminal", "password", "protocol",
        "network", "server", "data", "database", "module",
        "channel", "interface", "computer", "monitor", "keyboard",
        "security", "verify", "program", "algorithm", "process",
        "analysis", "scanning", "transfer", "connection", "application",
        "code", "binary", "cache", "client", "cloud",
        "debug", "delete", "detect", "device", "digital",
        "download", "email", "encrypt", "file", "firewall",
        "folder", "hardware", "install", "internet", "keyboard",
        "login", "memory", "message", "modem", "mouse",
        "offline", "online", "output", "processor", "proxy",
        "router", "script", "software", "storage", "update",
        "upload", "virtual", "website", "wireless", "cybernetic"
    };

    // Inicjalizacja generatora liczb losowych
    QRandomGenerator::securelySeeded();

    // Generowanie losowych słów z unikaniem powtórzeń
    m_words.clear();

    // Historia ostatnio wybranych słów (dla unikania powtórzeń)
    QStringList recentWords;

    // Inicjalizacja metryki czcionki do pomiaru szerokości tekstu
    QFont font("Consolas", 16);
    QFontMetrics metrics(font);

    // Maksymalna szerokość etykiety
    int maxWidth = 560; // 600 (szerokość panelu) - 2*20 (margines)

    // Symulacja tekstu i liczenie linii
    QString currentLine;
    int lineCount = 1;

    while (m_words.size() < 30) { // Maksymalna liczba słów dla bezpieczeństwa
        // Wybierz losowe słowo, które nie powtarza się zbyt często
        QString word;
        bool validWord = false;

        int attempts = 0;
        while (!validWord && attempts < 50) {
            int index = QRandomGenerator::global()->bounded(wordPool.size());
            word = wordPool.at(index);

            // Sprawdź, czy słowo nie występuje wśród ostatnich 2 wybranych
            if (!recentWords.contains(word)) {
                validWord = true;
            }
            attempts++;
        }

        // Jeśli nie znaleziono odpowiedniego słowa po 50 próbach, wybierz dowolne
        if (!validWord) {
            int index = QRandomGenerator::global()->bounded(wordPool.size());
            word = wordPool.at(index);
        }

        // Symulacja dodania słowa do tekstu
        QString testLine = currentLine;
        if (!testLine.isEmpty()) {
            testLine += " ";
        }
        testLine += word;

        // Sprawdź, czy testLine mieści się w jednej linii
        if (metrics.horizontalAdvance(testLine) <= maxWidth) {
            currentLine = testLine;
        } else {
            // Nowa linia
            lineCount++;

            // Sprawdź, czy nie przekraczamy 3 linii
            if (lineCount > 3) {
                // Spróbuj znaleźć krótsze słowo
                bool foundShorterWord = false;
                for (int i = 0; i < 10; i++) { // Maksymalnie 10 prób
                    int index = QRandomGenerator::global()->bounded(wordPool.size());
                    QString shorterWord = wordPool.at(index);

                    // Jeśli słowo jest krótsze i nie występuje wśród ostatnich 2
                    if (shorterWord.length() < word.length() && !recentWords.contains(shorterWord)) {
                        if (metrics.horizontalAdvance(shorterWord) <= maxWidth) {
                            word = shorterWord;
                            foundShorterWord = true;
                            break;
                        }
                    }
                }

                // Jeśli nie znaleziono krótszego słowa, kończymy generowanie
                if (!foundShorterWord) {
                    break;
                }

                // Reset linii z nowym krótszym słowem
                currentLine = word;
                lineCount = 1;
            } else {
                // Kontynuuj z nową linią
                currentLine = word;
            }
        }

        // Dodaj słowo do listy i zaktualizuj historię
        m_words.append(word);

        // Aktualizacja historii ostatnich słów
        recentWords.append(word);
        if (recentWords.size() > 2) {
            recentWords.removeFirst();
        }

        // Sprawdź, czy wygenerowano co najmniej 10 słów i czy są już 3 linie
        if (m_words.size() >= 10 && lineCount >= 3) {
            break;
        }
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