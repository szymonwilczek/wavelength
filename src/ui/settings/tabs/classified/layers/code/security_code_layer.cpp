#include "security_code_layer.h"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QKeyEvent>
#include <QRegularExpressionValidator>

SecurityCodeLayer::SecurityCodeLayer(QWidget *parent)
    : SecurityLayer(parent), is_current_code_numeric_(true)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    const auto title = new QLabel("SECURITY CODE VERIFICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    const auto instructions = new QLabel("Enter the security code", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    // Kontener dla inputów
    const auto code_input_container = new QWidget(this);
    const auto code_layout = new QHBoxLayout(code_input_container);
    code_layout->setSpacing(12); // Odstęp między polami
    code_layout->setContentsMargins(0, 0, 0, 0);
    code_layout->setAlignment(Qt::AlignCenter);

    // Tworzenie 4 pól na znaki
    for (int i = 0; i < 4; i++) {
        auto digit_input = new QLineEdit(this);
        digit_input->setFixedSize(60, 70);
        digit_input->setAlignment(Qt::AlignCenter);
        digit_input->setMaxLength(1);
        digit_input->setProperty("index", i); // Zapamiętujemy indeks pola

        digit_input->setStyleSheet(
            "QLineEdit {"
            "  color: #ff3333;"
            "  background-color: rgba(10, 25, 40, 220);"
            "  border: 1px solid #ff3333;"
            "  border-radius: 5px;"
            "  padding: 5px;"
            "  font-family: Consolas;"
            "  font-size: 30pt;"
            "  font-weight: bold;"
            "}"
            "QLineEdit:focus {"
            "  border: 2px solid #ff3333;"
            "  background-color: rgba(25, 40, 65, 220);"
            "}"
        );

        // Obsługa zmian w polu
        connect(digit_input, &QLineEdit::textChanged, this, &SecurityCodeLayer::OnDigitEntered);

        // Instalujemy filtr zdarzeń do obsługi klawiszy nawigacji
        digit_input->installEventFilter(this);

        code_inputs_.append(digit_input);
        code_layout->addWidget(digit_input);
    }

    security_code_hint_ = new QLabel("", this);
    security_code_hint_->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt; font-style: italic;");
    security_code_hint_->setAlignment(Qt::AlignCenter);
    security_code_hint_->setWordWrap(true);
    security_code_hint_->setFixedWidth(350);

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(instructions);
    layout->addSpacing(10);
    layout->addWidget(code_input_container, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(security_code_hint_);
    layout->addStretch();

    // Inicjalizacja bazy kodów bezpieczeństwa
    security_codes_ = {
        // Cyberpunk & technologiczne kody
        SecurityCode("2077", "Night City's most notorious year"),
        SecurityCode("1337", "Elite hacker language code"),
        SecurityCode("CODE", "Base representation of what you're entering", false),
        SecurityCode("HACK", "What hackers do best", false),
        SecurityCode("GHOS", "In the machine (famous cyberpunk concept)", false),
        SecurityCode("D3CK", "Cyberdeck connection interface (leet)", false),

        // Matematyczne i naukowe kody
        SecurityCode("3141", "First digits of π (pi)"),
        SecurityCode("1618", "Golden ratio φ (phi) first digits"),
        SecurityCode("2718", "Euler's number (e) first digits"),
        SecurityCode("1729", "Hardy-Ramanujan number (taxicab)"),
        SecurityCode("6174", "Kaprekar's constant"),
        SecurityCode("2048", "2^11, popular game"),

        // Daty związane z technologią, cyberpunkiem i sci-fi
        SecurityCode("1984", "Orwell's dystopian surveillance society"),
        SecurityCode("2001", "Kubrick's space odyssey with HAL"),
        SecurityCode("1982", "Blade Runner release year"),
        SecurityCode("1999", "The Matrix release year"),
        SecurityCode("1969", "ARPANET created (Internet precursor)"),
        SecurityCode("1989", "World Wide Web invented"),

        // Kulturowe i kryptograficzne
        SecurityCode("BOOK", "Digital knowledge container", false),
        SecurityCode("EV1L", "C0RP - Cyberpunk antagonist trope", false),
        SecurityCode("LOCK", "Security metaphor", false),

        // Easter eggi
        SecurityCode("RICK", "Never gonna give you up...", false),
        SecurityCode("8080", "Classic Intel processor"),
        SecurityCode("6502", "CPU used in Apple II and Commodore 64"),
    };
}

SecurityCodeLayer::~SecurityCodeLayer() {
}

void SecurityCodeLayer::Initialize() {
    Reset();

    bool is_numeric;
    QString hint;
    current_security_code_ = GetRandomSecurityCode(hint, is_numeric);
    is_current_code_numeric_ = is_numeric;
    SetInputValidators(is_numeric);
    security_code_hint_->setText("HINT: " + hint);

    if (graphicsEffect()) {
        dynamic_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }

    if (!code_inputs_.isEmpty()) {
        code_inputs_.first()->setFocus();
    }
}

void SecurityCodeLayer::Reset() {
    ResetInputs();
    security_code_hint_->setText("");

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void SecurityCodeLayer::ResetInputs() {
    // Resetujemy wszystkie pola
    for (QLineEdit* input : code_inputs_) {
        input->clear();
        // Przywróć domyślny styl (czerwony)
        input->setStyleSheet(
            "QLineEdit {"
            "  color: #ff3333;" // Czerwony tekst
            "  background-color: rgba(10, 25, 40, 220);"
            "  border: 1px solid #ff3333;" // Czerwona ramka
            "  border-radius: 5px;"
            "  padding: 5px;"
            "  font-family: Consolas;"
            "  font-size: 30pt;"
            "  font-weight: bold;"
            "}"
            "QLineEdit:focus {"
            "  border: 2px solid #ff3333;" // Czerwona ramka w focusie
            "  background-color: rgba(25, 40, 65, 220);"
            "}"
        );
    }

    // Ustawiamy fokus na pierwszym polu
    if (!code_inputs_.isEmpty()) {
        code_inputs_.first()->setFocus();
    }
}

void SecurityCodeLayer::SetInputValidators(const bool numeric_only) {
    for (QLineEdit* input : code_inputs_) {
        if (numeric_only) {
            // Tylko cyfry
            const auto validator = new QRegularExpressionValidator(QRegularExpression("[0-9]"), input);
            input->setValidator(validator);
        } else {
            // Cyfry i wielkie litery (styl cyberpunk)
            const auto validator = new QRegularExpressionValidator(QRegularExpression("[0-9A-Z]"), input);
            input->setValidator(validator);
        }
    }
}

void SecurityCodeLayer::OnDigitEntered() {
    // Sprawdzamy, które pole zostało zmienione
    const auto current_input = qobject_cast<QLineEdit*>(sender());
    if (!current_input)
        return;

    const int current_index = current_input->property("index").toInt();

    // Automatyczna konwersja na wielkie litery dla kodów alfanumerycznych
    if (!is_current_code_numeric_ && !current_input->text().isEmpty()) {
        const QString text = current_input->text().toUpper();
        if (text != current_input->text()) {
            current_input->setText(text);
            return; // Zapobiega nieskończonej pętli
        }
    }

    // Jeśli pole jest wypełnione, przechodzimy do następnego
    if (current_input->text().length() == 1 && current_index < code_inputs_.size() - 1) {
        code_inputs_[current_index + 1]->setFocus();
    }

    // Sprawdzamy, czy wszystkie pola są wypełnione
    bool all_filled = true;
    for (const QLineEdit* input : code_inputs_) {
        if (input->text().isEmpty()) {
            all_filled = false;
            break;
        }
    }

    // Jeśli wszystkie pola są wypełnione, sprawdzamy kod
    if (all_filled) {
        CheckSecurityCode();
    }
}

bool SecurityCodeLayer::eventFilter(QObject *obj, QEvent *event) {
    // Obsługa klawiszy w inputach
    if (event->type() == QEvent::KeyPress) {
        const auto key_event = static_cast<QKeyEvent*>(event);
        const auto input = qobject_cast<QLineEdit*>(obj);

        if (input && code_inputs_.contains(input)) {
            const int key = key_event->key();
            const int current_index = input->property("index").toInt();

            // Obsługa klawisza Backspace
            if (key == Qt::Key_Backspace && input->text().isEmpty() && current_index > 0) {
                // Jeśli pole jest puste i naciśnięto Backspace, przechodzimy do poprzedniego pola
                code_inputs_[current_index - 1]->setFocus();
                code_inputs_[current_index - 1]->setText("");
                return true;
            }

            // Obsługa strzałek
            if (key == Qt::Key_Left && current_index > 0) {
                code_inputs_[current_index - 1]->setFocus();
                return true;
            }

            if (key == Qt::Key_Right && current_index < code_inputs_.size() - 1) {
                code_inputs_[current_index + 1]->setFocus();
                return true;
            }
        }
    }

    return SecurityLayer::eventFilter(obj, event);
}

void SecurityCodeLayer::CheckSecurityCode() {
    // Pobieramy wprowadzony kod
    const QString entered_code = GetEnteredCode();

    // Sprawdzamy, czy wprowadzony kod zgadza się z wylosowanym
    if (entered_code == current_security_code_) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        for (QLineEdit* input : code_inputs_) {
            input->setStyleSheet(
                "QLineEdit {"
                "  color: #33ff33;"
                "  background-color: rgba(10, 25, 40, 220);"
                "  border: 2px solid #33ff33;"
                "  border-radius: 5px;"
                "  padding: 5px;"
                "  font-family: Consolas;"
                "  font-size: 30pt;"
                "  font-weight: bold;"
                "}"
            );
        }

        // Małe opóźnienie przed animacją zanikania, aby pokazać zmianę kolorów
        QTimer::singleShot(800, this, [this]() {
            // Animacja zanikania
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
    } else {
        // Efekt błędu
        ShowErrorEffect();
    }
}

QString SecurityCodeLayer::GetEnteredCode() const {
    QString code;
    for (const QLineEdit* input : code_inputs_) {
        code += input->text();
    }
    return code;
}

void SecurityCodeLayer::ShowErrorEffect() {
    // Zapisujemy oryginalny styl
    QString original_style =
        "QLineEdit {"
        "  color: #ff3333;"
        "  background-color: rgba(10, 25, 40, 220);"
        "  border: 1px solid #ff3333;"
        "  border-radius: 5px;"
        "  padding: 5px;"
        "  font-family: Consolas;"
        "  font-size: 30pt;"
        "  font-weight: bold;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #ff3333;"
        "  background-color: rgba(25, 40, 65, 220);"
        "}";

    // Styl błędu
    const QString error_style =
        "QLineEdit {"
        "  color: #ffffff;"
        "  background-color: rgba(255, 0, 0, 100);"
        "  border: 1px solid #ff0000;"
        "  border-radius: 5px;"
        "  padding: 5px;"
        "  font-family: Consolas;"
        "  font-size: 30pt;"
        "  font-weight: bold;"
        "}";

    // Ustawiamy styl błędu dla wszystkich pól
    for (QLineEdit* input : code_inputs_) {
        input->setStyleSheet(error_style);
    }

    // Przywracamy oryginalny styl po krótkim czasie i resetujemy inputy
    QTimer::singleShot(300, this, [this, original_style]() {
        for (QLineEdit* input : code_inputs_) {
            input->setStyleSheet(original_style);
        }
        ResetInputs();

        // Po resecie przywracamy właściwe walidatory
        SetInputValidators(is_current_code_numeric_);
    });
}

QString SecurityCodeLayer::GetRandomSecurityCode(QString& hint, bool& is_numeric) {
    const int index = QRandomGenerator::global()->bounded(security_codes_.size());
    hint = security_codes_[index].hint;
    is_numeric = security_codes_[index].is_numeric;
    return security_codes_[index].code;
}