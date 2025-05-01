#include "wavelength_chat_view.h"

#include "../chat/style/chat_style.h"
#include <QFileDialog>

WavelengthChatView::WavelengthChatView(QWidget *parent): QWidget(parent), m_scanlineOpacity(0.15) {
    setObjectName("chatViewContainer");

    // Podstawowy layout
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Nagłówek z dodatkowym efektem
    const auto headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    headerLabel = new QLabel(this);
    headerLabel->setObjectName("headerLabel");
    headerLayout->addWidget(headerLabel);

    // Wskaźnik status połączenia
    m_statusIndicator = new QLabel("AKTYWNE POŁĄCZENIE", this);
    m_statusIndicator->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_statusIndicator->setStyleSheet(
        "color: #00ffaa;"
        "background-color: rgba(0, 40, 30, 150);"
        "border: 1px solid #00aa77;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );
    headerLayout->addWidget(m_statusIndicator);

    mainLayout->addLayout(headerLayout);

    // Obszar wiadomości
    messageArea = new WavelengthStreamDisplay(this);
    messageArea->setStyleSheet(ChatStyle::GetScrollBarStyle());
    mainLayout->addWidget(messageArea, 1);

    // Panel informacji technicznych
    const auto infoPanel = new QWidget(this);
    const auto infoLayout = new QHBoxLayout(infoPanel);
    infoLayout->setContentsMargins(3, 3, 3, 3);
    infoLayout->setSpacing(10);

    // Techniczne etykiety w stylu cyberpunk
    const QString infoStyle =
            "color: #00aaff;"
            "background-color: transparent;"
            "font-family: 'Consolas';"
            "font-size: 8pt;";

    const auto sessionLabel = new QLabel(QString("SID:%1").arg(QRandomGenerator::global()->bounded(1000, 9999)), this);
    sessionLabel->setStyleSheet(infoStyle);
    infoLayout->addWidget(sessionLabel);

    const auto bufferLabel = new QLabel("BUFFER:100%", this);
    bufferLabel->setStyleSheet(infoStyle);
    infoLayout->addWidget(bufferLabel);

    infoLayout->addStretch();

    auto timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm:ss"), this);
    timeLabel->setStyleSheet(infoStyle);
    infoLayout->addWidget(timeLabel);

    // Timer dla aktualnego czasu
    const auto timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, [timeLabel]() {
        timeLabel->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
    });
    timeTimer->start(1000);

    mainLayout->addWidget(infoPanel);

    // Panel wprowadzania wiadomości
    const auto inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(10);

    // Przycisk załączania plików - niestandardowy wygląd
    attachButton = new CyberChatButton("📎", this);
    attachButton->setToolTip("Attach file");
    attachButton->setFixedSize(36, 36);
    inputLayout->addWidget(attachButton);

    // Pole wprowadzania wiadomości
    inputField = new QLineEdit(this);
    inputField->setObjectName("chatInputField");
    inputField->setPlaceholderText("Wpisz wiadomość...");
    inputField->setStyleSheet(ChatStyle::GetInputFieldStyle());
    inputLayout->addWidget(inputField, 1);

    // Przycisk wysyłania - niestandardowy wygląd
    sendButton = new CyberChatButton("Wyślij", this);
    sendButton->setMinimumWidth(80);
    sendButton->setFixedHeight(36);
    inputLayout->addWidget(sendButton);

    // --- NOWY PRZYCISK PTT ---
    pttButton = new CyberChatButton("MÓW", this);
    pttButton->setToolTip("Naciśnij i przytrzymaj, aby mówić");
    pttButton->setCheckable(false); // Działa na press/release
    connect(pttButton, &QPushButton::pressed, this, &WavelengthChatView::onPttButtonPressed);
    connect(pttButton, &QPushButton::released, this, &WavelengthChatView::onPttButtonReleased);
    inputLayout->addWidget(pttButton);
    // --- KONIEC NOWEGO PRZYCISKU PTT ---

    mainLayout->addLayout(inputLayout);

    // Przycisk przerwania połączenia
    abortButton = new CyberChatButton("Zakończ połączenie", this);
    abortButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #662222;"
        "  color: #ffaaaa;"
        "  border: 1px solid #993333;"
        "  margin-top: 5px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #883333;"
        "  border: 1px solid #bb4444;"
        "}"
    );
    mainLayout->addWidget(abortButton);

    initializeAudio();

    m_pttOnSound = new QSoundEffect(this);
    m_pttOnSound->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/ptt_on.wav"));
    m_pttOnSound->setVolume(0.8);

    m_pttOffSound = new QSoundEffect(this);
    m_pttOffSound->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/ptt_off.wav"));
    m_pttOffSound->setVolume(0.8);

    // Połączenia sygnałów
    connect(inputField, &QLineEdit::returnPressed, this, &WavelengthChatView::sendMessage);
    connect(sendButton, &QPushButton::clicked, this, &WavelengthChatView::sendMessage);
    connect(attachButton, &QPushButton::clicked, this, &WavelengthChatView::attachFile);
    connect(abortButton, &QPushButton::clicked, this, &WavelengthChatView::abortWavelength);

    const WavelengthMessageService *messageService = WavelengthMessageService::GetInstance();
    connect(messageService, &WavelengthMessageService::progressMessageUpdated,
            this, &WavelengthChatView::updateProgressMessage);
    // connect(messageService, &WavelengthMessageService::removeProgressMessage,
    //         this, &WavelengthChatView::removeProgressMessage);

    const WavelengthSessionCoordinator* coordinator = WavelengthSessionCoordinator::GetInstance();
    connect(coordinator, &WavelengthSessionCoordinator::pttGranted, this, &WavelengthChatView::onPttGranted);
    connect(coordinator, &WavelengthSessionCoordinator::pttDenied, this, &WavelengthChatView::onPttDenied);
    connect(coordinator, &WavelengthSessionCoordinator::pttStartReceiving, this, &WavelengthChatView::onPttStartReceiving);
    connect(coordinator, &WavelengthSessionCoordinator::pttStopReceiving, this, &WavelengthChatView::onPttStopReceiving);
    connect(coordinator, &WavelengthSessionCoordinator::audioDataReceived, this, &WavelengthChatView::onAudioDataReceived);

    // Timer dla efektów wizualnych
    const auto glitchTimer = new QTimer(this);
    connect(glitchTimer, &QTimer::timeout, this, &WavelengthChatView::triggerVisualEffect);
    glitchTimer->start(5000 + QRandomGenerator::global()->bounded(5000));

    QWidget::setVisible(false);
}

