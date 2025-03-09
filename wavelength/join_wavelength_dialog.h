#ifndef JOIN_WAVELENGTH_DIALOG_H
#define JOIN_WAVELENGTH_DIALOG_H

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QIntValidator>
#include "wavelength_manager.h"

class JoinWavelengthDialog : public QDialog {
    Q_OBJECT

public:
    explicit JoinWavelengthDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Join Wavelength");
        setModal(true);
        setFixedSize(400, 200);
        setStyleSheet("background-color: #2d2d2d; color: #e0e0e0;");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("Enter wavelength frequency to join (30Hz - 300Hz):", this);
        mainLayout->addWidget(infoLabel);

        QFormLayout *formLayout = new QFormLayout();

        frequencyEdit = new QLineEdit(this);
        frequencyEdit->setValidator(new QIntValidator(30, 300, this));
        frequencyEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        formLayout->addRow("Frequency (Hz):", frequencyEdit);

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

        WavelengthManager* manager = WavelengthManager::getInstance();
        connect(manager, &WavelengthManager::wavelengthJoined, this, &JoinWavelengthDialog::accept);
        connect(manager, &WavelengthManager::authenticationFailed, this, &JoinWavelengthDialog::onAuthFailed);
        connect(manager, &WavelengthManager::connectionError, this, &JoinWavelengthDialog::onConnectionError);

        validateInput();
    }

    int getFrequency() const {
        return frequencyEdit->text().toInt();
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
        int frequency = frequencyEdit->text().toInt();
        QString password = passwordEdit->text();

        statusLabel->hide();

        if (frequency < 30 || frequency > 300) {
            statusLabel->setText("Frequency must be between 30Hz and 300Hz");
            statusLabel->show();
            return;
        }

        WavelengthManager* manager = WavelengthManager::getInstance();

        QApplication::setOverrideCursor(Qt::WaitCursor);
        bool success = manager->joinWavelength(frequency, password);
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
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *joinButton;
    QPushButton *cancelButton;
};

#endif // JOIN_WAVELENGTH_DIALOG_H