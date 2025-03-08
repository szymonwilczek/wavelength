#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QIntValidator>
#include <QDebug>
#include <QMessageBox>

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
        formLayout->addRow("Wavelength Name (optional):", nameEdit);

        passwordProtectedCheckbox = new QCheckBox("Password Protected", this);
        formLayout->addRow("", passwordProtectedCheckbox);

        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setEnabled(false);
        passwordEdit->setStyleSheet("background-color: #3e3e3e; border: 1px solid #555; padding: 5px;");
        formLayout->addRow("Password:", passwordEdit);

        mainLayout->addLayout(formLayout);

        statusLabel = new QLabel(this);
        statusLabel->setStyleSheet("color: #ff5555; margin-top: 5px;");
        statusLabel->hide();
        mainLayout->addWidget(statusLabel);

        QHBoxLayout *buttonLayout = new QHBoxLayout();

        generateButton = new QPushButton("Generate Wavelength", this);
        generateButton->setEnabled(false);
        generateButton->setStyleSheet(
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

        buttonLayout->addWidget(cancelButton);
        buttonLayout->addWidget(generateButton);

        mainLayout->addStretch();
        mainLayout->addLayout(buttonLayout);

        connect(frequencyEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
        connect(passwordProtectedCheckbox, &QCheckBox::toggled, passwordEdit, &QLineEdit::setEnabled);
        connect(passwordProtectedCheckbox, &QCheckBox::toggled, this, &WavelengthDialog::validateInputs);
        connect(passwordEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
        connect(generateButton, &QPushButton::clicked, this, &WavelengthDialog::tryGenerate);
        connect(cancelButton, &QPushButton::clicked, this, &WavelengthDialog::reject);
    }

    int getFrequency() const {
        return frequencyEdit->text().toInt();
    }

    QString getName() const {
        return nameEdit->text();
    }

    bool isPasswordProtected() const {
        return passwordProtectedCheckbox->isChecked();
    }

    QString getPassword() const {
        return passwordEdit->text();
    }

private slots:
    void validateInputs() {
        bool valid = !frequencyEdit->text().isEmpty();

        if (passwordProtectedCheckbox->isChecked()) {
            valid = valid && !passwordEdit->text().isEmpty();
        }

        generateButton->setEnabled(valid);
        if (statusLabel->isVisible()) {
            statusLabel->hide();
        }
    }

    void tryGenerate() {
        int frequency = getFrequency();

        if (!checkFrequencyAvailable(frequency)) {
            statusLabel->setText(QString("Frequency %1Hz is already in use!").arg(frequency));
            statusLabel->show();
            return;
        }

        accept();
    }

    bool checkFrequencyAvailable(int frequency) {
        qDebug() << "Checking if frequency" << frequency << "is available...";

        return true;
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