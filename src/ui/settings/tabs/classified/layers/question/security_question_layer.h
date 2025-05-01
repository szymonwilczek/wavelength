#ifndef SECURITY_QUESTION_LAYER_H
#define SECURITY_QUESTION_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QTimer>

class SecurityQuestionLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit SecurityQuestionLayer(QWidget *parent = nullptr);
    ~SecurityQuestionLayer() override;

    void Initialize() override;
    void Reset() override;

    private slots:
        void CheckSecurityAnswer();
        void SecurityQuestionTimeout() const;

private:
    QLabel* security_question_label_;
    QLineEdit* security_question_input_;
    QTimer* security_question_timer_;
};

#endif // SECURITY_QUESTION_LAYER_H