WavelengthChatView::~WavelengthChatView() {
    // Zatrzymaj i zwolnij zasoby audio
    stopAudioInput();
    stopAudioOutput();
    delete m_audioInput;
    delete m_audioOutput;
    // m_inputDevice i m_outputDevice są zarządzane przez QAudioInput/Output
}

void WavelengthChatView::setScanlineOpacity(const double opacity) {
    m_scanlineOpacity = opacity;
    update();
}

void WavelengthChatView::setWavelength(const QString &frequency, const QString &name) {
    currentFrequency = frequency;

    resetStatusIndicator();

    QString title;
    if (name.isEmpty()) {
        title = QString("Wavelength: %1 Hz").arg(frequency);
    } else {
        title = QString("%1 (%2 Hz)").arg(name).arg(frequency);
    }
    headerLabel->setText(title);

    messageArea->Clear();

    const QString welcomeMsg = QString("<span style=\"color:#ffcc00;\">Połączono z wavelength %1 Hz o %2</span>")
            .arg(frequency)
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
    messageArea->AddMessage(welcomeMsg, "system", StreamMessage::MessageType::kSystem);

    // Efekt wizualny przy połączeniu
    triggerConnectionEffect();

    setVisible(true);
    inputField->setFocus();
    inputField->clear();
    pttButton->setEnabled(true); // Włącz przycisk PTT
    abortButton->setEnabled(true);
    resetStatusIndicator();
    m_isAborting = false;
    // Zatrzymaj PTT jeśli było aktywne
    if (m_pttState == Transmitting) {
        onPttButtonReleased();
    }
    m_pttState = Idle;
    updatePttButtonState();
}

