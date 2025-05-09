#include "typing_test_layer.h"
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFontMetrics>

#include "../../../../../../app/managers/translation_manager.h"

TypingTestLayer::TypingTestLayer(QWidget *parent) 
    : SecurityLayer(parent),
      current_position_(0),
      test_started_(false),
      test_completed_(false)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(15);

    const TranslationManager* translator = TranslationManager::GetInstance();

    title_label_ = new QLabel(translator->Translate("ClassifiedSettingsWidget.TypingTest.Title",
        "TYPING VERIFICATION TEST"), this);
    title_label_->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title_label_->setAlignment(Qt::AlignCenter);

    instructions_label_ = new QLabel(translator->Translate("ClassifiedSettingsWidget.TypingTest.Info",
        "Type the following text to continue"), this);
    instructions_label_->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions_label_->setAlignment(Qt::AlignCenter);

    // Panel z tekstem do przepisania
    const auto text_panel = new QWidget(this);
    text_panel->setFixedSize(600, 120);
    text_panel->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");

    const auto text_layout = new QVBoxLayout(text_panel);
    text_layout->setContentsMargins(20, 20, 20, 20);
    text_layout->setAlignment(Qt::AlignVCenter);

    display_text_label_ = new QLabel(text_panel);
    display_text_label_->setStyleSheet("color: #bbbbbb; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
    display_text_label_->setWordWrap(true);
    text_layout->addWidget(display_text_label_);

    // Ukryte pole do wprowadzania tekstu
    hidden_input_ = new QLineEdit(this);
    hidden_input_->setFixedWidth(0);
    hidden_input_->setFixedHeight(0);
    hidden_input_->setStyleSheet("background-color: transparent; border: none; color: transparent;");

    // Połączenie sygnału zmiany tekstu
    connect(hidden_input_, &QLineEdit::textChanged, this, &TypingTestLayer::OnTextChanged);

    layout->addWidget(title_label_);
    layout->addWidget(instructions_label_);
    layout->addWidget(text_panel);
    layout->addWidget(hidden_input_);
    layout->addStretch();

    // Generowanie tekstu do testu pisania
    GenerateWords();

    // Ustawienie wyświetlanego tekstu
    UpdateDisplayText();
}

TypingTestLayer::~TypingTestLayer() {
}

void TypingTestLayer::Initialize() {
    Reset();
    hidden_input_->setFocus();

    if (graphicsEffect()) {
         static_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }
}

void TypingTestLayer::Reset() {
    current_position_ = 0;
    test_started_ = false;
    test_completed_ = false;
    hidden_input_->clear();

    // Generuj nowy zestaw słów
    GenerateWords();
    UpdateDisplayText(); // Wyświetl nowy tekst

    // Reset stylów etykiety wyświetlającej tekst
    display_text_label_->setStyleSheet("color: #bbbbbb; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
    // Reset stylów panelu i tytułu (na wypadek gdyby były zmieniane)
    if (QWidget* text_panel = display_text_label_->parentWidget()) {
        text_panel->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;"); // Czerwony border
    }
    title_label_->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;"); // Czerwony tytuł
    instructions_label_->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;"); // Szare instrukcje


    // --- KLUCZOWA ZMIANA: Przywróć przezroczystość ---
    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
        // Opcjonalnie: Można usunąć efekt
        // this->setGraphicsEffect(nullptr);
        // delete effect;
    }
}

