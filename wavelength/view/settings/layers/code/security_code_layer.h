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
    // Struktura kodu zabezpieczającego
    struct SecurityCode {
        QString code;       // Kod jako string (może zawierać cyfry/litery)
        QString hint;       // Podpowiedź
        bool isNumeric;     // Czy kod jest tylko numeryczny

        SecurityCode(const QString& c, const QString& h, bool num = true)
            : code(c), hint(h), isNumeric(num) {}
    };

    QString getRandomSecurityCode(QString& hint, bool& isNumeric);
    void setupCodeInputs(bool isNumeric);
    void resetInputs();
    QString getEnteredCode() const;
    void showErrorEffect();
    void setInputValidators(bool numericOnly);

    QList<QLineEdit*> m_codeInputs;
    QLabel* m_securityCodeHint;
    QString m_currentSecurityCode;
    bool m_isCurrentCodeNumeric;

    // Baza kodów bezpieczeństwa z podpowiedziami
    QVector<SecurityCode> m_securityCodes;
};

#endif // SECURITY_CODE_LAYER_H