void WavelengthChatView::onMessageReceived(const QString &frequency, const QString &message) {
    if (frequency != currentFrequency) return;
    QTimer::singleShot(0, this, [this, message]() {
        messageArea->AddMessage(message, QString(), StreamMessage::MessageType::kReceived);
        triggerActivityEffect();
    });
}

void WavelengthChatView::onMessageSent(const QString &frequency, const QString &message) const {
    if (frequency != currentFrequency) return;
    messageArea->AddMessage(message, QString(), StreamMessage::MessageType::kTransmitted);
}

void WavelengthChatView::onWavelengthClosed(const QString &frequency) {
    if (frequency != currentFrequency || m_isAborting) {
        return;
    }

    m_statusIndicator->setText("POŁĄCZENIE ZAMKNIĘTE");
    m_statusIndicator->setStyleSheet(
        "color: #ff5555;"
        "background-color: rgba(40, 0, 0, 150);"
        "border: 1px solid #aa3333;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );

    const auto closeMsg = QString("<span style=\"color:#ff5555;\">Wavelength zostało zamknięte przez hosta.</span>");
    messageArea->AddMessage(closeMsg, "system", StreamMessage::MessageType::kSystem);

    QTimer::singleShot(2000, this, [this]() {
        clear();
        emit wavelengthAborted();
    });
}

void WavelengthChatView::clear() {
    currentFrequency = -1;
    messageArea->Clear();
    headerLabel->clear();
    inputField->clear();
    setVisible(false);
}

void WavelengthChatView::attachFile() {
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                    "Select File to Attach",
                                                    QString(),
                                                    "Media Files (*.jpg *.jpeg *.png *.gif *.mp3 *.mp4 *.wav);;All Files (*)");

    if (filePath.isEmpty()) {
        return;
    }

    // Pobieramy nazwę pliku
    const QFileInfo fileInfo(filePath);
    const QString fileName = fileInfo.fileName();

    // Generujemy identyfikator dla komunikatu
    const QString progressMsgId = "file_progress_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    // Wyświetlamy początkowy komunikat
    const QString processingMsg = QString("<span style=\"color:#888888;\">Sending file: %1...</span>")
            .arg(fileName);

    messageArea->AddMessage(processingMsg, progressMsgId, StreamMessage::MessageType::kTransmitted);

    // Uruchamiamy asynchroniczny proces przetwarzania pliku
    WavelengthMessageService *service = WavelengthMessageService::GetInstance();
    const bool started = service->SendFile(filePath, progressMsgId);

    if (!started) {
        messageArea->AddMessage(progressMsgId,
                                "<span style=\"color:#ff5555;\">Failed to start file processing.</span>",
                                StreamMessage::MessageType::kTransmitted);
    }
}

