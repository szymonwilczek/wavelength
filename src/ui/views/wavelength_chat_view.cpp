#include "wavelength_chat_view.h"

#include "../chat/style/chat_style.h"
#include <QFileDialog>

#include "../../app/managers/translation_manager.h"

WavelengthChatView::WavelengthChatView(QWidget *parent): QWidget(parent), scanline_opacity_(0.15) {
    setObjectName("chatViewContainer");

    translator_ = TranslationManager::GetInstance();
    // Podstawowy layout
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(15);

    // Nagłówek z dodatkowym efektem
    const auto header_layout = new QHBoxLayout();
    header_layout->setContentsMargins(0, 0, 0, 0);

    header_label_ = new QLabel(this);
    header_label_->setObjectName("headerLabel");
    header_layout->addWidget(header_label_);

    header_layout->addStretch(1);

    // Wskaźnik status połączenia
    status_indicator_ = new QLabel(
    translator_->Translate("ChatView.ConnectionActive", "CONNECTION ACTIVE"),
        this);
    status_indicator_->setAlignment(Qt::AlignVCenter);
    status_indicator_->setStyleSheet(
        "color: #00ffaa;"
        "background-color: rgba(0, 40, 30, 150);"
        "border: 1px solid #00aa77;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
        "padding: 4px 8px;"
    );
    status_indicator_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    header_layout->addWidget(status_indicator_);

    main_layout->addLayout(header_layout);

    // Obszar wiadomości
    message_area_ = new WavelengthStreamDisplay(this);
    message_area_->setStyleSheet(ChatStyle::GetScrollBarStyle());
    main_layout->addWidget(message_area_, 1);

    // Panel informacji technicznych
    const auto info_panel = new QWidget(this);
    const auto info_layout = new QHBoxLayout(info_panel);
    info_layout->setContentsMargins(3, 3, 3, 3);
    info_layout->setSpacing(10);

    // Techniczne etykiety w stylu cyberpunk
    const QString style_sheet =
            "color: #00aaff;"
            "background-color: transparent;"
            "font-family: 'Consolas';"
            "font-size: 8pt;";

    const auto session_label = new QLabel(QString("SID:%1").arg(QRandomGenerator::global()->bounded(1000, 9999)), this);
    session_label->setStyleSheet(style_sheet);
    info_layout->addWidget(session_label);

    const auto buffer_label = new QLabel(
        translator_->Translate("ChatView.Buffer100", "BUFFER:100%"),
        this);
    buffer_label->setStyleSheet(style_sheet);
    info_layout->addWidget(buffer_label);

    info_layout->addStretch();

    auto time_label = new QLabel(QDateTime::currentDateTime().toString("HH:mm:ss"), this);
    time_label->setStyleSheet(style_sheet);
    info_layout->addWidget(time_label);

    // Timer dla aktualnego czasu
    const auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [time_label]() {
        time_label->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
    });
    timer->start(1000);

    main_layout->addWidget(info_panel);

    // Panel wprowadzania wiadomości
    const auto input_layout = new QHBoxLayout();
    input_layout->setSpacing(10);

    // Przycisk załączania plików - niestandardowy wygląd
    attach_button_ = new CyberChatButton("📎", this);
    attach_button_->setToolTip(
        translator_->Translate("ChatView.AttachFile", "Attach file")
        );
    attach_button_->setFixedSize(36, 36);
    input_layout->addWidget(attach_button_);

    // Pole wprowadzania wiadomości
    input_field_ = new QLineEdit(this);
    input_field_->setObjectName("chatInputField");
    input_field_->setPlaceholderText("Aa");
    input_field_->setStyleSheet(ChatStyle::GetInputFieldStyle());
    input_layout->addWidget(input_field_, 1);

    // Przycisk wysyłania - niestandardowy wygląd
    send_button_ = new CyberChatButton(
        translator_->Translate("ChatView.SendButton", "Send"),
        this);
    send_button_->setMinimumWidth(80);
    send_button_->setFixedHeight(36);
    input_layout->addWidget(send_button_);

    // --- NOWY PRZYCISK PTT ---
    ptt_button_ = new CyberChatButton(
        translator_->Translate("ChatView.TalkButton", "TALK"),
        this);
    ptt_button_->setToolTip(
        translator_->Translate("ChatView.TalkTooltip", "Click and press to talk.")
        );
    ptt_button_->setCheckable(false); // Działa na press/release
    connect(ptt_button_, &QPushButton::pressed, this, &WavelengthChatView::OnPttButtonPressed);
    connect(ptt_button_, &QPushButton::released, this, &WavelengthChatView::OnPttButtonReleased);
    input_layout->addWidget(ptt_button_);
    // --- KONIEC NOWEGO PRZYCISKU PTT ---

    main_layout->addLayout(input_layout);

    // Przycisk przerwania połączenia
    abort_button_ = new CyberChatButton(
        translator_->Translate("ChatView.AbortConnection", "ABORT CONNECTION"),
        this);
    abort_button_->setStyleSheet(
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
    main_layout->addWidget(abort_button_);

    InitializeAudio();

    ptt_on_sound_ = new QSoundEffect(this);
    ptt_on_sound_->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/ptt_on.wav"));
    ptt_on_sound_->setVolume(0.8);

    ptt_off_sound_ = new QSoundEffect(this);
    ptt_off_sound_->setSource(QUrl::fromLocalFile(":/resources/sounds/interface/ptt_off.wav"));
    ptt_off_sound_->setVolume(0.8);

    // Połączenia sygnałów
    connect(input_field_, &QLineEdit::returnPressed, this, &WavelengthChatView::SendMessage);
    connect(send_button_, &QPushButton::clicked, this, &WavelengthChatView::SendMessage);
    connect(attach_button_, &QPushButton::clicked, this, &WavelengthChatView::AttachFile);
    connect(abort_button_, &QPushButton::clicked, this, &WavelengthChatView::AbortWavelength);

    const WavelengthMessageService *message_service = WavelengthMessageService::GetInstance();
    connect(message_service, &WavelengthMessageService::progressMessageUpdated,
            this, &WavelengthChatView::UpdateProgressMessage);

    const WavelengthSessionCoordinator* coordinator = WavelengthSessionCoordinator::GetInstance();
    connect(coordinator, &WavelengthSessionCoordinator::pttGranted, this, &WavelengthChatView::OnPttGranted);
    connect(coordinator, &WavelengthSessionCoordinator::pttDenied, this, &WavelengthChatView::OnPttDenied);
    connect(coordinator, &WavelengthSessionCoordinator::pttStartReceiving, this, &WavelengthChatView::OnPttStartReceiving);
    connect(coordinator, &WavelengthSessionCoordinator::pttStopReceiving, this, &WavelengthChatView::OnPttStopReceiving);
    connect(coordinator, &WavelengthSessionCoordinator::audioDataReceived, this, &WavelengthChatView::OnAudioDataReceived);

    const auto glitch_timer = new QTimer(this);
    connect(glitch_timer, &QTimer::timeout, this, &WavelengthChatView::TriggerVisualEffect);
    glitch_timer->start(5000 + QRandomGenerator::global()->bounded(5000));

    QWidget::setVisible(false);
}

