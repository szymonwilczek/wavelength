#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QApplication>
#include "wavelength_manager.h"

class WavelengthDialog : public QDialog {
    Q_OBJECT

public:
    explicit WavelengthDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Create New Wavelength");
        setModal(true);
        setFixedSize(400, 250);
        setStyleSheet("background-color: #2d2d2d; color: #e0e0e0;");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("Enter wavelength frequency (30Hz - 300Hz):", this);
        mainLayout->addWidget(infoLabel);

        QFormLayout *formLayout = new QFormLayout();

        frequencyEdit = new QLineEdit(this);
        frequencyEdit->setValidator(new QIntValidator(30, 300, this));
        frequencyEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        formLayout->addRow("Frequency (Hz):", frequencyEdit);

        nameEdit = new QLineEdit(this);
        nameEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        nameEdit->setPlaceholderText("Optional");
        formLayout->addRow("Name:", nameEdit);

        passwordProtectedCheckbox = new QCheckBox("Password Protected", this);
        passwordProtectedCheckbox->setStyleSheet(
            "QCheckBox { color: #e0e0e0; }"
            "QCheckBox::indicator { width: 16px; height: 16px; }"
            "QCheckBox::indicator:unchecked { background-color: #3e3e3e; border: 1px solid #555; }"
            "QCheckBox::indicator:checked { background-color: #3e3e3e; border: 1px solid #555; image: url(:/icons/check.png); }"
        );
        formLayout->addRow("", passwordProtectedCheckbox);

        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        passwordEdit->setEnabled(false);
        formLayout->addRow("Password:", passwordEdit);

        mainLayout->addLayout(formLayout);

        statusLabel = new QLabel(this);
        statusLabel->setStyleSheet("color: #ff5555; margin-top: 5px;");
        statusLabel->hide();
        mainLayout->addWidget(statusLabel);

        QHBoxLayout *buttonLayout = new QHBoxLayout();

        generateButton = new QPushButton("Create Wavelength", this);
        generateButton->setEnabled(false);
        generateButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #4a6db5;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "}"
            "QPushButton:hover { background-color: #5a7dc5; }"
            "QPushButton:pressed { background-color: #3a5da5; }"
            "QPushButton:disabled { background-color: #2c3e66; color: #aaaaaa; }"
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

        buttonLayout->addWidget(generateButton);
        buttonLayout->addWidget(cancelButton);
        mainLayout->addLayout(buttonLayout);

        connect(frequencyEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
        connect(nameEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
        connect(passwordProtectedCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            passwordEdit->setEnabled(checked);
            validateInputs();
        });
        connect(passwordEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
        connect(generateButton, &QPushButton::clicked, this, &WavelengthDialog::tryGenerate);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    int getFrequency() const {
        return frequencyEdit->text().toInt();
    }

    QString getName() const {
        return nameEdit->text().trimmed();
    }

    bool isPasswordProtected() const {
        return passwordProtectedCheckbox->isChecked();
    }

    QString getPassword() const {
        return passwordEdit->text();
    }

private slots:
    void validateInputs() {
        statusLabel->hide();

        bool isFrequencyValid = !frequencyEdit->text().isEmpty();

        bool isPasswordValid = true;
        if (passwordProtectedCheckbox->isChecked()) {
            isPasswordValid = !passwordEdit->text().isEmpty();
        }

        generateButton->setEnabled(isFrequencyValid && isPasswordValid);
    }


    void tryGenerate() {
        static bool isGenerating = false;

        if (isGenerating) {
            qDebug() << "LOG: Blokada wielokrotnego wywołania tryGenerate()";
            return;
        }

        isGenerating = true;
        qDebug() << "LOG: tryGenerate - start";

        // Tylko walidacja, bez tworzenia częstotliwości
        int frequency = frequencyEdit->text().toInt();
        QString name = nameEdit->text().trimmed();
        bool isPasswordProtected = passwordProtectedCheckbox->isChecked();
        QString password = passwordEdit->text();

        WavelengthManager* manager = WavelengthManager::getInstance();

        if (!manager->isFrequencyAvailable(frequency)) {
            qDebug() << "LOG: tryGenerate - częstotliwość niedostępna";
            statusLabel->setText("Frequency is already in use.");
            statusLabel->show();
            isGenerating = false;
            return;
        }

        qDebug() << "LOG: tryGenerate - walidacja pomyślna, akceptacja dialogu";
        QDialog::accept();

        isGenerating = false;
        qDebug() << "LOG: tryGenerate - koniec";
    }

    bool checkFrequencyAvailable(int frequency) {
        WavelengthManager* manager = WavelengthManager::getInstance();
        return manager->isFrequencyAvailable(frequency);
    }

private:
    QLineEdit *frequencyEdit;
    QLineEdit *nameEdit;
    QCheckBox *passwordProtectedCheckbox;
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *generateButton;
    QPushButton *cancelButton;
};

#endif // WAVELENGTH_DIALOG_H