void WavelengthChatView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysowanie ramki w stylu AR/cyberpunk
    constexpr QColor borderColor(0, 170, 255);
    painter.setPen(QPen(borderColor, 1));

    // Zewnętrzna technologiczna ramka ze ściętymi rogami
    QPainterPath frame;
    constexpr int clipSize = 15;

    // Górna krawędź
    frame.moveTo(clipSize, 0);
    frame.lineTo(width() - clipSize, 0);

    // Prawy górny róg
    frame.lineTo(width(), clipSize);

    // Prawa krawędź
    frame.lineTo(width(), height() - clipSize);

    // Prawy dolny róg
    frame.lineTo(width() - clipSize, height());

    // Dolna krawędź
    frame.lineTo(clipSize, height());

    // Lewy dolny róg
    frame.lineTo(0, height() - clipSize);

    // Lewa krawędź
    frame.lineTo(0, clipSize);

    // Lewy górny róg
    frame.lineTo(clipSize, 0);

    painter.drawPath(frame);

    // Znaczniki AR w rogach
    painter.setPen(QPen(borderColor, 1, Qt::SolidLine));
    constexpr int markerSize = 8;

    // Lewy górny
    painter.drawLine(clipSize, 5, clipSize + markerSize, 5);
    painter.drawLine(clipSize, 5, clipSize, 5 + markerSize);

    // Prawy górny
    painter.drawLine(width() - clipSize - markerSize, 5, width() - clipSize, 5);
    painter.drawLine(width() - clipSize, 5, width() - clipSize, 5 + markerSize);

    // Prawy dolny
    painter.drawLine(width() - clipSize - markerSize, height() - 5, width() - clipSize, height() - 5);
    painter.drawLine(width() - clipSize, height() - 5, width() - clipSize, height() - 5 - markerSize);

    // Lewy dolny
    painter.drawLine(clipSize, height() - 5, clipSize + markerSize, height() - 5);
    painter.drawLine(clipSize, height() - 5, clipSize, height() - 5 - markerSize);

    // Efekt scanlines (jeśli aktywny)
    if (m_scanlineOpacity > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 40 * m_scanlineOpacity));

        for (int y = 0; y < height(); y += 3) {
            painter.drawRect(0, y, width(), 1);
        }
    }
}

void WavelengthChatView::onPttButtonPressed() {
    // --- ODTWÓRZ DŹWIĘK NACIŚNIĘCIA ---
    if (m_pttOnSound->isLoaded()) {
        m_pttOnSound->play();
    } else {
        qWarning() << "PTT On sound not loaded!";
    }
    // --- KONIEC ODTWARZANIA ---

    if (currentFrequency == "-1.0" || m_pttState != Idle) {
        return;
    }
    qDebug() << "PTT Button Pressed - Requesting PTT for" << currentFrequency;
    m_pttState = Requesting;
    updatePttButtonState();
    WavelengthMessageService::GetInstance()->SendPttRequest(currentFrequency);
    pttButton->setStyleSheet("background-color: yellow; color: black;");
}

void WavelengthChatView::onPttButtonReleased() {
    // --- ODTWÓRZ DŹWIĘK ZWOLNIENIA ---
    // Odtwórz dźwięk niezależnie od tego, czy faktycznie nadawaliśmy
    if (m_pttOffSound->isLoaded()) {
        m_pttOffSound->play();
    } else {
        qWarning() << "PTT Off sound not loaded!";
    }
    // --- KONIEC ODTWARZANIA ---

    if (m_pttState == Transmitting) {
        qDebug() << "PTT Button Released - Stopping Transmission for" << currentFrequency;
        stopAudioInput();
        WavelengthMessageService::GetInstance()->SendPttRelease(currentFrequency);
        messageArea->ClearTransmittingUser();
        messageArea->SetAudioAmplitude(0.0);
    } else if (m_pttState == Requesting) {
        qDebug() << "PTT Button Released - Cancelling Request for" << currentFrequency;
        // Można dodać wysłanie anulowania żądania, jeśli serwer to obsługuje
    }
    m_pttState = Idle;
    updatePttButtonState();
    pttButton->setStyleSheet("");
}

void WavelengthChatView::onPttGranted(const QString &frequency) {
    if (frequency == currentFrequency && m_pttState == Requesting) {
        qDebug() << "PTT Granted for" << frequency;
        m_pttState = Transmitting;
        updatePttButtonState();
        startAudioInput();
        pttButton->setStyleSheet("background-color: red; color: white;");
        // --- USTAW WSKAŹNIK NADAJĄCEGO (LOKALNIE) ---
        messageArea->SetTransmittingUser("You");
        // --- KONIEC USTAWIANIA WSKAŹNIKA ---
    } else if (m_pttState == Requesting) {
        qDebug() << "Received PTT grant for wrong frequency or state:" << frequency << m_pttState;
        onPttButtonReleased();
    }
}

