#ifndef SECURITY_CODE_LAYER_H
#define SECURITY_CODE_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QPair>
#include <QVector>
#include <QList>

class SecurityCodeLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit SecurityCodeLayer(QWidget *parent = nullptr);
    ~SecurityCodeLayer() override;

    void initialize() override;
    void reset() override;


    private slots:
        void checkSecurityCode();
        void onDigitEntered();


protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    int getRandomSecurityCode(QString& hint);
    void setupCodeInputs();
    void resetInputs();
    QString getEnteredCode() const;
    void showErrorEffect();

    QList<QLineEdit*> m_codeInputs;
    QLabel* m_securityCodeHint;
    int m_currentSecurityCode;

    // Baza kodów bezpieczeństwa z podpowiedziami
    QVector<QPair<int, QString>> m_securityCodes;
};

#endif // SECURITY_CODE_LAYER_H