WavelengthChatView::~WavelengthChatView() {
    StopAudioInput();
    StopAudioOutput();
    delete audio_input_;
    delete audio_output_;
}

void WavelengthChatView::SetScanlineOpacity(const double opacity) {
    scanline_opacity_ = opacity;
    update();
}

void WavelengthChatView::SetWavelength(const QString &frequency, const QString &name) {
    current_frequency_ = frequency;

    ResetStatusIndicator();

    QString title;
    if (name.isEmpty()) {
        title = QString("WVLNGTH: %1 Hz").arg(frequency);
    } else {
        title = QString("%1 (%2 Hz)").arg(name).arg(frequency);
    }
    header_label_->setText(title);

    message_area_->Clear();

    const QString welcome_message = QString("<span style=\"color:#ffcc00;\">%1 %2 Hz @ %3</span>")
        .arg(translator_->Translate("ChatView.ConnectedToWavelength", "Connected to wavelength"))
            .arg(frequency, QDateTime::currentDateTime().toString("HH:mm:ss"));
    message_area_->AddMessage(welcome_message, "system", StreamMessage::MessageType::kSystem);

    TriggerConnectionEffect();

    setVisible(true);
    input_field_->setFocus();
    input_field_->clear();
    ptt_button_->setEnabled(true);
    abort_button_->setEnabled(true);
    ResetStatusIndicator();
    is_aborting_ = false;
    // Zatrzymaj PTT jeśli było aktywne
    if (ptt_state_ == Transmitting) {
        OnPttButtonReleased();
    }
    ptt_state_ = Idle;
    UpdatePttButtonState();
}