void WavelengthChatView::onPttDenied(const QString &frequency, const QString &reason) {
    if (frequency == currentFrequency && m_pttState == Requesting) {
        qDebug() << "PTT Denied for" << frequency << ":" << reason;
        // Wyświetl powód odmowy (opcjonalnie)
        // QMessageBox::warning(this, "PTT Denied", reason);
        messageArea->AddMessage(QString("<span style='color:#ffcc00;'>[SYSTEM] Nie można nadawać: %1</span>").arg(reason),
                                "", StreamMessage::kSystem);

        m_pttState = Idle;
        updatePttButtonState();
        pttButton->setStyleSheet(""); // Resetuj styl
    }
}

void WavelengthChatView::onPttStartReceiving(const QString &frequency, const QString &senderId) {
    if (frequency == currentFrequency && m_pttState == Idle) {
        qDebug() << "Starting to receive PTT audio from" << senderId << "on" << frequency;
        m_pttState = Receiving;
        updatePttButtonState();
        startAudioOutput();
        // --- USTAW WSKAŹNIK NADAJĄCEGO (ZDALNIE) ---
        m_currentTransmitterId = senderId; // Zapisz ID na wszelki wypadek
        messageArea->SetTransmittingUser(senderId);
        // --- KONIEC USTAWIANIA WSKAŹNIKA ---
        if (m_audioOutput) {
            qDebug() << "Audio Output State after start request:" << m_audioOutput->state() << "Error:" << m_audioOutput->error();
        }
    }
}

void WavelengthChatView::onPttStopReceiving(const QString &frequency) {
    if (frequency == currentFrequency && m_pttState == Receiving) {
        qDebug() << "Stopping PTT audio reception on" << frequency;
        stopAudioOutput();
        m_pttState = Idle;
        updatePttButtonState();
        // --- WYCZYŚĆ WSKAŹNIK NADAJĄCEGO ---
        m_currentTransmitterId = "";
        messageArea->ClearTransmittingUser();
        // --- ZRESETUJ AMPLITUDĘ ---
        messageArea->SetAudioAmplitude(0.0);
        // Usunięto reset glitchIntensity
    }
}

void WavelengthChatView::onAudioDataReceived(const QString &frequency, const QByteArray &audioData) const {
    if (frequency == currentFrequency && m_pttState == Receiving) {
        if (m_audioOutput) {
            qDebug() << "[CLIENT] onAudioDataReceived: Current Audio Output State before write:" << m_audioOutput->state();
        } else {
            qWarning() << "[CLIENT] onAudioDataReceived: m_audioOutput is null!";
            return;
        }

        if (m_outputDevice) {
            const qint64 bytesWritten = m_outputDevice->write(audioData);

            if (bytesWritten < 0) {
                qWarning() << "Audio Output Error writing data:" << m_audioOutput->error();
            } else if (bytesWritten != audioData.size()) {
                qWarning() << "Audio Output: Wrote fewer bytes than received:" << bytesWritten << "/" << audioData.size();
            }

            if (bytesWritten > 0) {
                // --- OBLICZ AMPLITUDĘ I ZAKTUALIZUJ WIZUALIZACJĘ ODBIORCY ---
                // Używamy danych, które faktycznie zostały zapisane (lub całego bufora, jeśli zapis się powiódł)
                const qreal amplitude = calculateAmplitude(audioData.left(bytesWritten));
                if (messageArea) {
                    messageArea->SetAudioAmplitude(amplitude); // Bezpośrednia aktualizacja wizualizacji
                }
                // --- KONIEC AKTUALIZACJI WIZUALIZACJI ODBIORCY ---

                // Komentarz: Poniższa linia jest teraz prawdopodobnie zbędna,
                // chyba że sygnał z koordynatora ma inne zastosowanie.
                // updateRemoteAudioAmplitude(frequency, amplitude);
            }
        } else {
            qWarning() << "Audio Output: Received audio data but m_outputDevice is null!";
        }
    }
}

