#include "wavelength_dialog.h"

#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QNetworkReply>
#include <QVBoxLayout>

WavelengthDialog::WavelengthDialog(QWidget *parent): AnimatedDialog(parent, AnimatedDialog::DigitalMaterialization),
                                                     m_shadowSize(10), m_scanlineOpacity(0.08) {

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // Preferuj animacje przez GPU
    setAttribute(Qt::WA_TranslucentBackground);

    setAttribute(Qt::WA_StaticContents, true);

    m_digitalizationProgress = 0.0;
    m_animationStarted = false;

    setWindowTitle("CREATE_WAVELENGTH::NEW_INSTANCE");
    setModal(true);
    setFixedSize(450, 350);

    setAnimationDuration(400);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // Nagłówek z tytułem
    auto titleLabel = new QLabel("GENERATE WAVELENGTH", this);
    titleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    titleLabel->setContentsMargins(0, 0, 0, 3);
    mainLayout->addWidget(titleLabel);

    // Panel informacyjny z ID sesji
    auto infoPanel = new QWidget(this);
    auto infoPanelLayout = new QHBoxLayout(infoPanel);
    infoPanelLayout->setContentsMargins(0, 0, 0, 0);
    infoPanelLayout->setSpacing(5);

    QString sessionId = QString("%1-%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10000, 99999));
    auto sessionLabel = new QLabel(QString("SESSION_ID: %1").arg(sessionId), this);
    sessionLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    auto timeLabel = new QLabel(QString("TS: %1").arg(timestamp), this);
    timeLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    infoPanelLayout->addWidget(sessionLabel);
    infoPanelLayout->addStretch();
    infoPanelLayout->addWidget(timeLabel);
    mainLayout->addWidget(infoPanel);

    // Panel instrukcji - poprawka dla responsywności
    auto infoLabel = new QLabel("System automatically assigns the lowest available frequency", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    infoLabel->setAlignment(Qt::AlignLeft);
    infoLabel->setWordWrap(true); // Włącz zawijanie tekstu
    mainLayout->addWidget(infoLabel);

    // Uprościliśmy panel formularza - bez dodatkowego kontenera
    auto formLayout = new QFormLayout();
    formLayout->setSpacing(12); // Zwiększ odstępy
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // Wyśrodkuj w pionie
    formLayout->setContentsMargins(0, 15, 0, 15); // Dodaj więcej przestrzeni w pionie

    // Etykiety w formularzu
    auto frequencyTitleLabel = new QLabel("ASSIGNED FREQUENCY:", this);
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
    auto passwordLabel = new QLabel("PASSWORD:", this);
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
    auto buttonLayout = new QHBoxLayout();
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
    connect(passwordProtectedCheckbox, &QCheckBox::toggled, this, [this](const bool checked) {
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

    frequencyWatcher = new QFutureWatcher<QString>(this);
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
            const int currentScanlineY = static_cast<int>(height() * m_digitalizationProgress);

            if (currentScanlineY != m_lastScanlineY) {
                constexpr int scanlineHeight = 20;
                // Określ obszar do odświeżenia: obejmuje starą i nową pozycję linii
                int updateY_start = qMin(currentScanlineY, m_lastScanlineY) - scanlineHeight / 2 - 2; // Trochę zapasu
                int updateY_end = qMax(currentScanlineY, m_lastScanlineY) + scanlineHeight / 2 + 2;

                // Ogranicz do granic widgetu
                updateY_start = qMax(0, updateY_start);
                updateY_end = qMin(height(), updateY_end);

                // Odśwież tylko ten prostokątny obszar
                update(0, updateY_start, width(), updateY_end - updateY_start);

                m_lastScanlineY = currentScanlineY; // Zaktualizuj ostatnią pozycję
            }
            m_refreshTimer->setInterval(16); // Utrzymaj 60fps podczas animacji
        } else {
            m_refreshTimer->setInterval(100); // Znacznie wolniej, gdy nic się nie animuje
        }
    });
}

WavelengthDialog::~WavelengthDialog() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        delete m_refreshTimer;
        m_refreshTimer = nullptr;
    }
}

void WavelengthDialog::setDigitalizationProgress(const double progress) {
    if (!m_animationStarted && progress > 0.01)
        m_animationStarted = true;
    m_digitalizationProgress = progress;
    update();
}

void WavelengthDialog::setCornerGlowProgress(const double progress) {
    m_cornerGlowProgress = progress;
    update();
}

void WavelengthDialog::setScanlineOpacity(const double opacity) {
    m_scanlineOpacity = opacity;
    update();
}

