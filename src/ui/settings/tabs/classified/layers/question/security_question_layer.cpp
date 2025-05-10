#include "security_question_layer.h"
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>

#include "../../../../../../app/managers/translation_manager.h"

SecurityQuestionLayer::SecurityQuestionLayer(QWidget *parent)
    : SecurityLayer(parent), security_question_timer_(nullptr) {
    translator_ = TranslationManager::GetInstance();
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    const auto title = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.SecurityQuestion.Title", "SECURITY QUESTION VERIFICATION"),
        this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    const auto instructions = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.SecurityQuestion.Info", "Answer your security question."),
        this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    security_question_label_ = new QLabel("", this);
    security_question_label_->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    security_question_label_->setAlignment(Qt::AlignCenter);
    security_question_label_->setWordWrap(true);
    security_question_label_->setFixedWidth(400);

    security_question_input_ = new QLineEdit(this);
    security_question_input_->setFixedWidth(300);
    security_question_input_->setStyleSheet(
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
    layout->addWidget(security_question_label_);
    layout->addSpacing(20);
    layout->addWidget(security_question_input_, 0, Qt::AlignCenter);
    layout->addStretch();

    security_question_timer_ = new QTimer(this);
    security_question_timer_->setSingleShot(true);
    security_question_timer_->setInterval(10000);
    connect(security_question_timer_, &QTimer::timeout, this, &SecurityQuestionLayer::SecurityQuestionTimeout);

    connect(security_question_input_, &QLineEdit::returnPressed, this, &SecurityQuestionLayer::CheckSecurityAnswer);
}

SecurityQuestionLayer::~SecurityQuestionLayer() {
    if (security_question_timer_) {
        security_question_timer_->stop();
        delete security_question_timer_;
        security_question_timer_ = nullptr;
    }
}

void SecurityQuestionLayer::Initialize() {
    Reset();

    security_question_label_->setText(translator_->Translate("ClassifiedSettingsWidget.SecurityQuestion.Label",
                                                             "What is your top secret security question?"));
    security_question_input_->setFocus();
    security_question_timer_->start();

    if (graphicsEffect()) {
        static_cast<QGraphicsOpacityEffect *>(graphicsEffect())->setOpacity(1.0);
    }
}

void SecurityQuestionLayer::Reset() {
    if (security_question_timer_ && security_question_timer_->isActive()) {
        security_question_timer_->stop();
    }
    security_question_input_->clear();
    security_question_label_->setText("");

    security_question_input_->setStyleSheet(
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
    security_question_label_->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    security_question_input_->setReadOnly(false);

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect *>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void SecurityQuestionLayer::CheckSecurityAnswer() {
    security_question_input_->setStyleSheet(
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

    security_question_label_->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");
    security_question_label_->setText(
        translator_->Translate("ClassifiedSettingsWidget.SecurityQuestion.Verified", "✓ AUTHENTICATION VERIFIED"));

    QTimer::singleShot(800, this, [this]() {
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

void SecurityQuestionLayer::SecurityQuestionTimeout() const {
    security_question_label_->setText(
        translator_->Translate("ClassifiedSettingsWidget.SecurityQuestion.Timeout",
                               "If this was really you, you would know the answer.\nYou don't need a question."));
}
