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
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QShowEvent>  // Dodany nagłówek
#include <QCloseEvent> // Dodany nagłówek
#include <QPainter>
#include "../session/wavelength_session_coordinator.h"

class WavelengthDialog : public QDialog {
    Q_OBJECT

public:
    explicit WavelengthDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setWindowTitle("Create New Wavelength");
        setModal(true);
        setFixedSize(400, 250);
        setAttribute(Qt::WA_TranslucentBackground); // Kluczowe dla zaokrąglonych rogów

        setAutoFillBackground(false);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel("System automatically assigns the lowest available frequency:", this);
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
        generateButton->setEnabled(false); // Disable until frequency is found

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

        // Inicjalizacja watcher'a dla asynchronicznego wyszukiwania
        frequencyWatcher = new QFutureWatcher<double>(this);
        connect(frequencyWatcher, &QFutureWatcher<double>::finished, this, &WavelengthDialog::onFrequencyFound);

        // Uruchom asynchroniczne wyszukiwanie
        QFuture<double> future = QtConcurrent::run(&WavelengthDialog::findLowestAvailableFrequency);
        frequencyWatcher->setFuture(future);

        frequencyLabel->setText("...");
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

protected:

    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor bgColor(45, 45, 45); // #2d2d2d
        painter.setBrush(bgColor);

        QPen pen(QColor(74, 74, 74), 1); // #4a4a4a, 1px szerokości
        painter.setPen(pen);

        painter.drawRoundedRect(rect(), 10, 10);

    }

    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);

        // Pobierz aktualną pozycję finalną dialogu - to jest docelowa geometria
        QRect endGeometry = geometry();

        // Znajdź rzeczywisty środek okna
        QPoint centerPoint = endGeometry.center();

        // Utwórz początkową geometrię jako mały punkt dokładnie w środku
        int smallSize = 20; // Bardzo mały rozmiar początkowy
        QRect startGeometry(0, 0, smallSize, smallSize);
        startGeometry.moveCenter(centerPoint);

        setGeometry(startGeometry);

        layout()->activate();
        repaint();

        // Utwórz animację
        QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
        animation->setDuration(500);
        animation->setStartValue(startGeometry);
        animation->setEndValue(endGeometry);
        animation->setEasingCurve(QEasingCurve::OutExpo);

        // Uruchom animację
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

void closeEvent(QCloseEvent *event) override {
    event->ignore();

    QRect startGeometry = geometry();
    QPoint centerPoint = startGeometry.center();
    QRect endGeometry = QRect(0, 0, startGeometry.width() * 0.1, startGeometry.height() * 0.1);
    endGeometry.moveCenter(centerPoint);

    QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(400);  // Długość animacji
    animation->setStartValue(startGeometry);
    animation->setEndValue(endGeometry);
    animation->setEasingCurve(QEasingCurve::InBack);

    // Dodatkowe punkty pośrednie dla płynniejszego efektu
    animation->setKeyValueAt(0.5, QRect(
        centerPoint.x() - startGeometry.width() * 0.35,
        centerPoint.y() - startGeometry.height() * 0.35,
        startGeometry.width() * 0.7,
        startGeometry.height() * 0.7
    ));

    connect(animation, &QPropertyAnimation::finished, this, &QDialog::reject);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
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
    loaderSlideAnimation->setDuration(400); // Skrócony czas dla lepszej responsywności
    loaderSlideAnimation->setStartValue(loadingIndicator->sizeHint().height());
    loaderSlideAnimation->setEndValue(0);
    loaderSlideAnimation->setEasingCurve(QEasingCurve::OutQuint); // Płynniejsza krzywa

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

    // Aby uzyskać wysoką płynność animacji:
    // 1. Wyłączamy cache tła dla animowanych widżetów
    loadingIndicator->setAttribute(Qt::WA_OpaquePaintEvent, false);
    frequencyLabel->setAttribute(Qt::WA_OpaquePaintEvent, false);

    // 2. Ustaw wyższą częstotliwość aktualizacji dla animacji
    frequencyAnimation->setDuration(600);  // 600ms
    loaderSlideAnimation->setDuration(400);  // 400ms

    // Dla większej płynności możemy ustawić więcej klatek animacji
    const int framesPerSecond = 60;
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

    void updateFrequencyLabel() {
        frequencyLabel->setText(formatFrequencyText(m_frequency));
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
};

#endif // WAVELENGTH_DIALOG_H