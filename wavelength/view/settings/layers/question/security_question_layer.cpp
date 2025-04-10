#include "security_question_layer.h"
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

SecurityQuestionLayer::SecurityQuestionLayer(QWidget *parent) 
    : SecurityLayer(parent), m_securityQuestionTimer(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("SECURITY QUESTION VERIFICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    QLabel *instructions = new QLabel("Answer your security question", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    m_securityQuestionLabel = new QLabel("", this);
    m_securityQuestionLabel->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    m_securityQuestionLabel->setAlignment(Qt::AlignCenter);
    m_securityQuestionLabel->setWordWrap(true);
    m_securityQuestionLabel->setFixedWidth(400);

    m_securityQuestionInput = new QLineEdit(this);
    m_securityQuestionInput->setFixedWidth(300);
    m_securityQuestionInput->setStyleSheet(
        "QLineEdit {"
        "  color: #ff3333;"
        "  background-color: rgba(10, 25, 40, 220);"
        "  border: 1px solid #ff3333;"
        "  border-radius: 5px;"
        "  padding: 8px;"
        "  font-family: Consolas;"
        "  font-size: 11pt;"
        "}"
    );

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(instructions);
    layout->addSpacing(10);
    layout->addWidget(m_securityQuestionLabel);
    layout->addSpacing(20);
    layout->addWidget(m_securityQuestionInput, 0, Qt::AlignCenter);
    layout->addStretch();

    m_securityQuestionTimer = new QTimer(this);
    m_securityQuestionTimer->setSingleShot(true);
    m_securityQuestionTimer->setInterval(10000);
    connect(m_securityQuestionTimer, &QTimer::timeout, this, &SecurityQuestionLayer::securityQuestionTimeout);

    connect(m_securityQuestionInput, &QLineEdit::returnPressed, this, &SecurityQuestionLayer::checkSecurityAnswer);
}

SecurityQuestionLayer::~SecurityQuestionLayer() {
    if (m_securityQuestionTimer) {
        m_securityQuestionTimer->stop();
        delete m_securityQuestionTimer;
        m_securityQuestionTimer = nullptr;
    }
}

void SecurityQuestionLayer::initialize() {
    reset();
    m_securityQuestionLabel->setText("What is your top secret security question?");
    m_securityQuestionInput->setFocus();
    m_securityQuestionTimer->start();
}

void SecurityQuestionLayer::reset() {
    if (m_securityQuestionTimer->isActive()) {
        m_securityQuestionTimer->stop();
    }
    m_securityQuestionInput->clear();
    m_securityQuestionLabel->setText("");
}

void SecurityQuestionLayer::checkSecurityAnswer() {
    // W tej implementacji każda odpowiedź jest prawidłowa - to część żartu
    // W prawdziwym systemie bezpieczeństwa byłoby to inaczej zaimplementowane
    
    // Zmiana kolorów na zielony po pomyślnym przejściu
    m_securityQuestionInput->setStyleSheet(
        "QLineEdit {"
        "  color: #33ff33;"
        "  background-color: rgba(10, 25, 40, 220);"
        "  border: 2px solid #33ff33;"
        "  border-radius: 5px;"
        "  padding: 8px;"
        "  font-family: Consolas;"
        "  font-size: 11pt;"
        "}"
    );

    m_securityQuestionLabel->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");
    m_securityQuestionLabel->setText("✓ AUTHENTICATION VERIFIED");

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
}

void SecurityQuestionLayer::securityQuestionTimeout() {
    // Po 10 sekundach pokazujemy podpowiedź
    m_securityQuestionLabel->setText("If this was really you, you would know the answer.\nYou don't need a question.");
}