void WavelengthChatView::OnMessageReceived(const QString &frequency, const QString &message) {
    if (frequency != current_frequency_) return;
    QTimer::singleShot(0, this, [this, message]() {
        message_area_->AddMessage(message, QString(), StreamMessage::MessageType::kReceived);
        TriggerActivityEffect();
    });
}

void WavelengthChatView::OnMessageSent(const QString &frequency, const QString &message) const {
    if (frequency != current_frequency_) return;
    message_area_->AddMessage(message, QString(), StreamMessage::MessageType::kTransmitted);
}

void WavelengthChatView::OnWavelengthClosed(const QString &frequency) {
    if (frequency != current_frequency_ || is_aborting_) {
        return;
    }

    status_indicator_->setText(
        translator_->Translate("ChatView.ConnectionClosed", "CONNECTION CLOSED")
        );
    status_indicator_->setStyleSheet(
        "color: #ff5555;"
        "background-color: rgba(40, 0, 0, 150);"
        "border: 1px solid #aa3333;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );

    const auto close_message = QString("<span style=\"color:#ff5555;\">Wavelength has been closed by host.</span>");
    message_area_->AddMessage(close_message, "system", StreamMessage::MessageType::kSystem);

    QTimer::singleShot(2000, this, [this]() {
        Clear();
        emit wavelengthAborted();
    });
}

void WavelengthChatView::Clear() {
    current_frequency_ = -1;
    message_area_->Clear();
    header_label_->clear();
    input_field_->clear();
    setVisible(false);
}

void WavelengthChatView::AttachFile() {
    const QString file_path = QFileDialog::getOpenFileName(this,
                                                    translator_->Translate("ChatView.SelectFileToAttach", "Select File to Attach"),
                                                    QString(),
                                                    "Media Files (*.jpg *.jpeg *.png *.gif *.mp3 *.mp4 *.wav);;All Files (*)");

    if (file_path.isEmpty()) {
        return;
    }

    // Pobieramy nazwę pliku
    const QFileInfo file_info(file_path);
    const QString filename = file_info.fileName();

    // Generujemy identyfikator dla komunikatu
    const QString progress_message_id = "file_progress_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    // Wyświetlamy początkowy komunikat
    const QString processing_message = QString("<span style=\"color:#888888;\">%1: %2...</span>")
            .arg(translator_->Translate("ChatView.SendingFile", "Sending file"))
            .arg(filename);

    message_area_->AddMessage(processing_message, progress_message_id, StreamMessage::MessageType::kTransmitted);

    // Uruchamiamy asynchroniczny proces przetwarzania pliku
    WavelengthMessageService *service = WavelengthMessageService::GetInstance();
    const bool started = service->SendFile(file_path, progress_message_id);

    if (!started) {
        message_area_->AddMessage(progress_message_id,
                                QString("<span style=\"color:#ff5555;\">%1</span>")
                                    .arg(translator_->Translate("ChatView.FailedToStartFileProcessing", "Failed to start file processing.")),
                                StreamMessage::MessageType::kTransmitted);
    }
}

void WavelengthChatView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
}

void WavelengthChatView::OnPttButtonPressed() {
    // --- ODTWÓRZ DŹWIĘK NACIŚNIĘCIA ---
    if (ptt_on_sound_->isLoaded()) {
        ptt_on_sound_->play();
    } else {
        qWarning() << "PTT On sound not loaded!";
    }
    // --- KONIEC ODTWARZANIA ---

    if (current_frequency_ == "-1.0" || ptt_state_ != Idle) {
        return;
    }
    qDebug() << "PTT Button Pressed - Requesting PTT for" << current_frequency_;
    ptt_state_ = Requesting;
    UpdatePttButtonState();
    WavelengthMessageService::GetInstance()->SendPttRequest(current_frequency_);
    ptt_button_->setStyleSheet("background-color: yellow; color: black;");
}

