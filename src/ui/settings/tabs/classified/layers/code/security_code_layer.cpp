#include "security_code_layer.h"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QKeyEvent>
#include <QRegularExpressionValidator>

SecurityCodeLayer::SecurityCodeLayer(QWidget *parent)
    : SecurityLayer(parent), m_isCurrentCodeNumeric(true)
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
    const auto codeInputContainer = new QWidget(this);
    const auto codeLayout = new QHBoxLayout(codeInputContainer);
    codeLayout->setSpacing(12); // Odstęp między polami
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setAlignment(Qt::AlignCenter);

    // Tworzenie 4 pól na znaki
    for (int i = 0; i < 4; i++) {
        auto digitInput = new QLineEdit(this);
        digitInput->setFixedSize(60, 70);
        digitInput->setAlignment(Qt::AlignCenter);
        digitInput->setMaxLength(1);
        digitInput->setProperty("index", i); // Zapamiętujemy indeks pola

        digitInput->setStyleSheet(
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
        connect(digitInput, &QLineEdit::textChanged, this, &SecurityCodeLayer::onDigitEntered);

        // Instalujemy filtr zdarzeń do obsługi klawiszy nawigacji
        digitInput->installEventFilter(this);

        m_codeInputs.append(digitInput);
        codeLayout->addWidget(digitInput);
    }

    m_securityCodeHint = new QLabel("", this);
    m_securityCodeHint->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt; font-style: italic;");
    m_securityCodeHint->setAlignment(Qt::AlignCenter);
    m_securityCodeHint->setWordWrap(true);
    m_securityCodeHint->setFixedWidth(350);

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(instructions);
    layout->addSpacing(10);
    layout->addWidget(codeInputContainer, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(m_securityCodeHint);
    layout->addStretch();

    // Inicjalizacja bazy kodów bezpieczeństwa
    m_securityCodes = {
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

void SecurityCodeLayer::initialize() {
    reset();

    bool isNumeric;
    QString hint;
    m_currentSecurityCode = getRandomSecurityCode(hint, isNumeric);
    m_isCurrentCodeNumeric = isNumeric;
    setInputValidators(isNumeric);
    m_securityCodeHint->setText("HINT: " + hint);

    if (graphicsEffect()) {
        dynamic_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }

    if (!m_codeInputs.isEmpty()) {
        m_codeInputs.first()->setFocus();
    }
}

void SecurityCodeLayer::reset() {
    resetInputs();
    m_securityCodeHint->setText("");

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void SecurityCodeLayer::resetInputs() {
    // Resetujemy wszystkie pola
    for (QLineEdit* input : m_codeInputs) {
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
    if (!m_codeInputs.isEmpty()) {
        m_codeInputs.first()->setFocus();
    }
}

void SecurityCodeLayer::setInputValidators(const bool numericOnly) {
    for (QLineEdit* input : m_codeInputs) {
        if (numericOnly) {
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

void SecurityCodeLayer::onDigitEntered() {
    // Sprawdzamy, które pole zostało zmienione
    const auto currentInput = qobject_cast<QLineEdit*>(sender());
    if (!currentInput)
        return;

    const int currentIndex = currentInput->property("index").toInt();

    // Automatyczna konwersja na wielkie litery dla kodów alfanumerycznych
    if (!m_isCurrentCodeNumeric && !currentInput->text().isEmpty()) {
        const QString text = currentInput->text().toUpper();
        if (text != currentInput->text()) {
            currentInput->setText(text);
            return; // Zapobiega nieskończonej pętli
        }
    }

    // Jeśli pole jest wypełnione, przechodzimy do następnego
    if (currentInput->text().length() == 1 && currentIndex < m_codeInputs.size() - 1) {
        m_codeInputs[currentIndex + 1]->setFocus();
    }

    // Sprawdzamy, czy wszystkie pola są wypełnione
    bool allFilled = true;
    for (const QLineEdit* input : m_codeInputs) {
        if (input->text().isEmpty()) {
            allFilled = false;
            break;
        }
    }

    // Jeśli wszystkie pola są wypełnione, sprawdzamy kod
    if (allFilled) {
        checkSecurityCode();
    }
}

bool SecurityCodeLayer::eventFilter(QObject *obj, QEvent *event) {
    // Obsługa klawiszy w inputach
    if (event->type() == QEvent::KeyPress) {
        const auto keyEvent = static_cast<QKeyEvent*>(event);
        const auto input = qobject_cast<QLineEdit*>(obj);

        if (input && m_codeInputs.contains(input)) {
            const int key = keyEvent->key();
            const int currentIndex = input->property("index").toInt();

            // Obsługa klawisza Backspace
            if (key == Qt::Key_Backspace && input->text().isEmpty() && currentIndex > 0) {
                // Jeśli pole jest puste i naciśnięto Backspace, przechodzimy do poprzedniego pola
                m_codeInputs[currentIndex - 1]->setFocus();
                m_codeInputs[currentIndex - 1]->setText("");
                return true;
            }

            // Obsługa strzałek
            if (key == Qt::Key_Left && currentIndex > 0) {
                m_codeInputs[currentIndex - 1]->setFocus();
                return true;
            }

            if (key == Qt::Key_Right && currentIndex < m_codeInputs.size() - 1) {
                m_codeInputs[currentIndex + 1]->setFocus();
                return true;
            }
        }
    }

    return SecurityLayer::eventFilter(obj, event);
}

void SecurityCodeLayer::checkSecurityCode() {
    // Pobieramy wprowadzony kod
    const QString enteredCode = getEnteredCode();

    // Sprawdzamy, czy wprowadzony kod zgadza się z wylosowanym
    if (enteredCode == m_currentSecurityCode) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        for (QLineEdit* input : m_codeInputs) {
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
        showErrorEffect();
    }
}

QString SecurityCodeLayer::getEnteredCode() const {
    QString code;
    for (const QLineEdit* input : m_codeInputs) {
        code += input->text();
    }
    return code;
}

void SecurityCodeLayer::showErrorEffect() {
    // Zapisujemy oryginalny styl
    QString originalStyle =
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
    const QString errorStyle =
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
    for (QLineEdit* input : m_codeInputs) {
        input->setStyleSheet(errorStyle);
    }

    // Przywracamy oryginalny styl po krótkim czasie i resetujemy inputy
    QTimer::singleShot(300, this, [this, originalStyle]() {
        for (QLineEdit* input : m_codeInputs) {
            input->setStyleSheet(originalStyle);
        }
        resetInputs();

        // Po resecie przywracamy właściwe walidatory
        setInputValidators(m_isCurrentCodeNumeric);
    });
}

QString SecurityCodeLayer::getRandomSecurityCode(QString& hint, bool& isNumeric) {
    const int index = QRandomGenerator::global()->bounded(m_securityCodes.size());
    hint = m_securityCodes[index].hint;
    isNumeric = m_securityCodes[index].isNumeric;
    return m_securityCodes[index].code;
}