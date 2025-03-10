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
#include "session/wavelength_session_coordinator.h"

class WavelengthDialog : public QDialog {
    Q_OBJECT

public:
    explicit WavelengthDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Create New Wavelength");
        setModal(true);
        setFixedSize(400, 250);
        setStyleSheet("background-color: #2d2d2d; color: #e0e0e0;");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("System automatically assigns the lowest available frequency:", this);
        mainLayout->addWidget(infoLabel);

        QFormLayout *formLayout = new QFormLayout();

        // Pole wyświetlające częstotliwość (tylko do odczytu)
        frequencyLabel = new QLabel(this);
        frequencyLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #4a6db5;");
        formLayout->addRow("Assigned frequency:", frequencyLabel);

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

        connect(passwordProtectedCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            passwordEdit->setEnabled(checked);
            validateInputs();
        });
        connect(passwordEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
        connect(generateButton, &QPushButton::clicked, this, &WavelengthDialog::tryGenerate);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        // Znajdź najniższą dostępną częstotliwość i wyświetl ją
        findLowestAvailableFrequency();
        validateInputs();
    }

    double getFrequency() const {
        return m_frequency;
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

        bool isPasswordValid = true;
        if (passwordProtectedCheckbox->isChecked()) {
            isPasswordValid = !passwordEdit->text().isEmpty();
        }

        generateButton->setEnabled(isPasswordValid);
    }

    void tryGenerate() {
        static bool isGenerating = false;

        if (isGenerating) {
            qDebug() << "LOG: Blokada wielokrotnego wywołania tryGenerate()";
            return;
        }

        isGenerating = true;
        qDebug() << "LOG: tryGenerate - start";

        QString name = nameEdit->text().trimmed();
        bool isPasswordProtected = passwordProtectedCheckbox->isChecked();
        QString password = passwordEdit->text();

        qDebug() << "LOG: tryGenerate - walidacja pomyślna, akceptacja dialogu";
        QDialog::accept();

        isGenerating = false;
        qDebug() << "LOG: tryGenerate - koniec";
    }

private:
    void findLowestAvailableFrequency() {
        DatabaseManager* manager = DatabaseManager::getInstance();

        // Początek od 130Hz i szukamy do 1800MHz
        double frequency = 130;
        double maxFrequency = 180.0 * 1000000; // 180MHz w Hz

        while (frequency <= maxFrequency) {
            if (manager->isFrequencyAvailable(frequency)) {
                m_frequency = frequency;
                updateFrequencyLabel();
                return;
            }

            // Zwiększamy o 0.1 dla częstotliwości poniżej 1000Hz
            // Dla większych częstotliwości zwiększamy o większe skoki
            if (frequency < 1000) {
                frequency += 0.1;
            } else if (frequency < 10000) {
                frequency += 1.0;
            } else if (frequency < 100000) {
                frequency += 10.0;
            } else if (frequency < 1000000) {
                frequency += 100.0;
            } else {
                frequency += 1000.0;
            }
        }

        // Jeśli nie znaleziono, ustawiamy domyślną wartość
        m_frequency = 130.0;
        updateFrequencyLabel();
    }

    void updateFrequencyLabel() {
        QString unitText;
        double displayValue;

        if (m_frequency >= 1000000) {
            displayValue = m_frequency / 1000000.0;
            unitText = " MHz";
        } else if (m_frequency >= 1000) {
            displayValue = m_frequency / 1000.0;
            unitText = " kHz";
        } else {
            displayValue = m_frequency;
            unitText = " Hz";
        }

        // Formatuj z jednym miejscem po przecinku
        QString displayText = QString::number(displayValue, 'f', 1) + unitText;
        frequencyLabel->setText(displayText);
    }

private:
    QLabel *frequencyLabel;
    QLineEdit *nameEdit;
    QCheckBox *passwordProtectedCheckbox;
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *generateButton;
    QPushButton *cancelButton;
    double m_frequency = 130.0; // Domyślna wartość
};

#endif // WAVELENGTH_DIALOG_H