void WavelengthChatView::onReadyReadInput() const {
    // --- DODANE LOGOWANIE ---
    qDebug() << "[HOST] onReadyReadInput triggered!";
    // --- KONIEC DODANIA ---

    if (!m_inputDevice || m_pttState != Transmitting) {
        qDebug() << "[HOST] onReadyReadInput: Guard failed (device null or state not Transmitting)";
        return;
    }

    const QByteArray buffer = m_inputDevice->readAll();
    qDebug() << "[HOST] onReadyReadInput: Read" << buffer.size() << "bytes from input device."; // Loguj rozmiar bufora

    if (!buffer.isEmpty()) {
        // 1. Wyślij dane binarne przez WebSocket
        const bool sent = WavelengthMessageService::GetInstance()->SendAudioData(currentFrequency, buffer);
        // --- DODANE LOGOWANIE ---
        qDebug() << "[HOST] onReadyReadInput: Attempted to send audio data. Success:" << sent;
        // --- KONIEC DODANIA ---


        // 2. Oblicz amplitudę
        const qreal amplitude = calculateAmplitude(buffer);

        // 3. Zaktualizuj wizualizację lokalnie
        if (messageArea) {
            // messageArea->setGlitchIntensity(amplitude * 1.5); // Stara metoda
            messageArea->SetAudioAmplitude(amplitude * 1.5); // Nowa metoda
        }
    } else {
        qDebug() << "[HOST] onReadyReadInput: Buffer was empty.";
    }
}

void WavelengthChatView::updateProgressMessage(const QString &messageId, const QString &message) const {
    messageArea->AddMessage(message, messageId, StreamMessage::MessageType::kSystem);
}

void WavelengthChatView::sendMessage() const {
    const QString message = inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    inputField->clear();

    WavelengthMessageService *manager = WavelengthMessageService::GetInstance();
    manager->SendMessage(message);
}