void WavelengthChatView::OnPttButtonReleased() {
    // --- ODTWÓRZ DŹWIĘK ZWOLNIENIA ---
    // Odtwórz dźwięk niezależnie od tego, czy faktycznie nadawaliśmy
    if (ptt_off_sound_->isLoaded()) {
        ptt_off_sound_->play();
    } else {
        qWarning() << "PTT Off sound not loaded!";
    }
    // --- KONIEC ODTWARZANIA ---

    if (ptt_state_ == Transmitting) {
        qDebug() << "PTT Button Released - Stopping Transmission for" << current_frequency_;
        StopAudioInput();
        WavelengthMessageService::GetInstance()->SendPttRelease(current_frequency_);
        message_area_->ClearTransmittingUser();
        message_area_->SetAudioAmplitude(0.0);
    } else if (ptt_state_ == Requesting) {
        qDebug() << "PTT Button Released - Cancelling Request for" << current_frequency_;
        // Można dodać wysłanie anulowania żądania, jeśli serwer to obsługuje
    }
    ptt_state_ = Idle;
    UpdatePttButtonState();
    ptt_button_->setStyleSheet("");
}

void WavelengthChatView::OnPttGranted(const QString &frequency) {
    if (frequency == current_frequency_ && ptt_state_ == Requesting) {
        qDebug() << "PTT Granted for" << frequency;
        ptt_state_ = Transmitting;
        UpdatePttButtonState();
        StartAudioInput();
        ptt_button_->setStyleSheet("background-color: red; color: white;");
        // --- USTAW WSKAŹNIK NADAJĄCEGO (LOKALNIE) ---
        message_area_->SetTransmittingUser(
            translator_->Translate("ChatView.PttYou", "You")
            );
        // --- KONIEC USTAWIANIA WSKAŹNIKA ---
    } else if (ptt_state_ == Requesting) {
        qDebug() << "Received PTT grant for wrong frequency or state:" << frequency << ptt_state_;
        OnPttButtonReleased();
    }
}

void WavelengthChatView::OnPttDenied(const QString &frequency, const QString &reason) {
    if (frequency == current_frequency_ && ptt_state_ == Requesting) {
        qDebug() << "PTT Denied for" << frequency << ":" << reason;
        // Wyświetl powód odmowy (opcjonalnie)
        // QMessageBox::warning(this, "PTT Denied", reason);
        message_area_->AddMessage(QString("<span style='color:#ffcc00;'>%1: %2</span>")
        .arg(translator_->Translate("ChatView.PttDenied", "[SYSTEM] Cannot start broadcasting"))
        .arg(reason),
                                "", StreamMessage::kSystem);

        ptt_state_ = Idle;
        UpdatePttButtonState();
        ptt_button_->setStyleSheet(""); // Resetuj styl
    }
}

void WavelengthChatView::OnPttStartReceiving(const QString &frequency, const QString &sender_id) {
    if (frequency == current_frequency_ && ptt_state_ == Idle) {
        qDebug() << "Starting to receive PTT audio from" << sender_id << "on" << frequency;
        ptt_state_ = Receiving;
        UpdatePttButtonState();
        StartAudioOutput();
        // --- USTAW WSKAŹNIK NADAJĄCEGO (ZDALNIE) ---
        current_transmitter_id_ = sender_id; // Zapisz ID na wszelki wypadek
        message_area_->SetTransmittingUser(sender_id);
        // --- KONIEC USTAWIANIA WSKAŹNIKA ---
        if (audio_output_) {
            qDebug() << "Audio Output State after start request:" << audio_output_->state() << "Error:" << audio_output_->error();
        }
    }
}

void WavelengthChatView::OnPttStopReceiving(const QString &frequency) {
    if (frequency == current_frequency_ && ptt_state_ == Receiving) {
        qDebug() << "Stopping PTT audio reception on" << frequency;
        StopAudioOutput();
        ptt_state_ = Idle;
        UpdatePttButtonState();
        // --- WYCZYŚĆ WSKAŹNIK NADAJĄCEGO ---
        current_transmitter_id_ = "";
        message_area_->ClearTransmittingUser();
        // --- ZRESETUJ AMPLITUDĘ ---
        message_area_->SetAudioAmplitude(0.0);
        // Usunięto reset glitchIntensity
    }
}

