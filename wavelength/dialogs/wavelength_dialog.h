#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QSurfaceFormat>

#include "../session/wavelength_session_coordinator.h"
#include "../ui/dialogs/animated_dialog.h"
#include "../ui/button/cyber_button.h"
#include "../ui/checkbox/cyber_checkbox.h"
#include "../ui/input/cyber_line_edit.h"

class WavelengthDialog : public AnimatedDialog {
    Q_OBJECT
     Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
     Q_PROPERTY(double digitalizationProgress READ digitalizationProgress WRITE setDigitalizationProgress)
     Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)
     Q_PROPERTY(double glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)

public:
    explicit WavelengthDialog(QWidget *parent = nullptr)
    : AnimatedDialog(parent, AnimatedDialog::DigitalMaterialization),
      m_shadowSize(10), m_scanlineOpacity(0.08)
{

        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_NoSystemBackground, true);

        // Preferuj animacje przez GPU
        setAttribute(Qt::WA_TranslucentBackground);

        setAttribute(Qt::WA_StaticContents, true);  // Optymalizuje częściowe aktualizacje
        setAttribute(Qt::WA_ContentsPropagated, false);  // Zapobiega propagacji zawartości

        // QSurfaceFormat format;
        // format.setRenderableType(QSurfaceFormat::OpenGL);
        // QSurfaceFormat::setDefaultFormat(format);

        m_glitchLines = QList<int>();
        m_digitalizationProgress = 0.0;
        m_animationStarted = false;
        
    setWindowTitle("CREATE_WAVELENGTH::NEW_INSTANCE");
    setModal(true);
    setFixedSize(450, 350);

    setAnimationDuration(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // Nagłówek z tytułem
    QLabel *titleLabel = new QLabel("GENERATE WAVELENGTH", this);
    titleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    titleLabel->setContentsMargins(0, 0, 0, 3);
    mainLayout->addWidget(titleLabel);

    // Panel informacyjny z ID sesji
    QWidget *infoPanel = new QWidget(this);
    QHBoxLayout *infoPanelLayout = new QHBoxLayout(infoPanel);
    infoPanelLayout->setContentsMargins(0, 0, 0, 0);
    infoPanelLayout->setSpacing(5);

    QString sessionId = QString("%1-%2")
        .arg(QRandomGenerator::global()->bounded(1000, 9999))
        .arg(QRandomGenerator::global()->bounded(10000, 99999));
    QLabel *sessionLabel = new QLabel(QString("SESSION_ID: %1").arg(sessionId), this);
    sessionLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QLabel *timeLabel = new QLabel(QString("TS: %1").arg(timestamp), this);
    timeLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    infoPanelLayout->addWidget(sessionLabel);
    infoPanelLayout->addStretch();
    infoPanelLayout->addWidget(timeLabel);
    mainLayout->addWidget(infoPanel);

    // Panel instrukcji - poprawka dla responsywności
    QLabel *infoLabel = new QLabel("System automatically assigns the lowest available frequency", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    infoLabel->setAlignment(Qt::AlignLeft);
    infoLabel->setWordWrap(true); // Włącz zawijanie tekstu
    mainLayout->addWidget(infoLabel);

    // Uprościliśmy panel formularza - bez dodatkowego kontenera
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setSpacing(12); // Zwiększ odstępy
        formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // Wyśrodkuj w pionie
        formLayout->setContentsMargins(0, 15, 0, 15); // Dodaj więcej przestrzeni w pionie

    // Etykiety w formularzu
    QLabel* frequencyTitleLabel = new QLabel("ASSIGNED FREQUENCY:", this);
    frequencyTitleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    // Pole wyświetlające częstotliwość
    frequencyLabel = new QLabel(this);
    frequencyLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 16pt;");
    formLayout->addRow(frequencyTitleLabel, frequencyLabel);

    // Wskaźnik ładowania przy wyszukiwaniu częstotliwości
    loadingIndicator = new QLabel("SEARCHING FOR AVAILABLE FREQUENCY...", this);
    loadingIndicator->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    formLayout->addRow("", loadingIndicator);

    // Checkbox zabezpieczenia hasłem
    passwordProtectedCheckbox = new CyberCheckBox("PASSWORD PROTECTED", this);
    // Aktualizacja stylów dla checkboxa
    formLayout->addRow("", passwordProtectedCheckbox);

    // Etykieta i pole hasła
    QLabel* passwordLabel = new QLabel("PASSWORD:", this);
    passwordLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    passwordEdit = new CyberLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setEnabled(false);
    passwordEdit->setPlaceholderText("ENTER WAVELENGTH PASSWORD");
    passwordEdit->setStyleSheet("font-family: Consolas; font-size: 9pt;");
        passwordEdit->setFixedHeight(30); // Wymuszenie wysokości 30px
        passwordEdit->setVisible(true);   // Upewnij się, że jest widoczne
    formLayout->addRow(passwordLabel, passwordEdit);

        if (!passwordProtectedCheckbox->isChecked()) {
            passwordEdit->setStyleSheet("border: none; background-color: transparent; padding: 6px; font-family: Consolas; font-size: 9pt; color: #005577;");
        }

    mainLayout->addLayout(formLayout);

    // Etykieta statusu
    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet("color: #ff3355; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    statusLabel->hide(); // Ukryj na początku
    mainLayout->addWidget(statusLabel);

    // Panel przycisków
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);

    generateButton = new CyberButton("CREATE WAVELENGTH", this, true);
    generateButton->setFixedHeight(35);
    generateButton->setEnabled(false); // Disable until frequency is found

    cancelButton = new CyberButton("CANCEL", this, false);
    cancelButton->setFixedHeight(35);

    buttonLayout->addWidget(generateButton, 2);
    buttonLayout->addWidget(cancelButton, 1);
    mainLayout->addLayout(buttonLayout);

    // Połączenia sygnałów i slotów
        connect(passwordProtectedCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        passwordEdit->setEnabled(checked);

        // Zmiana wyglądu pola w zależności od stanu
        if (checked) {
            passwordEdit->setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt; color: #00ccff;");
        } else {
            passwordEdit->setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt; color: #005577;");
        }

        // Ukrywamy etykietę statusu przy zmianie stanu checkboxa
        statusLabel->hide();
        validateInputs();
    });
    connect(passwordEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
    connect(generateButton, &QPushButton::clicked, this, &WavelengthDialog::tryGenerate);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        frequencyWatcher = new QFutureWatcher<double>(this);
        connect(frequencyWatcher, &QFutureWatcher<double>::finished, this, &WavelengthDialog::onFrequencyFound);

        // Ustawiamy text w etykiecie częstotliwości na domyślną wartość
        frequencyLabel->setText("...");

        // Podłączamy sygnał zakończenia animacji do rozpoczęcia wyszukiwania częstotliwości
        connect(this, &AnimatedDialog::showAnimationFinished,
                this, &WavelengthDialog::startFrequencySearch);

        // Inicjuj timer odświeżania
        m_refreshTimer = new QTimer(this);
        m_refreshTimer->setInterval(16); // ~60fps dla płynności animacji
        connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
    if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0) {
        int scanLineY = height() * m_digitalizationProgress;
        int clipSize = 20;

        if (scanLineY != m_lastScanlineY) {
            // Odśwież tylko obszar wokół aktualnej i poprzedniej pozycji linii skanującej
            int minY = qMin(scanLineY, m_lastScanlineY) - 15;
            int maxY = qMax(scanLineY, m_lastScanlineY) + 15;

            // Ograniczenie zakresu do widocznego obszaru
            minY = qMax(0, minY);
            maxY = qMin(height(), maxY);

            update(0, minY, width(), maxY - minY);
        }
    }
    else if (m_glitchIntensity > 0.01) {
        for (int i = 0; i < m_glitchLines.size(); i++) {
            int y = m_glitchLines[i];
            update(0, y-5, width(), 10);
        }
    }
    else {
        m_refreshTimer->setInterval(33); // Mniej intensywne odświeżanie gdy nie ma animacji
    }
});
}

    ~WavelengthDialog()  override {
        if (m_refreshTimer) {
            m_refreshTimer->stop();
            delete m_refreshTimer;
            m_refreshTimer = nullptr;
        }
    }

    double digitalizationProgress() const { return m_digitalizationProgress; }
    void setDigitalizationProgress(double progress) {
        if (!m_animationStarted && progress > 0.01)
            m_animationStarted = true;
        m_digitalizationProgress = progress;
        update();
    }

    double cornerGlowProgress() const { return m_cornerGlowProgress; }
    void setCornerGlowProgress(double progress) {
        m_cornerGlowProgress = progress;
        update();
    }

    double glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(double intensity) {
        m_glitchIntensity = intensity;
        update();
        if (intensity > 0.4) regenerateGlitchLines();
    }

    void regenerateGlitchLines() {
        // Nie generuj linii glitch zbyt często - najwyżej co 100ms
        static QElapsedTimer lastGlitchUpdate;
        if (!lastGlitchUpdate.isValid() || lastGlitchUpdate.elapsed() > 100) {
            m_glitchLines.clear();
            int glitchCount = 3 + static_cast<int>(glitchIntensity() * 7); // Mniej linii dla wydajności
            for (int i = 0; i < glitchCount; i++) {
                m_glitchLines.append(QRandomGenerator::global()->bounded(height()));
            }
            lastGlitchUpdate.restart();
        }
    }

    // Akcesory dla animacji scanlines
    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    QTimer* getRefreshTimer() const { return m_refreshTimer; }
    void startRefreshTimer() {
        if (m_refreshTimer) {
            m_refreshTimer->start();
        }
    }
    void stopRefreshTimer() {
        if (m_refreshTimer) {
            m_refreshTimer->stop();
        }
    }
    void setRefreshTimerInterval(int interval) {
        if (m_refreshTimer) {
            m_refreshTimer->setInterval(interval);
        }
    }

    void WavelengthDialog::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Główne tło dialogu z gradientem
    QLinearGradient bgGradient(0, 0, 0, height());
    bgGradient.setColorAt(0, QColor(10, 21, 32));
    bgGradient.setColorAt(1, QColor(7, 18, 24));

    // Ścieżka dialogu ze ściętymi rogami
    QPainterPath dialogPath;
    int clipSize = 20; // rozmiar ścięcia

    // Górna krawędź
    dialogPath.moveTo(clipSize, 0);
    dialogPath.lineTo(width() - clipSize, 0);

    // Prawy górny róg
    dialogPath.lineTo(width(), clipSize);

    // Prawa krawędź
    dialogPath.lineTo(width(), height() - clipSize);

    // Prawy dolny róg
    dialogPath.lineTo(width() - clipSize, height());

    // Dolna krawędź
    dialogPath.lineTo(clipSize, height());

    // Lewy dolny róg
    dialogPath.lineTo(0, height() - clipSize);

    // Lewa krawędź
    dialogPath.lineTo(0, clipSize);

    // Lewy górny róg
    dialogPath.lineTo(clipSize, 0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgGradient);
    painter.drawPath(dialogPath);

    // Obramowanie dialogu
    QColor borderColor(0, 195, 255, 150);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(dialogPath);

        if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0 && m_animationStarted) {
            // Inicjalizacja buforów
            initRenderBuffers();

            int scanLineY = static_cast<int>(height() * m_digitalizationProgress);
            int clipSize = 20;

            if (scanLineY != m_lastScanlineY) {
                painter.setClipping(false);
                QPainter::CompositionMode previousMode = painter.compositionMode();
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

                // Rysuj wszystkie linie poziome od góry do aktualnej pozycji
                painter.drawPixmap(0, 0, m_scanLinesBuffer, 0, 0, width(), scanLineY);

                // Oblicz szerokość linii skanującej na danej wysokości
                int startX = 0;
                int endX = width();

                // Górna część dialogu
                if (scanLineY < clipSize) {
                    float ratio = (float)scanLineY / clipSize;
                    startX = clipSize * (1.0f - ratio);
                    endX = width() - clipSize * (1.0f - ratio);
                }
                // Dolna część dialogu
                else if (scanLineY > height() - clipSize) {
                    float ratio = (float)(height() - scanLineY) / clipSize;
                    startX = clipSize * (1.0f - ratio);
                    endX = width() - clipSize * (1.0f - ratio);
                }

                // Rysuj linię skanującą z uwzględnieniem szerokości w danym punkcie Y
                int scanWidth = endX - startX;
                if (scanWidth > 0) {
                    painter.drawPixmap(startX, scanLineY - 10, scanWidth, 20,
                                      m_scanlineBuffer,
                                      (width() - scanWidth) / 2, 0, scanWidth, 20);
                }

                painter.setCompositionMode(previousMode);
                m_lastScanlineY = scanLineY;
            }
        }

    // NOWY KOD - efekt glitch
    if (m_glitchIntensity > 0.01) {
        // Glitch na brzegach
        painter.setPen(QPen(QColor(255, 50, 120, 150 * m_glitchIntensity), 1));

        for (int i = 0; i < m_glitchLines.size(); i++) {
            int y = m_glitchLines[i];
            int offset = QRandomGenerator::global()->bounded(5, 15) * m_glitchIntensity;
            int width = QRandomGenerator::global()->bounded(30, 80) * m_glitchIntensity;

            // Losowe położenie glitchy
            if (QRandomGenerator::global()->bounded(2) == 0) {
                // Lewa strona
                painter.drawLine(0, y, width, y + offset);
            } else {
                // Prawa strona
                painter.drawLine(this->width() - width, y, this->width(), y + offset);
            }
        }
    }

    // NOWY KOD - sekwencyjne podświetlanie rogów
    if (m_cornerGlowProgress > 0.0) {
        QColor cornerColor(0, 220, 255, 150);
        painter.setPen(QPen(cornerColor, 2));

        // Obliczamy które rogi powinny być podświetlone
        double step = 1.0 / 4; // 4 rogi

        if (m_cornerGlowProgress >= step * 0) { // Lewy górny
            painter.drawLine(0, clipSize * 1.5, 0, clipSize);
            painter.drawLine(0, clipSize, clipSize, 0);
            painter.drawLine(clipSize, 0, clipSize * 1.5, 0);
        }

        if (m_cornerGlowProgress >= step * 1) { // Prawy górny
            painter.drawLine(width() - clipSize * 1.5, 0, width() - clipSize, 0);
            painter.drawLine(width() - clipSize, 0, width(), clipSize);
            painter.drawLine(width(), clipSize, width(), clipSize * 1.5);
        }

        if (m_cornerGlowProgress >= step * 2) { // Prawy dolny
            painter.drawLine(width(), height() - clipSize * 1.5, width(), height() - clipSize);
            painter.drawLine(width(), height() - clipSize, width() - clipSize, height());
            painter.drawLine(width() - clipSize, height(), width() - clipSize * 1.5, height());
        }

        if (m_cornerGlowProgress >= step * 3) { // Lewy dolny
            painter.drawLine(clipSize * 1.5, height(), clipSize, height());
            painter.drawLine(clipSize, height(), 0, height() - clipSize);
            painter.drawLine(0, height() - clipSize, 0, height() - clipSize * 1.5);
        }
    }

    // Linie skanowania (scanlines)
    if (m_scanlineOpacity > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * m_scanlineOpacity));

        for (int y = 0; y < height(); y += 3) {
            painter.drawRect(0, y, width(), 1);
        }
    }
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

            // Nie pokazujemy błędu od razu po zaznaczeniu checkboxa
            // Komunikat pojawi się tylko przy próbie wygenerowania
        }

        // Przycisk jest aktywny tylko jeśli znaleziono częstotliwość i hasło jest prawidłowe (jeśli wymagane)
        generateButton->setEnabled(isPasswordValid && m_frequencyFound);
    }

    void startFrequencySearch() {
        qDebug() << "LOG: Rozpoczynam wyszukiwanie częstotliwości po zakończeniu animacji";
        loadingIndicator->setText("SEARCHING FOR AVAILABLE FREQUENCY...");

        // Timer zabezpieczający przed zbyt długim wyszukiwaniem
        QTimer* timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);
        timeoutTimer->setInterval(5000); // 5 sekund limitu na szukanie
        connect(timeoutTimer, &QTimer::timeout, [this]() {
            if (frequencyWatcher && frequencyWatcher->isRunning()) {
                loadingIndicator->setText("USING DEFAULT FREQUENCY...");
                m_frequency = 130.0;
                m_frequencyFound = true;
                onFrequencyFound();
                qDebug() << "LOG: Timeout podczas szukania częstotliwości - używam wartości domyślnej";
            }
        });
        timeoutTimer->start();

        // Uruchom asynchroniczne wyszukiwanie
        QFuture<double> future = QtConcurrent::run(&WavelengthDialog::findLowestAvailableFrequency);
        frequencyWatcher->setFuture(future);
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

        // Sprawdź warunki bezpieczeństwa - tutaj pokazujemy błąd, gdy próbujemy wygenerować hasło
        if (isPasswordProtected && password.isEmpty()) {
            statusLabel->setText("PASSWORD REQUIRED");
            statusLabel->show();
            isGenerating = false;
            return;
        }

        qDebug() << "LOG: tryGenerate - walidacja pomyślna, akceptacja dialogu";
        QDialog::accept();

        isGenerating = false;
        qDebug() << "LOG: tryGenerate - koniec";
    }

    void onFrequencyFound() {
        // Zatrzymujemy timer zabezpieczający jeśli istnieje
        QTimer* timeoutTimer = findChild<QTimer*>();
        if (timeoutTimer && timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }

        // Pobierz wynik asynchronicznego wyszukiwania
        if (frequencyWatcher && frequencyWatcher->isFinished()) {
            m_frequency = frequencyWatcher->result();
            qDebug() << "LOG: onFrequencyFound otrzymało wartość:" << m_frequency;
        } else {
            qDebug() << "LOG: onFrequencyFound - brak wyniku, używam domyślnego 130 Hz";
            m_frequency = 130.0;
        }

        m_frequencyFound = true;

        // Przygotuj tekst częstotliwości
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

        // Dodaj efekt scanline przy znalezieniu częstotliwości
        QPropertyAnimation* scanAnim = new QPropertyAnimation(this, "scanlineOpacity");
        scanAnim->setDuration(1500);
        scanAnim->setStartValue(0.3);
        scanAnim->setEndValue(0.08);
        scanAnim->setKeyValueAt(0.3, 0.4);
        scanAnim->start(QAbstractAnimation::DeleteWhenStopped);
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
    qDebug() << "LOG: Rozpoczęto szukanie dostępnej częstotliwości";

    // Pobierz preferowaną częstotliwość startową z konfiguracji
    WavelengthConfig* config = WavelengthConfig::getInstance();
    double preferredFreq = config->getPreferredStartFrequency();
    qDebug() << "LOG: Używam preferowanej częstotliwości startowej:" << preferredFreq << "Hz";

    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply;

    // Utwórz URL i dodaj preferowaną częstotliwość jako parametr zapytania
    QUrl url("http://localhost:3000/api/next-available-frequency");
    QUrlQuery query;
    query.addQueryItem("preferredStartFrequency", QString::number(preferredFreq, 'f', 1)); // Przekaż jako string z 1 miejscem po przecinku
    url.setQuery(query);

    qDebug() << "LOG: Wysyłanie żądania do:" << url.toString();
    QNetworkRequest request(url);

    // Wykonaj żądanie synchronicznie (w kontekście asynchronicznego QtConcurrent::run)
    reply = manager.get(request);

    // Połącz sygnał zakończenia z pętlą zdarzeń
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // Uruchom pętlę zdarzeń aż do zakończenia żądania
    loop.exec();

    double resultFrequency = 130.0; // Domyślna wartość w razie błędu

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (obj.contains("frequency") && obj["frequency"].isDouble()) {
            resultFrequency = obj["frequency"].toDouble();
            qDebug() << "LOG: Serwer zwrócił dostępną częstotliwość:" << resultFrequency << "Hz";
        } else {
             qDebug() << "LOG: Błąd parsowania odpowiedzi JSON lub brak klucza 'frequency'. Odpowiedź:" << responseData;
             // Użyj domyślnej wartości 130.0
        }
    } else {
        qDebug() << "LOG: Błąd podczas komunikacji z serwerem:" << reply->errorString();
        // Użyj domyślnej wartości 130.0
    }

    reply->deleteLater();

    qDebug() << "LOG: Zakończono szukanie, zwracam częstotliwość:" << resultFrequency << "Hz";
    return resultFrequency;
}

    void initRenderBuffers() {
    if (!m_buffersInitialized || height() != m_previousHeight) {
        int clipSize = 20; // rozmiar wycięcia rogów dialogu

        // Bufor głównej linii skanującej - nadal na pełnej szerokości
        m_scanlineBuffer = QPixmap(width(), 20);
        m_scanlineBuffer.fill(Qt::transparent);

        QPainter scanPainter(&m_scanlineBuffer);
        scanPainter.setRenderHint(QPainter::Antialiasing, false);

        // Główna linia skanująca jako gradient
        QLinearGradient scanGradient(0, 0, 0, 20);
        scanGradient.setColorAt(0, QColor(0, 200, 255, 0));
        scanGradient.setColorAt(0.5, QColor(0, 220, 255, 180));
        scanGradient.setColorAt(1, QColor(0, 200, 255, 0));

        // Rysujemy na pełnej szerokości - ale uwzględniając kształt dialogu
        scanPainter.setPen(Qt::NoPen);
        scanPainter.setBrush(scanGradient);

        // Ścieżka dialogu ze ściętymi rogami dla linii skanującej (mniejszej wysokości)
        QPainterPath scanlinePath;
        scanlinePath.moveTo(clipSize, 0);
        scanlinePath.lineTo(width() - clipSize, 0);
        scanlinePath.lineTo(width(), 0);
        scanlinePath.lineTo(width(), 20);
        scanlinePath.lineTo(0, 20);
        scanlinePath.lineTo(0, 0);
        scanlinePath.closeSubpath();

        scanPainter.drawPath(scanlinePath);

        // Bufor dla linii poziomych - tworzymy nowy dla każdej wysokości
        m_scanLinesBuffer = QPixmap(width(), height());
        m_scanLinesBuffer.fill(Qt::transparent);

        QPainter linesPainter(&m_scanLinesBuffer);
        linesPainter.setRenderHint(QPainter::Antialiasing, true);
        linesPainter.setPen(QPen(QColor(0, 180, 255, 40)));

        // Rysujemy linie poziome z uwzględnieniem kształtu dialogu
        for (int y = 0; y < height(); y += 6) {
            // Obliczamy szerokość w danym punkcie Y
            int startX = 0;
            int endX = width();

            // Górny obszar ścięć
            if (y < clipSize) {
                // Liniowa zmiana szerokości przy górnych ścięciach
                float ratio = (float)y / clipSize;
                startX = clipSize * (1.0f - ratio);
                endX = width() - clipSize * (1.0f - ratio);
            }
            // Dolny obszar ścięć
            else if (y > height() - clipSize) {
                // Liniowa zmiana szerokości przy dolnych ścięciach
                float ratio = (float)(height() - y) / clipSize;
                startX = clipSize * (1.0f - ratio);
                endX = width() - clipSize * (1.0f - ratio);
            }

            // Rysuj linię tylko jeśli jest widoczna
            if (startX < endX) {
                linesPainter.drawLine(startX, y, endX, y);
            }
        }

        m_buffersInitialized = true;
        m_previousHeight = height();
    }
}

private:
    bool m_animationStarted = false;
    QLabel *frequencyLabel;
    QLabel *loadingIndicator;
    CyberCheckBox *passwordProtectedCheckbox;
    CyberLineEdit *passwordEdit;
    QLabel *statusLabel;
    CyberButton *generateButton;
    CyberButton *cancelButton;
    QFutureWatcher<double> *frequencyWatcher;
    QTimer *m_refreshTimer;
    double m_frequency = 130.0; // Domyślna wartość
    bool m_frequencyFound = false; // Flaga oznaczająca znalezienie częstotliwości
    const int m_shadowSize; // Rozmiar cienia
    double m_scanlineOpacity; // Przezroczystość linii skanowania
    QPixmap m_scanlineBuffer;
    QPixmap m_scanLinesBuffer;
    bool m_buffersInitialized = false;
    int m_lastScanlineY = -1;
    int m_previousHeight = 0;
};

#endif // WAVELENGTH_DIALOG_H