void WavelengthChatView::abortWavelength() {
    if (currentFrequency == "-1") {
        return;
    }

    m_isAborting = true;

    m_statusIndicator->setText("ROZŁĄCZANIE...");
    m_statusIndicator->setStyleSheet(
        "color: #ffaa55;"
        "background-color: rgba(40, 20, 0, 150);"
        "border: 1px solid #aa7733;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );

    const WavelengthSessionCoordinator *coordinator = WavelengthSessionCoordinator::GetInstance();

    bool isHost = false;
    if (coordinator->GetWavelengthInfo(currentFrequency, &isHost).is_host) {
        if (isHost) {
            coordinator->CloseWavelength(currentFrequency);
        } else {
            coordinator->LeaveWavelength();
        }
    } else {
        coordinator->LeaveWavelength();
    }

    // Emitujemy sygnał przed czyszczeniem widoku
    emit wavelengthAborted();

    // Opóźniamy czyszczenie widoku do momentu zakończenia animacji przejścia
    QTimer::singleShot(250, this, [this]() {
        clear();
        m_isAborting = false;
    });
}

void WavelengthChatView::triggerVisualEffect() {
    // Losowy efekt scanlines co jakiś czas
    const auto anim = new QPropertyAnimation(this, "scanlineOpacity");
    anim->setDuration(800);
    anim->setStartValue(m_scanlineOpacity);
    anim->setKeyValueAt(0.4, 0.3 + QRandomGenerator::global()->bounded(10) / 100.0);
    anim->setEndValue(0.15);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void WavelengthChatView::triggerConnectionEffect() {
    // Efekt przy połączeniu
    const auto anim = new QPropertyAnimation(this, "scanlineOpacity");
    anim->setDuration(1200);
    anim->setStartValue(0.6);
    anim->setEndValue(0.15);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void WavelengthChatView::triggerActivityEffect() {
    // Subtelny efekt przy aktywności
    const auto anim = new QPropertyAnimation(this, "scanlineOpacity");
    anim->setDuration(300);
    anim->setStartValue(m_scanlineOpacity);
    anim->setKeyValueAt(0.3, 0.25);
    anim->setEndValue(0.15);
    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void WavelengthChatView::initializeAudio() {
    m_audioFormat.setSampleRate(16000);       // Przykładowa częstotliwość próbkowania
    m_audioFormat.setChannelCount(1);         // Mono
    m_audioFormat.setSampleSize(16);          // 16 bitów na próbkę
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);

    const QAudioDeviceInfo inputInfo = QAudioDeviceInfo::defaultInputDevice();
    if (!inputInfo.isFormatSupported(m_audioFormat)) {
        qWarning() << "Domyślny format wejściowy nie jest obsługiwany, próbuję znaleźć najbliższy.";
        m_audioFormat = inputInfo.nearestFormat(m_audioFormat);
    }

    const QAudioDeviceInfo outputInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (!outputInfo.isFormatSupported(m_audioFormat)) {
        qWarning() << "Domyślny format wyjściowy nie jest obsługiwany, próbuję znaleźć najbliższy.";
        m_audioFormat = outputInfo.nearestFormat(m_audioFormat);
    }

    // Tworzymy obiekty, ale nie startujemy ich jeszcze
    m_audioInput = new QAudioInput(inputInfo, m_audioFormat, this);
    m_audioOutput = new QAudioOutput(outputInfo, m_audioFormat, this);

    // Ustawienie bufora dla płynniejszego odtwarzania/nagrywania
    m_audioInput->setBufferSize(4096); // Rozmiar bufora wejściowego
    m_audioOutput->setBufferSize(8192); // Większy bufor wyjściowy dla kompensacji opóźnień sieciowych

    qDebug() << "Audio initialized with format:" << m_audioFormat;
}

void WavelengthChatView::startAudioInput() {
    if (!m_audioInput) {
        qWarning() << "[HOST] Audio Input: Cannot start, m_audioInput is null!";
        return;
    }
    if (m_pttState != Transmitting) {
        qDebug() << "[HOST] Audio Input: Not starting, state is not Transmitting.";
        return;
    }
    if (m_audioInput->state() != QAudio::StoppedState) {
        qDebug() << "[HOST] Audio Input: Already running or stopping. State:" << m_audioInput->state();
        return;
    }

    qDebug() << "[HOST] Audio Input: Attempting to start...";
    m_inputDevice = m_audioInput->start(); // Rozpocznij przechwytywanie
    if (m_inputDevice) {
        connect(m_inputDevice, &QIODevice::readyRead, this, &WavelengthChatView::onReadyReadInput);
        // --- DODANE LOGOWANIE ---
        qDebug() << "[HOST] Audio Input: Started successfully. State:" << m_audioInput->state() << "Error:" << m_audioInput->error();
        // --- KONIEC DODANIA ---
    } else {
        qWarning() << "[HOST] Audio Input: Failed to start! State:" << m_audioInput->state() << "Error:" << m_audioInput->error();
        onPttButtonReleased();
    }
}

void WavelengthChatView::stopAudioInput() {
    if (m_audioInput && m_audioInput->state() != QAudio::StoppedState) {
        m_audioInput->stop();
        if (m_inputDevice) {
            disconnect(m_inputDevice, &QIODevice::readyRead, this, &WavelengthChatView::onReadyReadInput);
            m_inputDevice = nullptr; // QAudioInput zarządza jego usunięciem
        }
        qDebug() << "Audio input stopped.";
    }
    // Resetuj wizualizację po zatrzymaniu nadawania
    if (messageArea && m_pttState != Receiving) { // Nie resetuj jeśli zaraz zaczniemy odbierać
        messageArea->SetGlitchIntensity(0.0);
    }
}

void WavelengthChatView::startAudioOutput() {
    if (!m_audioOutput) {
        qWarning() << "Audio Output: Cannot start, m_audioOutput is null!";
        return;
    }
    if (m_pttState != Receiving) {
        qDebug() << "Audio Output: Not starting, state is not Receiving.";
        return;
    }
    if (m_audioOutput->state() != QAudio::StoppedState && m_audioOutput->state() != QAudio::IdleState) {
        qDebug() << "Audio Output: Already running or in an intermediate state:" << m_audioOutput->state();
        // Jeśli jest już aktywny, upewnij się, że m_outputDevice jest ustawiony
        if (!m_outputDevice && m_audioOutput->state() == QAudio::ActiveState) {
            // To nie powinno się zdarzyć, ale na wszelki wypadek
            // m_outputDevice = m_audioOutput->start(); // Ponowne startowanie może być problematyczne
            qWarning() << "Audio Output: Active state but m_outputDevice is null. Potential issue.";
        }
        return;
    }

    qDebug() << "Audio Output: Attempting to start...";
    m_outputDevice = m_audioOutput->start(); // Rozpocznij odtwarzanie (przygotuj urządzenie)
    if(m_outputDevice) {
        qDebug() << "Audio Output: Started successfully. State:" << m_audioOutput->state();
    } else {
        qWarning() << "Audio Output: Failed to start! State:" << m_audioOutput->state() << "Error:" << m_audioOutput->error();
        // Obsłuż błąd - np. wróć do stanu Idle
        // Symulujemy zatrzymanie, aby zresetować stan
        onPttStopReceiving(currentFrequency);
    }
}

void WavelengthChatView::stopAudioOutput() {
    if (m_audioOutput && m_audioOutput->state() != QAudio::StoppedState) {
        qDebug() << "Audio Output: Stopping... Current state:" << m_audioOutput->state();
        m_audioOutput->stop();
        m_outputDevice = nullptr; // QAudioOutput zarządza jego usunięciem
        qDebug() << "Audio Output: Stopped. State:" << m_audioOutput->state();
    } else if (m_audioOutput) {
        qDebug() << "Audio Output: Already stopped. State:" << m_audioOutput->state();
    }
    // Resetuj wizualizację po zatrzymaniu odbierania
    if (messageArea) {
        messageArea->SetGlitchIntensity(0.0);
    }
}

qreal WavelengthChatView::calculateAmplitude(const QByteArray &buffer) const {
    if (buffer.isEmpty() || m_audioFormat.sampleSize() != 16 || m_audioFormat.sampleType() != QAudioFormat::SignedInt) {
        return 0.0;
    }

    const auto data = reinterpret_cast<const qint16*>(buffer.constData());
    const int sampleCount = buffer.size() / (m_audioFormat.sampleSize() / 8);
    if (sampleCount == 0) return 0.0;

    double sumOfSquares = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        // Normalizuj próbkę do zakresu [-1.0, 1.0]
        const double normalizedSample = static_cast<double>(data[i]) / 32767.0;
        sumOfSquares += normalizedSample * normalizedSample;
    }

    const double meanSquare = sumOfSquares / sampleCount;
    const double rms = std::sqrt(meanSquare);

    // Zwróć wartość RMS (zwykle między 0.0 a ~0.7, rzadko 1.0)
    // Można przeskalować, jeśli potrzebny jest większy zakres dla glitchIntensity
    return static_cast<qreal>(rms);
}

void WavelengthChatView::updatePttButtonState() const {
    switch (m_pttState) {
        case Idle:
            pttButton->setEnabled(true);
            pttButton->setText("MÓW");
            inputField->setEnabled(true); // Włącz inne kontrolki
            sendButton->setEnabled(true);
            attachButton->setEnabled(true);
            break;
        case Requesting:
            pttButton->setEnabled(true); // Nadal można go zwolnić
            pttButton->setText("..."); // Wskaźnik oczekiwania
            inputField->setEnabled(false); // Wyłącz inne kontrolki
            sendButton->setEnabled(false);
            attachButton->setEnabled(false);
            break;
        case Transmitting:
            pttButton->setEnabled(true); // Nadal można go zwolnić
            pttButton->setText("NADAJĘ");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
            attachButton->setEnabled(false);
            break;
        case Receiving:
            pttButton->setEnabled(false); // Nie można nacisnąć podczas odbierania
            pttButton->setText("ODBIÓR");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
            attachButton->setEnabled(false);
            break;
    }
}

void WavelengthChatView::resetStatusIndicator() const {
    m_statusIndicator->setText("AKTYWNE POŁĄCZENIE");
    m_statusIndicator->setStyleSheet(
        "color: #00ffaa;"
        "background-color: rgba(0, 40, 30, 150);"
        "border: 1px solid #00aa77;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );
}