void WavelengthDialog::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Główne tło dialogu z gradientem (bez zmian)
    QLinearGradient bgGradient(0, 0, 0, height());
    bgGradient.setColorAt(0, QColor(10, 21, 32));
    bgGradient.setColorAt(1, QColor(7, 18, 24));

    // Ścieżka dialogu ze ściętymi rogami (bez zmian)
    QPainterPath dialogPath;
    int clipSize = 20; // rozmiar ścięcia
    dialogPath.moveTo(clipSize, 0);
    dialogPath.lineTo(width() - clipSize, 0);
    dialogPath.lineTo(width(), clipSize);
    dialogPath.lineTo(width(), height() - clipSize);
    dialogPath.lineTo(width() - clipSize, height());
    dialogPath.lineTo(clipSize, height());
    dialogPath.lineTo(0, height() - clipSize);
    dialogPath.lineTo(0, clipSize);
    dialogPath.closeSubpath(); // Zamknij ścieżkę

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgGradient);
    painter.drawPath(dialogPath);

    // Obramowanie dialogu (bez zmian)
    QColor borderColor(0, 195, 255, 150);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(dialogPath);

    // --- ZMIANA: Rysowanie tylko pojedynczej linii skanującej ---
    if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0 && m_animationStarted) {
        // Inicjalizacja bufora linii skanującej (jeśli potrzebna)
        initRenderBuffers(); // Teraz inicjalizuje tylko m_scanlineBuffer

        int scanLineY = static_cast<int>(height() * m_digitalizationProgress);
        int clip_size = 20;

        // Sprawdź, czy bufor linii skanującej jest gotowy
        if (!m_scanlineBuffer.isNull()) {
            painter.setClipping(false); // Wyłącz clipping na czas rysowania linii
            QPainter::CompositionMode previousMode = painter.compositionMode();
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);


            // Oblicz szerokość linii skanującej na danej wysokości (bez zmian)
            int startX = 0;
            int endX = width();
            if (scanLineY < clip_size) {
                float ratio = static_cast<float>(scanLineY) / clip_size;
                startX = clip_size * (1.0f - ratio);
                endX = width() - clip_size * (1.0f - ratio);
            } else if (scanLineY > height() - clip_size) {
                float ratio = static_cast<float>(height() - scanLineY) / clip_size;
                startX = clip_size * (1.0f - ratio);
                endX = width() - clip_size * (1.0f - ratio);
            }

            // Rysuj tylko główną linię skanującą (bez zmian)
            int scanWidth = endX - startX;
            if (scanWidth > 0) {
                painter.drawPixmap(startX, scanLineY - 10, scanWidth, 20, // Pozycja Y centruje gradient
                                   m_scanlineBuffer,
                                   (width() - scanWidth) / 2, 0, scanWidth, 20); // Wybierz odpowiedni fragment bufora
            }

            painter.setCompositionMode(previousMode);
        }
    }
    // --- KONIEC ZMIANY ---


    if (m_cornerGlowProgress > 0.0) {
        QColor cornerColor(0, 220, 255, 150);
        painter.setPen(QPen(cornerColor, 2));
        double step = 1.0 / 4;
        if (m_cornerGlowProgress >= step * 0) { /* Lewy górny */ painter.drawLine(0, clipSize * 1.5, 0, clipSize); painter.drawLine(0, clipSize, clipSize, 0); painter.drawLine(clipSize, 0, clipSize * 1.5, 0); }
        if (m_cornerGlowProgress >= step * 1) { /* Prawy górny */ painter.drawLine(width() - clipSize * 1.5, 0, width() - clipSize, 0); painter.drawLine(width() - clipSize, 0, width(), clipSize); painter.drawLine(width(), clipSize, width(), clipSize * 1.5); }
        if (m_cornerGlowProgress >= step * 2) { /* Prawy dolny */ painter.drawLine(width(), height() - clipSize * 1.5, width(), height() - clipSize); painter.drawLine(width(), height() - clipSize, width() - clipSize, height()); painter.drawLine(width() - clipSize, height(), width() - clipSize * 1.5, height()); }
        if (m_cornerGlowProgress >= step * 3) { /* Lewy dolny */ painter.drawLine(clipSize * 1.5, height(), clipSize, height()); painter.drawLine(clipSize, height(), 0, height() - clipSize); painter.drawLine(0, height() - clipSize, 0, height() - clipSize * 1.5); }
    }

    // Linie skanowania (scanlines) - tło (bez zmian)
    if (m_scanlineOpacity > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * m_scanlineOpacity));
        for (int y = 0; y < height(); y += 3) {
            painter.drawRect(0, y, width(), 1);
        }
    }
}

