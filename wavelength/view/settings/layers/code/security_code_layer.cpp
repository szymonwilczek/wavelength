#include "security_code_layer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QKeyEvent>
#include <QRegularExpressionValidator>

SecurityCodeLayer::SecurityCodeLayer(QWidget *parent)
    : SecurityLayer(parent), m_currentSecurityCode(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("SECURITY CODE VERIFICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    QLabel *instructions = new QLabel("Enter the 4-digit security code", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    // Kontener dla inputów
    QWidget* codeInputContainer = new QWidget(this);
    QHBoxLayout* codeLayout = new QHBoxLayout(codeInputContainer);
    codeLayout->setSpacing(12); // Odstęp między polami
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setAlignment(Qt::AlignCenter);

    // Tworzenie 4 pól na cyfry
    for (int i = 0; i < 4; i++) {
        QLineEdit* digitInput = new QLineEdit(this);
        digitInput->setFixedSize(60, 70);
        digitInput->setAlignment(Qt::AlignCenter);
        digitInput->setMaxLength(1);
        digitInput->setProperty("index", i); // Zapamiętujemy indeks pola

        // Tylko cyfry
        QRegularExpressionValidator* validator = new QRegularExpressionValidator(QRegularExpression("[0-9]"), digitInput);
        digitInput->setValidator(validator);

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
        {1969, "First human moon landing (Apollo 11)"},
        {1944, "Warsaw Uprising"},
        {1984, "Orwell's dystopian novel"},
        {1947, "Roswell incident"},
        {2001, "Space Odyssey movie by Kubrick"},
        {1903, "Wright brothers' first flight"},
        {1989, "Fall of the Berlin Wall"},
        {1986, "Chernobyl disaster"},
        {1955, "Einstein's death year"},
        {1962, "Cuban Missile Crisis"},
        {1215, "Magna Carta signing"},
        {1939, "Start of World War II"},
        {1969, "Woodstock music festival"},
        {1776, "US Declaration of Independence"},
        {1789, "French Revolution began"},
        {1917, "Russian Revolution"},
        {1957, "Sputnik launch (first satellite)"},
        {1912, "Titanic sank"},
        {1859, "Darwin's Origin of Species published"},
        {1994, "Genocide in Rwanda"},
        {1963, "JFK assassination"}
    };
}

SecurityCodeLayer::~SecurityCodeLayer() {
}

void SecurityCodeLayer::initialize() {
    reset();
    QString hint;
    m_currentSecurityCode = getRandomSecurityCode(hint);
    m_securityCodeHint->setText("HINT: " + hint);

    // Ustaw fokus na pierwszym polu
    if (!m_codeInputs.isEmpty()) {
        m_codeInputs.first()->setFocus();
    }
}

void SecurityCodeLayer::reset() {
    resetInputs();
    m_securityCodeHint->setText("");
}

void SecurityCodeLayer::resetInputs() {
    // Resetujemy wszystkie pola
    for (QLineEdit* input : m_codeInputs) {
        input->clear();
        input->setStyleSheet(
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
    }

    // Ustawiamy fokus na pierwszym polu
    if (!m_codeInputs.isEmpty()) {
        m_codeInputs.first()->setFocus();
    }
}

void SecurityCodeLayer::onDigitEntered() {
    // Sprawdzamy, które pole zostało zmienione
    QLineEdit* currentInput = qobject_cast<QLineEdit*>(sender());
    if (!currentInput)
        return;

    int currentIndex = currentInput->property("index").toInt();

    // Jeśli pole jest wypełnione, przechodzimy do następnego
    if (currentInput->text().length() == 1 && currentIndex < m_codeInputs.size() - 1) {
        m_codeInputs[currentIndex + 1]->setFocus();
    }

    // Sprawdzamy, czy wszystkie pola są wypełnione
    bool allFilled = true;
    for (QLineEdit* input : m_codeInputs) {
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
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        QLineEdit* input = qobject_cast<QLineEdit*>(obj);

        if (input && m_codeInputs.contains(input)) {
            int key = keyEvent->key();
            int currentIndex = input->property("index").toInt();

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
    QString enteredCode = getEnteredCode();

    if (enteredCode.toInt() == m_currentSecurityCode) {
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
    } else {
        // Efekt błędu
        showErrorEffect();
    }
}

QString SecurityCodeLayer::getEnteredCode() const {
    QString code;
    for (QLineEdit* input : m_codeInputs) {
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
    QString errorStyle =
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
    });
}

int SecurityCodeLayer::getRandomSecurityCode(QString& hint) {
    int index = QRandomGenerator::global()->bounded(m_securityCodes.size());
    hint = m_securityCodes[index].second;
    return m_securityCodes[index].first;
}