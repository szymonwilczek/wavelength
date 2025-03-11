#ifndef JOIN_WAVELENGTH_DIALOG_H
#define JOIN_WAVELENGTH_DIALOG_H

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include "../session/wavelength_session_coordinator.h"

class JoinWavelengthDialog : public QDialog {
    Q_OBJECT

public:
    explicit JoinWavelengthDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Join Wavelength");
        setModal(true);
        setFixedSize(400, 250); // Zwiększona wysokość dla dodatkowej etykiety
        setStyleSheet("background-color: #2d2d2d; color: #e0e0e0;");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("Enter wavelength frequency to join (130Hz - 180MHz):", this);
        mainLayout->addWidget(infoLabel);

        QFormLayout *formLayout = new QFormLayout();

        // Układ dla pola częstotliwości i selecta jednostki
        QHBoxLayout *freqLayout = new QHBoxLayout();

        frequencyEdit = new QLineEdit(this);
        // Walidator dla liczb zmiennoprzecinkowych (min, max, precyzja)
        QDoubleValidator* validator = new QDoubleValidator(130, 180000000.0, 1, this);
        QLocale locale(QLocale::English);
        locale.setNumberOptions(QLocale::RejectGroupSeparator);
        validator->setLocale(locale);
        frequencyEdit->setValidator(validator);
        frequencyEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        freqLayout->addWidget(frequencyEdit);

        frequencyUnitCombo = new QComboBox(this);
        frequencyUnitCombo->addItem("Hz");
        frequencyUnitCombo->addItem("kHz");
        frequencyUnitCombo->addItem("MHz");
        frequencyUnitCombo->setStyleSheet(
            "QComboBox {"
            "  background-color: #3e3e3e;"
            "  border: 1px solid #555;"
            "  padding: 5px;"
            "  min-width: 70px;"
            "}"
            "QComboBox::drop-down {"
            "  subcontrol-origin: padding;"
            "  subcontrol-position: top right;"
            "  width: 20px;"
            "  border-left-width: 1px;"
            "  border-left-color: #555;"
            "  border-left-style: solid;"
            "}"
        );
        freqLayout->addWidget(frequencyUnitCombo);

        formLayout->addRow("Frequency:", freqLayout);

        // Dodaj etykietę z informacją o używaniu kropki
        QLabel *decimalHintLabel = new QLabel("Use dot (.) as decimal separator (e.g. 98.7)", this);
        decimalHintLabel->setStyleSheet("color: #aaaaaa; font-size: 11px; padding-left: 5px;");
        formLayout->addRow("", decimalHintLabel);

        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        formLayout->addRow("Password (if required):", passwordEdit);

        mainLayout->addLayout(formLayout);

        statusLabel = new QLabel(this);
        statusLabel->setStyleSheet("color: #ff5555; margin-top: 5px;");
        statusLabel->hide();
        mainLayout->addWidget(statusLabel);

        QHBoxLayout *buttonLayout = new QHBoxLayout();

        joinButton = new QPushButton("Join Wavelength", this);
        joinButton->setEnabled(false);
        joinButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #3e3e3e;"
            "  color: #e0e0e0;"
            "  border: 1px solid #4a4a4a;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "}"
            "QPushButton:hover { background-color: #4a4a4a; }"
            "QPushButton:pressed { background-color: #2e2e2e; }"
            "QPushButton:disabled { background-color: #2a2a2a; color: #555555; }"
        );

        cancelButton = new QPushButton("Cancel", this);
        cancelButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #3e3e3e;"
            "  color: #e0e0e0;"
            "  border: 1px solid #4a4a4a;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "}"
            "QPushButton:hover { background-color: #4a4a4a; }"
            "QPushButton:pressed { background-color: #2e2e2e; }"
        );

        buttonLayout->addWidget(joinButton);
        buttonLayout->addWidget(cancelButton);
        mainLayout->addLayout(buttonLayout);

        connect(frequencyEdit, &QLineEdit::textChanged, this, &JoinWavelengthDialog::validateInput);
        connect(passwordEdit, &QLineEdit::textChanged, this, &JoinWavelengthDialog::validateInput);
        connect(joinButton, &QPushButton::clicked, this, &JoinWavelengthDialog::tryJoin);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        WavelengthJoiner* joiner = WavelengthJoiner::getInstance();
        connect(joiner, &WavelengthJoiner::wavelengthJoined, this, &JoinWavelengthDialog::accept);
        connect(joiner, &WavelengthJoiner::authenticationFailed, this, &JoinWavelengthDialog::onAuthFailed);
        connect(joiner, &WavelengthJoiner::connectionError, this, &JoinWavelengthDialog::onConnectionError);

        validateInput();
    }

    double getFrequency() const {
        double value = frequencyEdit->text().toDouble();

        // Przelicz wartość na Hz w zależności od wybranej jednostki
        if (frequencyUnitCombo->currentText() == "kHz") {
            value *= 1000.0;
        } else if (frequencyUnitCombo->currentText() == "MHz") {
            value *= 1000000.0;
        }

        return value;
    }

    QString getPassword() const {
        return passwordEdit->text();
    }

private slots:
    void validateInput() {
        if (statusLabel->isVisible()) {
            statusLabel->hide();
        }

        bool isValid = !frequencyEdit->text().isEmpty();
        joinButton->setEnabled(isValid);
    }

    void tryJoin() {
        double frequency = getFrequency();
        QString password = passwordEdit->text();

        statusLabel->hide();

        if (frequency < 130 || frequency > 180000000.0) {
            statusLabel->setText("Frequency must be between 130Hz and 180MHz");
            statusLabel->show();
            return;
        }

        WavelengthJoiner* joiner = WavelengthJoiner::getInstance();

        QApplication::setOverrideCursor(Qt::WaitCursor);
        bool success = joiner->joinWavelength(frequency, password);
        QApplication::restoreOverrideCursor();

        if (!success) {
            statusLabel->setText("Failed to join wavelength. It may not exist or server is unavailable.");
            statusLabel->show();
        }
    }

    void onAuthFailed(int frequency) {
        statusLabel->setText("Authentication failed. Incorrect password.");
        statusLabel->show();
    }

    void onConnectionError(const QString& errorMessage) {
        statusLabel->setText("Connection error: " + errorMessage);
        statusLabel->show();
    }

private:
    QLineEdit *frequencyEdit;
    QComboBox *frequencyUnitCombo;
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *joinButton;
    QPushButton *cancelButton;
};

#endif // JOIN_WAVELENGTH_DIALOG_H