void WavelengthChatView::OnAudioDataReceived(const QString &frequency, const QByteArray &audio_data) const {
    if (frequency == current_frequency_ && ptt_state_ == Receiving) {
        if (audio_output_) {
            qDebug() << "[CLIENT] onAudioDataReceived: Current Audio Output State before write:" << audio_output_->state();
        } else {
            qWarning() << "[CLIENT] onAudioDataReceived: m_audioOutput is null!";
            return;
        }

        if (output_device_) {
            const qint64 bytes_written = output_device_->write(audio_data);

            if (bytes_written < 0) {
                qWarning() << "Audio Output Error writing data:" << audio_output_->error();
            } else if (bytes_written != audio_data.size()) {
                qWarning() << "Audio Output: Wrote fewer bytes than received:" << bytes_written << "/" << audio_data.size();
            }

            if (bytes_written > 0) {
                // --- OBLICZ AMPLITUDĘ I ZAKTUALIZUJ WIZUALIZACJĘ ODBIORCY ---
                // Używamy danych, które faktycznie zostały zapisane (lub całego bufora, jeśli zapis się powiódł)
                const qreal amplitude = CalculateAmplitude(audio_data.left(bytes_written));
                if (message_area_) {
                    message_area_->SetAudioAmplitude(amplitude); // Bezpośrednia aktualizacja wizualizacji
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

void WavelengthChatView::OnReadyReadInput() const {
    qDebug() << "[HOST] onReadyReadInput triggered! Current ptt_state_:" << ptt_state_
             << "audio_input_ state:" << (audio_input_ ? QString::number(audio_input_->state()) : "null")
             << "input_device_:" << static_cast<void *>(input_device_);

    // Krok 1: Sprawdź ogólny stan audio_input_ i ptt_state_
    // Jeśli PTT nie jest w stanie nadawania, lub audio_input_ nie istnieje/jest zatrzymany, nie przetwarzaj.
    if (ptt_state_ != Transmitting || !audio_input_ || audio_input_->state() == QAudio::StoppedState) {
        qDebug() << "[HOST] onReadyReadInput: Guard failed (ptt_state_ not Transmitting, audio_input_ null or stopped). ptt_state_:" << ptt_state_
                 << "audio_input_ state:" << (audio_input_ ? QString::number(audio_input_->state()) : "null");
        return;
    }

    // Krok 2: Sprawdź konkretnie input_device_
    // To jest kluczowe zabezpieczenie przed użyciem nieprawidłowego wskaźnika.
    if (!input_device_) {
        qDebug() << "[HOST] onReadyReadInput: Guard failed (input_device_ is null).";
        return;
    }

    // Dodatkowe sprawdzenie, czy urządzenie jest otwarte. Chociaż readyRead powinno to implikować.
    // Na niektórych platformach/sytuacjach readAll() na zamkniętym (ale nie-null) urządzeniu może crashować.
    if (!input_device_->isOpen() || !input_device_->isReadable()) {
        qWarning() << "[HOST] onReadyReadInput: input_device_ is not open or not readable. isOpen:" << input_device_->isOpen() << "isReadable:" << input_device_->isReadable();
        return;
    }

    const QByteArray buffer = input_device_->readAll();
    qDebug() << "[HOST] onReadyReadInput: Read" << buffer.size() << "bytes from input device.";

    if (!buffer.isEmpty()) {
        const bool sent = WavelengthMessageService::GetInstance()->SendAudioData(current_frequency_, buffer);
        qDebug() << "[HOST] onReadyReadInput: Attempted to send audio data. Success:" << sent;

        const qreal amplitude = CalculateAmplitude(buffer);
        if (message_area_) {
            message_area_->SetAudioAmplitude(amplitude * 1.5);
        }
    } else {
        qDebug() << "[HOST] onReadyReadInput: Buffer was empty (read 0 bytes, or device closed/error between check and read). audio_input_ error:" << (audio_input_ ? audio_input_->error() : -1) ;
    }
}

void WavelengthChatView::UpdateProgressMessage(const QString &message_id, const QString &message) const {
    message_area_->AddMessage(message, message_id, StreamMessage::MessageType::kSystem);
}

void WavelengthChatView::SendMessage() const {
    const QString message = input_field_->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    input_field_->clear();

    WavelengthMessageService *manager = WavelengthMessageService::GetInstance();
    manager->SendTextMessage(message);
}

void WavelengthChatView::AbortWavelength() {
    if (current_frequency_ == "-1") {
        return;
    }

    is_aborting_ = true;

    status_indicator_->setText(
        translator_->Translate("ChatView.Aborting", "ABORTING...")
        );
    status_indicator_->setStyleSheet(
        "color: #ffaa55;"
        "background-color: rgba(40, 20, 0, 150);"
        "border: 1px solid #aa7733;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );

    const WavelengthSessionCoordinator *coordinator = WavelengthSessionCoordinator::GetInstance();

    bool isHost = false;
    if (coordinator->GetWavelengthInfo(current_frequency_, &isHost).is_host) {
        if (isHost) {
            coordinator->CloseWavelength(current_frequency_);
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
        Clear();
        is_aborting_ = false;
    });
}

void WavelengthChatView::TriggerVisualEffect() {
    // Losowy efekt scanlines co jakiś czas
    const auto animation = new QPropertyAnimation(this, "scanlineOpacity");
    animation->setDuration(800);
    animation->setStartValue(scanline_opacity_);
    animation->setKeyValueAt(0.4, 0.3 + QRandomGenerator::global()->bounded(10) / 100.0);
    animation->setEndValue(0.15);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void WavelengthChatView::TriggerConnectionEffect() {
    // Efekt przy połączeniu
    const auto animation = new QPropertyAnimation(this, "scanlineOpacity");
    animation->setDuration(1200);
    animation->setStartValue(0.6);
    animation->setEndValue(0.15);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void WavelengthChatView::TriggerActivityEffect() {
    // Subtelny efekt przy aktywności
    const auto animation = new QPropertyAnimation(this, "scanlineOpacity");
    animation->setDuration(300);
    animation->setStartValue(scanline_opacity_);
    animation->setKeyValueAt(0.3, 0.25);
    animation->setEndValue(0.15);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void WavelengthChatView::InitializeAudio() {
    audio_format_.setSampleRate(16000);       // Przykładowa częstotliwość próbkowania
    audio_format_.setChannelCount(1);         // Mono
    audio_format_.setSampleSize(16);          // 16 bitów na próbkę
    audio_format_.setCodec("audio/pcm");
    audio_format_.setByteOrder(QAudioFormat::LittleEndian);
    audio_format_.setSampleType(QAudioFormat::SignedInt);

    const QAudioDeviceInfo input_info = QAudioDeviceInfo::defaultInputDevice();
    if (!input_info.isFormatSupported(audio_format_)) {
        qWarning() << "Domyślny format wejściowy nie jest obsługiwany, próbuję znaleźć najbliższy.";
        audio_format_ = input_info.nearestFormat(audio_format_);
    }

    const QAudioDeviceInfo outputInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (!outputInfo.isFormatSupported(audio_format_)) {
        qWarning() << "Domyślny format wyjściowy nie jest obsługiwany, próbuję znaleźć najbliższy.";
        audio_format_ = outputInfo.nearestFormat(audio_format_);
    }

    // Tworzymy obiekty, ale nie startujemy ich jeszcze
    audio_input_ = new QAudioInput(input_info, audio_format_, this);
    audio_output_ = new QAudioOutput(outputInfo, audio_format_, this);

    // Ustawienie bufora dla płynniejszego odtwarzania/nagrywania
    audio_input_->setBufferSize(4096); // Rozmiar bufora wejściowego
    audio_output_->setBufferSize(8192); // Większy bufor wyjściowy dla kompensacji opóźnień sieciowych

    qDebug() << "Audio initialized with format:" << audio_format_;
}

void WavelengthChatView::StartAudioInput() {
    if (!audio_input_) {
        qWarning() << "[HOST] Audio Input: Cannot start, m_audioInput is null!";
        return;
    }
    if (ptt_state_ != Transmitting) {
        qDebug() << "[HOST] Audio Input: Not starting, state is not Transmitting.";
        return;
    }
    if (audio_input_->state() != QAudio::StoppedState) {
        qDebug() << "[HOST] Audio Input: Already running or stopping. State:" << audio_input_->state();
        return;
    }

    qDebug() << "[HOST] Audio Input: Attempting to start...";
    input_device_ = audio_input_->start(); // Rozpocznij przechwytywanie
    if (input_device_) {
        connect(input_device_, &QIODevice::readyRead, this, &WavelengthChatView::OnReadyReadInput);
        // --- DODANE LOGOWANIE ---
        qDebug() << "[HOST] Audio Input: Started successfully. State:" << audio_input_->state() << "Error:" << audio_input_->error();
        // --- KONIEC DODANIA ---
    } else {
        qWarning() << "[HOST] Audio Input: Failed to start! State:" << audio_input_->state() << "Error:" << audio_input_->error();
        OnPttButtonReleased();
    }
}

void WavelengthChatView::StopAudioInput() {
    if (audio_input_ && (audio_input_->state() == QAudio::ActiveState || audio_input_->state() == QAudio::IdleState)) {
        qDebug() << "[HOST] StopAudioInput: Attempting to stop. Current audio_input_ state:" << audio_input_->state();

        // Krok 1: Rozłącz sygnał od input_device_, jeśli istnieje i jest połączony.
        // To zapobiegnie nowym wywołaniom OnReadyReadInput po tym punkcie dla tego urządzenia.
        if (input_device_) {
            // disconnect zwraca true, jeśli połączenie zostało pomyślnie usunięte.
            bool disconnected = disconnect(input_device_, &QIODevice::readyRead, this, &WavelengthChatView::OnReadyReadInput);
            qDebug() << "[HOST] StopAudioInput: Disconnected readyRead from input_device_:" << disconnected;
        }

        // Krok 2: Zatrzymaj QAudioInput.
        // To powinno zatrzymać przepływ danych i unieważnić poprzedni input_device_.
        audio_input_->stop();
        qDebug() << "[HOST] StopAudioInput: audio_input_->stop() called. New audio_input_ state:" << audio_input_->state();

        // Krok 3: Wyzeruj wskaźnik input_device_.
        // Po audio_input_->stop(), poprzedni wskaźnik QIODevice nie powinien być już używany.
        input_device_ = nullptr;

        qDebug() << "[HOST] Audio input stopped and input_device_ nulled.";
    } else if (audio_input_) {
        qDebug() << "[HOST] StopAudioInput: Audio input not in Active or Idle state, or already stopped. Current audio_input_ state:" << audio_input_->state();
        // Jeśli był już zatrzymany, ale input_device_ nie jest nullptr (co nie powinno się zdarzyć przy poprawnej logice),
        // dla bezpieczeństwa również rozłącz i wyzeruj.
        if (input_device_ && audio_input_->state() == QAudio::StoppedState) {
             disconnect(input_device_, &QIODevice::readyRead, this, &WavelengthChatView::OnReadyReadInput); // Na wszelki wypadek
             input_device_ = nullptr;
             qDebug() << "[HOST] StopAudioInput: Cleaned up input_device_ for already stopped audio_input.";
        }
    } else {
        qDebug() << "[HOST] StopAudioInput: audio_input_ is null.";
    }

    // Resetuj wizualizację po zatrzymaniu nadawania
    if (message_area_ && ptt_state_ != Receiving) { // Nie resetuj jeśli zaraz zaczniemy odbierać
        message_area_->SetAudioAmplitude(0.0);
    }
}

void WavelengthChatView::StartAudioOutput() {
    if (!audio_output_) {
        qWarning() << "Audio Output: Cannot start, m_audioOutput is null!";
        return;
    }
    if (ptt_state_ != Receiving) {
        qDebug() << "Audio Output: Not starting, state is not Receiving.";
        return;
    }
    if (audio_output_->state() != QAudio::StoppedState && audio_output_->state() != QAudio::IdleState) {
        qDebug() << "Audio Output: Already running or in an intermediate state:" << audio_output_->state();
        // Jeśli jest już aktywny, upewnij się, że m_outputDevice jest ustawiony
        if (!output_device_ && audio_output_->state() == QAudio::ActiveState) {
            // To nie powinno się zdarzyć, ale na wszelki wypadek
            // m_outputDevice = m_audioOutput->start(); // Ponowne startowanie może być problematyczne
            qWarning() << "Audio Output: Active state but m_outputDevice is null. Potential issue.";
        }
        return;
    }

    qDebug() << "Audio Output: Attempting to start...";
    output_device_ = audio_output_->start(); // Rozpocznij odtwarzanie (przygotuj urządzenie)
    if(output_device_) {
        qDebug() << "Audio Output: Started successfully. State:" << audio_output_->state();
    } else {
        qWarning() << "Audio Output: Failed to start! State:" << audio_output_->state() << "Error:" << audio_output_->error();
        // Obsłuż błąd - np. wróć do stanu Idle
        // Symulujemy zatrzymanie, aby zresetować stan
        OnPttStopReceiving(current_frequency_);
    }
}

void WavelengthChatView::StopAudioOutput() {
    if (audio_output_ && audio_output_->state() != QAudio::StoppedState) {
        qDebug() << "Audio Output: Stopping... Current state:" << audio_output_->state();
        audio_output_->stop();
        output_device_ = nullptr; // QAudioOutput zarządza jego usunięciem
        qDebug() << "Audio Output: Stopped. State:" << audio_output_->state();
    } else if (audio_output_) {
        qDebug() << "Audio Output: Already stopped. State:" << audio_output_->state();
    }
    // Resetuj wizualizację po zatrzymaniu odbierania
    if (message_area_) {
        message_area_->SetGlitchIntensity(0.0);
    }
}

qreal WavelengthChatView::CalculateAmplitude(const QByteArray &buffer) const {
    if (buffer.isEmpty() || audio_format_.sampleSize() != 16 || audio_format_.sampleType() != QAudioFormat::SignedInt) {
        return 0.0;
    }

    const auto data = reinterpret_cast<const qint16*>(buffer.constData());
    const int sample_count = buffer.size() / (audio_format_.sampleSize() / 8);
    if (sample_count == 0) return 0.0;

    double sum_of_squares = 0.0;
    for (int i = 0; i < sample_count; ++i) {
        // Normalizuj próbkę do zakresu [-1.0, 1.0]
        const double normalized_sample = static_cast<double>(data[i]) / 32767.0;
        sum_of_squares += normalized_sample * normalized_sample;
    }

    const double mean_square = sum_of_squares / sample_count;
    const double rms = std::sqrt(mean_square);

    return rms;
}

void WavelengthChatView::UpdatePttButtonState() const {
    switch (ptt_state_) {
        case Idle:
            ptt_button_->setEnabled(true);
            ptt_button_->setText(
                translator_->Translate("ChatView.TalkButton", "TALK")
                );
            input_field_->setEnabled(true); // Włącz inne kontrolki
            send_button_->setEnabled(true);
            attach_button_->setEnabled(true);
            break;
        case Requesting:
            ptt_button_->setEnabled(true); // Nadal można go zwolnić
            ptt_button_->setText("..."); // Wskaźnik oczekiwania
            input_field_->setEnabled(false); // Wyłącz inne kontrolki
            send_button_->setEnabled(false);
            attach_button_->setEnabled(false);
            break;
        case Transmitting:
            ptt_button_->setEnabled(true); // Nadal można go zwolnić
            ptt_button_->setText(
                translator_->Translate("ChatView.PttTransmitting", "BRDCST")
                );
            input_field_->setEnabled(false);
            send_button_->setEnabled(false);
            attach_button_->setEnabled(false);
            break;
        case Receiving:
            ptt_button_->setEnabled(false); // Nie można nacisnąć podczas odbierania
            ptt_button_->setText(
                translator_->Translate("ChatView.PttReceiving", "RCVNG")
                );
            input_field_->setEnabled(false);
            send_button_->setEnabled(false);
            attach_button_->setEnabled(false);
            break;
    }
}

void WavelengthChatView::ResetStatusIndicator() const {
    status_indicator_->setText(
        translator_->Translate("ChatView.ConnectionActive", "CONNECTION ACTIVE")
        );
    status_indicator_->setStyleSheet(
        "color: #00ffaa;"
        "background-color: rgba(0, 40, 30, 150);"
        "border: 1px solid #00aa77;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );
}
