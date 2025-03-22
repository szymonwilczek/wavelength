#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPainter>
#include "../session/wavelength_session_coordinator.h"
#include "../ui/dialogs/animated_dialog.h"

class WavelengthDialog : public AnimatedDialog {
    Q_OBJECT

public:
    explicit WavelengthDialog(QWidget *parent = nullptr)
        : AnimatedDialog(parent, AnimatedDialog::SlideFromBottom),
          m_shadowSize(10)
    {
        setWindowTitle("Create New Wavelength");
        setModal(true);
        setFixedSize(450 + m_shadowSize, 250 + m_shadowSize);

        // Ustaw czas trwania animacji
        setAnimationDuration(400);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(m_shadowSize + 10, m_shadowSize + 10, m_shadowSize + 10, m_shadowSize + 10);

        QLabel *titleLabel = new QLabel("GENERATE WAVELENGTH", this);
        titleLabel->setStyleSheet("font-size: 15px; font-weight: bold;");
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignHCenter);
        QFont font = titleLabel->font();
        font.setStyleStrategy(QFont::PreferAntialias);
        titleLabel->setFont(font);
        titleLabel->setContentsMargins(0, 0, 0, 10);
        mainLayout->addWidget(titleLabel);

        QLabel *infoLabel = new QLabel("System automatically assigns the lowest available frequency", this);
        infoLabel->setStyleSheet("font-size: 14px; font-weight: semibold;");
        infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignHCenter);
        mainLayout->addWidget(infoLabel);

        QFormLayout *formLayout = new QFormLayout();

        // Pole wyświetlające częstotliwość (tylko do odczytu)
        frequencyLabel = new QLabel(this);
        frequencyLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #4a6db5;");
        formLayout->addRow("Assigned frequency:", frequencyLabel);

        // Wskaźnik ładowania przy wyszukiwaniu częstotliwości
        loadingIndicator = new QLabel("Searching for available frequency...", this);
        loadingIndicator->setStyleSheet("color: #ffcc00;");
        formLayout->addRow("", loadingIndicator);

        passwordProtectedCheckbox = new QCheckBox("Password Protected", this);
        formLayout->addRow("", passwordProtectedCheckbox);

        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setStyleSheet("background-color: #27272A; border: 1.5px solid #71717A; padding: 5px; border-radius: 8px;");
        passwordEdit->setEnabled(false);
        passwordEdit->setPlaceholderText("Enter Wavelength Password...");
        QFont passwordFont = passwordEdit->font();
        passwordFont.setFamily("Roboto");
        passwordFont.setPointSize(10);
        passwordFont.setStyleStrategy(QFont::PreferAntialias);
        passwordEdit->setFont(passwordFont);
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
        generateButton->setEnabled(false); // Disable until frequency is found

        cancelButton = new QPushButton("Cancel", this);
        cancelButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #441729;"
            "  color: #cf386e;"
            "  border-radius: 10%;"
            "  padding: 8px 16px;"
            "}"
            "QPushButton:hover { cursor: pointer; }"
            "QPushButton:pressed { background-color: #401827; }"
        );

        QFont buttonFont = generateButton->font();
        buttonFont.setFamily("Roboto");
        buttonFont.setPointSize(10);
        generateButton->setFont(buttonFont);
        cancelButton->setFont(buttonFont);

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

        // Inicjalizacja watcher'a dla asynchronicznego wyszukiwania
        frequencyWatcher = new QFutureWatcher<double>(this);
        connect(frequencyWatcher, &QFutureWatcher<double>::finished, this, &WavelengthDialog::onFrequencyFound);

        // Uruchom asynchroniczne wyszukiwanie
        QFuture<double> future = QtConcurrent::run(&WavelengthDialog::findLowestAvailableFrequency);
        frequencyWatcher->setFuture(future);

        frequencyLabel->setText("...");
    }

    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Parametry cienia
        const int cornerRadius = 15;
        const int shadowSteps = 15;

        // Rysowanie złożonego cienia z większą liczbą warstw
        for (int i = 0; i < shadowSteps; i++) {
            qreal ratio = (qreal)i / shadowSteps;
            qreal size = m_shadowSize * pow(ratio, 0.8);
            int alpha = 30 * pow(1.0 - ratio, 0.7);

            QColor shadowColor(20, 20, 20, alpha);
            painter.setPen(Qt::NoPen);
            painter.setBrush(shadowColor);

            QRect shadowRect = rect().adjusted(
                m_shadowSize - size,
                m_shadowSize - size,
                size - m_shadowSize,
                size - m_shadowSize
            );

            painter.drawRoundedRect(shadowRect, cornerRadius, cornerRadius);
        }

        // Główne tło dialogu
        QColor bgColor(24, 24, 27);
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        QRect mainRect = rect().adjusted(m_shadowSize, m_shadowSize, -m_shadowSize, -m_shadowSize);
        painter.drawRoundedRect(mainRect, cornerRadius, cornerRadius);
    }

    double getFrequency() const {
        return m_frequency;
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

        // Przycisk jest aktywny tylko jeśli znaleziono częstotliwość i hasło jest prawidłowe
        generateButton->setEnabled(isPasswordValid && m_frequencyFound);
    }

    void tryGenerate() {
        static bool isGenerating = false;

        if (isGenerating) {
            qDebug() << "LOG: Blokada wielokrotnego wywołania tryGenerate()";
            return;
        }

        isGenerating = true;
        qDebug() << "LOG: tryGenerate - start";

        bool isPasswordProtected = passwordProtectedCheckbox->isChecked();
        QString password = passwordEdit->text();

        qDebug() << "LOG: tryGenerate - walidacja pomyślna, akceptacja dialogu";
        QDialog::accept();

        isGenerating = false;
        qDebug() << "LOG: tryGenerate - koniec";
    }

    void onFrequencyFound() {
        // Pobierz wynik asynchronicznego wyszukiwania
        m_frequency = frequencyWatcher->result();
        m_frequencyFound = true;

        // Przygotuj tekst częstotliwości, ale jeszcze go nie wyświetlaj
        QString frequencyText = formatFrequencyText(m_frequency);

        // Przygotowanie animacji dla wskaźnika ładowania (znikanie)
        QPropertyAnimation *loaderSlideAnimation = new QPropertyAnimation(loadingIndicator, "maximumHeight");
        loaderSlideAnimation->setDuration(400);
        loaderSlideAnimation->setStartValue(loadingIndicator->sizeHint().height());
        loaderSlideAnimation->setEndValue(0);
        loaderSlideAnimation->setEasingCurve(QEasingCurve::OutQuint);

        // Dodanie efektu przezroczystości dla etykiety częstotliwości
        QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(frequencyLabel);
        frequencyLabel->setGraphicsEffect(opacityEffect);
        opacityEffect->setOpacity(0.0); // Początkowo niewidoczny

        // Animacja pojawiania się etykiety częstotliwości
        QPropertyAnimation *frequencyAnimation = new QPropertyAnimation(opacityEffect, "opacity");
        frequencyAnimation->setDuration(600);
        frequencyAnimation->setStartValue(0.0);
        frequencyAnimation->setEndValue(1.0);
        frequencyAnimation->setEasingCurve(QEasingCurve::InQuad);

        // Utwórz grupę animacji, która zostanie uruchomiona sekwencyjnie
        QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
        animGroup->addAnimation(loaderSlideAnimation);

        // Sygnalizacja kiedy pierwsza animacja się kończy
        connect(loaderSlideAnimation, &QPropertyAnimation::finished, [this, frequencyText]() {
            // Ukryj loadingIndicator
            loadingIndicator->hide();

            // Ustaw tekst częstotliwości
            frequencyLabel->setText(frequencyText);
        });

        // Połączenie sekwencyjne - druga animacja po zakończeniu pierwszej
        connect(loaderSlideAnimation, &QPropertyAnimation::finished, [this, frequencyAnimation]() {
            frequencyAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        });

        // Dla większej płynności animacji
        loadingIndicator->setAttribute(Qt::WA_OpaquePaintEvent, false);
        frequencyLabel->setAttribute(Qt::WA_OpaquePaintEvent, false);

        frequencyAnimation->setKeyValueAt(0.2, 0.3);
        frequencyAnimation->setKeyValueAt(0.4, 0.6);
        frequencyAnimation->setKeyValueAt(0.6, 0.8);
        frequencyAnimation->setKeyValueAt(0.8, 0.95);

        // Rozpocznij pierwszą animację
        animGroup->start(QAbstractAnimation::DeleteWhenStopped);

        // Sprawdź czy przycisk powinien być aktywowany
        validateInputs();
    }

    // Nowa pomocnicza metoda do formatowania tekstu częstotliwości
    QString formatFrequencyText(double frequency) {
        QString unitText;
        double displayValue;

        if (frequency >= 1000000) {
            displayValue = frequency / 1000000.0;
            unitText = " MHz";
        } else if (frequency >= 1000) {
            displayValue = frequency / 1000.0;
            unitText = " kHz";
        } else {
            displayValue = frequency;
            unitText = " Hz";
        }

        // Formatuj z jednym miejscem po przecinku
        return QString::number(displayValue, 'f', 1) + unitText;
    }

private:
    static double findLowestAvailableFrequency() {
        DatabaseManager* manager = DatabaseManager::getInstance();

        // Początek od 130Hz i szukamy do 1800MHz
        double frequency = 130;
        double maxFrequency = 180.0 * 1000000; // 180MHz w Hz

        while (frequency <= maxFrequency) {
            if (manager->isFrequencyAvailable(frequency)) {
                return frequency;
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
        return 130.0;
    }

private:
    QLabel *frequencyLabel;
    QLabel *loadingIndicator;
    QCheckBox *passwordProtectedCheckbox;
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *generateButton;
    QPushButton *cancelButton;
    QFutureWatcher<double> *frequencyWatcher;
    double m_frequency = 130.0; // Domyślna wartość
    bool m_frequencyFound = false; // Flaga oznaczająca znalezienie częstotliwości
    const int m_shadowSize; // Rozmiar cienia
};

#endif // WAVELENGTH_DIALOG_H