void WavelengthDialog::validateInputs() const {
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

void WavelengthDialog::startFrequencySearch() {
    qDebug() << "LOG: Rozpoczynam wyszukiwanie częstotliwości po zakończeniu animacji";
    loadingIndicator->setText("SEARCHING FOR AVAILABLE FREQUENCY...");

    // Timer zabezpieczający przed zbyt długim wyszukiwaniem
    const auto timeoutTimer = new QTimer(this);
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
    const QFuture<QString> future = QtConcurrent::run(&WavelengthDialog::findLowestAvailableFrequency);
    frequencyWatcher->setFuture(future);
}

void WavelengthDialog::tryGenerate() {
    static bool isGenerating = false;

    if (isGenerating) {
        qDebug() << "LOG: Blokada wielokrotnego wywołania tryGenerate()";
        return;
    }

    isGenerating = true;
    qDebug() << "LOG: tryGenerate - start";

    const bool isPasswordProtected = passwordProtectedCheckbox->isChecked();
    const QString password = passwordEdit->text();

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

void WavelengthDialog::onFrequencyFound() {
    // Zatrzymujemy timer zabezpieczający jeśli istnieje
    const auto timeoutTimer = findChild<QTimer*>();
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
    QString frequencyText = m_frequency;

    // Przygotowanie animacji dla wskaźnika ładowania (znikanie)
    const auto loaderSlideAnimation = new QPropertyAnimation(loadingIndicator, "maximumHeight");
    loaderSlideAnimation->setDuration(400);
    loaderSlideAnimation->setStartValue(loadingIndicator->sizeHint().height());
    loaderSlideAnimation->setEndValue(0);
    loaderSlideAnimation->setEasingCurve(QEasingCurve::OutQuint);

    // Dodanie efektu przezroczystości dla etykiety częstotliwości
    const auto opacityEffect = new QGraphicsOpacityEffect(frequencyLabel);
    frequencyLabel->setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(0.0); // Początkowo niewidoczny

    // Animacja pojawiania się etykiety częstotliwości
    auto frequencyAnimation = new QPropertyAnimation(opacityEffect, "opacity");
    frequencyAnimation->setDuration(600);
    frequencyAnimation->setStartValue(0.0);
    frequencyAnimation->setEndValue(1.0);
    frequencyAnimation->setEasingCurve(QEasingCurve::InQuad);

    // Utwórz grupę animacji, która zostanie uruchomiona sekwencyjnie
    const auto animGroup = new QParallelAnimationGroup(this);
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
    const auto scanAnim = new QPropertyAnimation(this, "scanlineOpacity");
    scanAnim->setDuration(1500);
    scanAnim->setStartValue(0.3);
    scanAnim->setEndValue(0.08);
    scanAnim->setKeyValueAt(0.3, 0.4);
    scanAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

QString WavelengthDialog::formatFrequencyText(const double frequency) {
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

QString WavelengthDialog::findLowestAvailableFrequency() {
    qDebug() << "LOG: Rozpoczęto szukanie dostępnej częstotliwości";

    // Pobierz preferowaną częstotliwość startową z konfiguracji
    const WavelengthConfig* config = WavelengthConfig::GetInstance();
    const QString preferredFreq = config->GetPreferredStartFrequency();
    qDebug() << "LOG: Używam preferowanej częstotliwości startowej:" << preferredFreq << "Hz";

    QNetworkAccessManager manager;
    QEventLoop loop;

    const QString serverAddress = config->GetRelayServerAddress();
    const int serverPort = config->GetRelayServerPort();

    const QString baseUrlString = QString("http://%1:%2/api/next-available-frequency").arg(serverAddress).arg(serverPort);
    QUrl url(baseUrlString);

    QUrlQuery query;
    query.addQueryItem("preferredStartFrequency", preferredFreq);
    url.setQuery(query);

    qDebug() << "LOG: Wysyłanie żądania do:" << url.toString();
    const QNetworkRequest request(url);

    // Wykonaj żądanie synchronicznie (w kontekście asynchronicznego QtConcurrent::run)
    QNetworkReply *reply = manager.get(request);

    // Połącz sygnał zakończenia z pętlą zdarzeń
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // Uruchom pętlę zdarzeń aż do zakończenia żądania
    loop.exec();

    QString resultFrequency = "130.0"; // Domyślna wartość w razie błędu

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray responseData = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (obj.contains("frequency") && obj["frequency"].isString()) {
            resultFrequency = obj["frequency"].toString();
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

void WavelengthDialog::initRenderBuffers() {
    if (!m_buffersInitialized || height() != m_previousHeight) {
        // --- ZMIANA: Inicjalizuj tylko bufor głównej linii skanującej ---
        m_scanlineBuffer = QPixmap(width(), 20); // Stała wysokość 20px
        m_scanlineBuffer.fill(Qt::transparent);

        QPainter scanPainter(&m_scanlineBuffer);
        scanPainter.setRenderHint(QPainter::Antialiasing, false); // Antialiasing niepotrzebny dla gradientu

        // Główna linia skanująca jako gradient (bez zmian)
        QLinearGradient scanGradient(0, 0, 0, 20); // Gradient w pionie na wysokości 20px
        scanGradient.setColorAt(0, QColor(0, 200, 255, 0));   // Przezroczysty na górze
        scanGradient.setColorAt(0.5, QColor(0, 220, 255, 180)); // Najjaśniejszy w środku
        scanGradient.setColorAt(1, QColor(0, 200, 255, 0));   // Przezroczysty na dole

        scanPainter.setPen(Qt::NoPen);
        scanPainter.setBrush(scanGradient);
        scanPainter.drawRect(0, 0, width(), 20); // Rysuj gradient na całej szerokości bufora


        m_buffersInitialized = true;
        m_previousHeight = height(); // Zapisz wysokość dla ewentualnych przyszłych sprawdzeń
        m_lastScanlineY = -1; // Zresetuj ostatnią pozycję przy reinicjalizacji
    }
}