void TypingTestLayer::GenerateWords() {
    // Lista angielskich słów do losowania
    const QStringList word_pool = {
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
    words_.clear();

    // Historia ostatnio wybranych słów (dla unikania powtórzeń)
    QStringList recent_words;

    // Inicjalizacja metryki czcionki do pomiaru szerokości tekstu
    const QFont font("Consolas", 16);
    const QFontMetrics metrics(font);

    // Maksymalna szerokość etykiety
    constexpr int max_width = 560; // 600 (szerokość panelu) - 2*20 (margines)

    // Symulacja tekstu i liczenie linii
    QString current_line;
    int line_count = 1;

    while (words_.size() < 30) { // Maksymalna liczba słów dla bezpieczeństwa
        // Wybierz losowe słowo, które nie powtarza się zbyt często
        QString word;
        bool valid_word = false;

        int attempts = 0;
        while (!valid_word && attempts < 50) {
            const int index = QRandomGenerator::global()->bounded(word_pool.size());
            word = word_pool.at(index);

            // Sprawdź, czy słowo nie występuje wśród ostatnich 2 wybranych
            if (!recent_words.contains(word)) {
                valid_word = true;
            }
            attempts++;
        }

        // Jeśli nie znaleziono odpowiedniego słowa po 50 próbach, wybierz dowolne
        if (!valid_word) {
            const int index = QRandomGenerator::global()->bounded(word_pool.size());
            word = word_pool.at(index);
        }

        // Symulacja dodania słowa do tekstu
        QString test_line = current_line;
        if (!test_line.isEmpty()) {
            test_line += " ";
        }
        test_line += word;

        // Sprawdź, czy testLine mieści się w jednej linii
        if (metrics.horizontalAdvance(test_line) <= max_width) {
            current_line = test_line;
        } else {
            // Nowa linia
            line_count++;

            // Sprawdź, czy nie przekraczamy 3 linii
            if (line_count > 3) {
                // Spróbuj znaleźć krótsze słowo
                bool found_shorter_word = false;
                for (int i = 0; i < 10; i++) { // Maksymalnie 10 prób
                    const int index = QRandomGenerator::global()->bounded(word_pool.size());
                    QString shorter_word = word_pool.at(index);

                    // Jeśli słowo jest krótsze i nie występuje wśród ostatnich 2
                    if (shorter_word.length() < word.length() && !recent_words.contains(shorter_word)) {
                        if (metrics.horizontalAdvance(shorter_word) <= max_width) {
                            word = shorter_word;
                            found_shorter_word = true;
                            break;
                        }
                    }
                }

                // Jeśli nie znaleziono krótszego słowa, kończymy generowanie
                if (!found_shorter_word) {
                    break;
                }

                // Reset linii z nowym krótszym słowem
                current_line = word;
                line_count = 1;
            } else {
                // Kontynuuj z nową linią
                current_line = word;
            }
        }

        // Dodaj słowo do listy i zaktualizuj historię
        words_.append(word);

        // Aktualizacja historii ostatnich słów
        recent_words.append(word);
        if (recent_words.size() > 2) {
            recent_words.removeFirst();
        }

        // Sprawdź, czy wygenerowano co najmniej 10 słów i czy są już 3 linie
        if (words_.size() >= 10 && line_count >= 3) {
            break;
        }
    }

    // Tworzenie pełnego tekstu
    full_text_ = words_.join(" ");
}

void TypingTestLayer::UpdateDisplayText() const {
    if (full_text_.isEmpty()) {
        return;
    }

    // Tworzenie rich text z wyróżnieniem
    QString rich_text;
    
    if (current_position_ > 0) {
        // Już wpisany tekst - zielony
        rich_text += "<span style='color: #33ff33;'>" +
                   full_text_.left(current_position_).toHtmlEscaped() +
                   "</span>";
    }
    
    if (current_position_ < full_text_.length()) {
        // Aktualny znak - czerwone tło
        if (current_position_ < full_text_.length()) {
            rich_text += "<span style='background-color: #ff3333; color: #ffffff;'>" +
                       QString(full_text_.at(current_position_)).toHtmlEscaped() +
                       "</span>";
        }
        
        // Pozostały tekst - szary
        if (current_position_ + 1 < full_text_.length()) {
            rich_text += "<span style='color: #bbbbbb;'>" +
                       full_text_.mid(current_position_ + 1).toHtmlEscaped() +
                       "</span>";
        }
    }
    
    display_text_label_->setText(rich_text);
}

void TypingTestLayer::OnTextChanged(const QString& text) {
    if (test_completed_) {
        return;
    }
    
    test_started_ = true;
    
    // Sprawdzenie poprawności wprowadzonego tekstu
    bool is_correct = true;
    for (int i = 0; i < text.length() && i < full_text_.length(); i++) {
        if (text.at(i) != full_text_.at(i)) {
            is_correct = false;
            break;
        }
    }
    
    if (is_correct) {
        current_position_ = text.length();
        UpdateDisplayText();
        
        // Sprawdzenie czy użytkownik przepisał cały tekst
        if (current_position_ == full_text_.length()) {
            test_completed_ = true;
            UpdateHighlight();
            
            // Zmiana kolorów na zielony po pomyślnym zakończeniu
            display_text_label_->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 16pt; background-color: transparent; border: none;");
            
            // Opóźnienie przed zakończeniem warstwy
            QTimer::singleShot(1000, this, [this]() {
                const auto effect = new QGraphicsOpacityEffect(this);
                this->setGraphicsEffect(effect);

                const auto animation = new QPropertyAnimation(effect, "opacity");
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
        QSignalBlocker blocker(hidden_input_);
        const QString correct_text = text.left(current_position_);
        hidden_input_->setText(correct_text);
    }
}

void TypingTestLayer::UpdateHighlight() const {
    // Aktualizacja wyróżnienia po zakończeniu
    if (test_completed_) {
        const QString rich_text = "<span style='color: #33ff33;'>" +
                          full_text_.toHtmlEscaped() +
                          "</span>";
        display_text_label_->setText(rich_text);
    }
}
