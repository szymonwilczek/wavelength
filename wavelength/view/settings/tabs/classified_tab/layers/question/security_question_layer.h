#ifndef SECURITY_QUESTION_LAYER_H
#define SECURITY_QUESTION_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QTimer>

class SecurityQuestionLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit SecurityQuestionLayer(QWidget *parent = nullptr);
    ~SecurityQuestionLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void checkSecurityAnswer();
    void securityQuestionTimeout();

private:
    QLabel* m_securityQuestionLabel;
    QLineEdit* m_securityQuestionInput;
    QTimer* m_securityQuestionTimer;
};

#endif // SECURITY_QUESTION_LAYER_H