#ifndef SECURITY_CODE_LAYER_H
#define SECURITY_CODE_LAYER_H

#include <QLabel>

#include "../security_layer.h"
#include <QLineEdit>
#include <QVector>
#include <QList>

class SecurityCodeLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit SecurityCodeLayer(QWidget *parent = nullptr);
    ~SecurityCodeLayer() override;

    void Initialize() override;
    void Reset() override;

    private slots:
        void CheckSecurityCode();
        void OnDigitEntered();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    struct SecurityCode {
        QString code;       // Kod jako string (może zawierać cyfry/litery)
        QString hint;       // Podpowiedź
        bool is_numeric;     // Czy kod jest tylko numeryczny

        SecurityCode(const QString& c, const QString& h, const bool num = true)
            : code(c), hint(h), is_numeric(num) {}
    };

    QString GetRandomSecurityCode(QString& hint, bool& is_numeric);
    void ResetInputs();
    QString GetEnteredCode() const;
    void ShowErrorEffect();
    void SetInputValidators(bool numeric_only);

    QList<QLineEdit*> code_inputs_;
    QLabel* security_code_hint_;
    QString current_security_code_;
    bool is_current_code_numeric_;

    QVector<SecurityCode> security_codes_;
};

#endif // SECURITY_CODE_LAYER_H