#include "security_code_layer.h"
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>

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

    m_securityCodeInput = new QLineEdit(this);
    m_securityCodeInput->setFixedWidth(150);
    m_securityCodeInput->setStyleSheet(
        "QLineEdit {"
        "  color: #ff3333;"
        "  background-color: rgba(10, 25, 40, 220);"
        "  border: 1px solid #ff3333;"
        "  border-radius: 5px;"
        "  padding: 8px;"
        "  font-family: Consolas;"
        "  font-size: 14pt;"
        "  text-align: center;"
        "}"
    );
    m_securityCodeInput->setAlignment(Qt::AlignCenter);
    m_securityCodeInput->setMaxLength(4);
    m_securityCodeInput->setInputMask("9999");

    m_securityCodeHint = new QLabel("", this);
    m_securityCodeHint->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt; font-style: italic;");
    m_securityCodeHint->setAlignment(Qt::AlignCenter);
    m_securityCodeHint->setWordWrap(true);
    m_securityCodeHint->setFixedWidth(350);

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(instructions);
    layout->addSpacing(10);
    layout->addWidget(m_securityCodeInput, 0, Qt::AlignCenter);
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

    connect(m_securityCodeInput, &QLineEdit::returnPressed, this, &SecurityCodeLayer::checkSecurityCode);
}

SecurityCodeLayer::~SecurityCodeLayer() {
}

void SecurityCodeLayer::initialize() {
    reset();
    QString hint;
    m_currentSecurityCode = getRandomSecurityCode(hint);
    m_securityCodeHint->setText("HINT: " + hint);
    m_securityCodeInput->setFocus();
}

void SecurityCodeLayer::reset() {
    m_securityCodeInput->clear();
    m_securityCodeHint->setText("");
}

void SecurityCodeLayer::checkSecurityCode() {
    if (m_securityCodeInput->text().toInt() == m_currentSecurityCode) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        m_securityCodeInput->setStyleSheet(
            "QLineEdit {"
            "  color: #33ff33;"
            "  background-color: rgba(10, 25, 40, 220);"
            "  border: 2px solid #33ff33;"
            "  border-radius: 5px;"
            "  padding: 8px;"
            "  font-family: Consolas;"
            "  font-size: 14pt;"
            "  text-align: center;"
            "}"
        );

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
        // Efekt błędu (pozostaje bez zmian)
        QString currentStyle = m_securityCodeInput->styleSheet();
        m_securityCodeInput->setStyleSheet(
            "QLineEdit {"
            "  color: #ffffff;"
            "  background-color: rgba(255, 0, 0, 100);"
            "  border: 1px solid #ff0000;"
            "  border-radius: 5px;"
            "  padding: 8px;"
            "  font-family: Consolas;"
            "  font-size: 14pt;"
            "  text-align: center;"
            "}"
        );

        QTimer::singleShot(300, this, [this, currentStyle]() {
            m_securityCodeInput->setStyleSheet(currentStyle);
            m_securityCodeInput->clear();
            m_securityCodeInput->setFocus();
        });
    }
}

int SecurityCodeLayer::getRandomSecurityCode(QString& hint) {
    int index = QRandomGenerator::global()->bounded(m_securityCodes.size());
    hint = m_securityCodes[index].second;
    return m_securityCodes[index].first;
}