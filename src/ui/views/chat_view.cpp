#include "chat_view.h"

#include "../chat/style/chat_style.h"
#include <QFileDialog>

#include "../../app/managers/translation_manager.h"
#include "../buttons/cyber_chat_button.h"

ChatView::ChatView(QWidget *parent): QWidget(parent), scanline_opacity_(0.15) {
    setObjectName("chatViewContainer");

    translator_ = TranslationManager::GetInstance();

    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(15);

    const auto header_layout = new QHBoxLayout();

    header_layout->setContentsMargins(0, 0, 0, 0);

    header_label_ = new QLabel(this);
    header_label_->setObjectName("headerLabel");

    header_layout->addWidget(header_label_);

    header_layout->addStretch(1);

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

    message_area_ = new WavelengthStreamDisplay(this);
    message_area_->setStyleSheet(ChatStyle::GetScrollBarStyle());
    main_layout->addWidget(message_area_, 1);

    const auto info_panel = new QWidget(this);
    const auto info_layout = new QHBoxLayout(info_panel);
    info_layout->setContentsMargins(3, 3, 3, 3);
    info_layout->setSpacing(10);

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

    const auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [time_label] {
        time_label->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
    });
    timer->start(1000);

    main_layout->addWidget(info_panel);

    const auto input_layout = new QHBoxLayout();
    input_layout->setSpacing(10);

    attach_button_ = new CyberChatButton("📎", this);
    attach_button_->setToolTip(
        translator_->Translate("ChatView.AttachFile", "Attach file")
    );
    attach_button_->setFixedSize(36, 36);
    input_layout->addWidget(attach_button_);

    input_field_ = new QLineEdit(this);
    input_field_->setObjectName("chatInputField");
    input_field_->setPlaceholderText("Aa");
    input_field_->setStyleSheet(ChatStyle::GetInputFieldStyle());
    input_layout->addWidget(input_field_, 1);

    send_button_ = new CyberChatButton(
        translator_->Translate("ChatView.SendButton", "Send"),
        this);
    send_button_->setMinimumWidth(80);
    send_button_->setFixedHeight(36);
    input_layout->addWidget(send_button_);

    ptt_button_ = new CyberChatButton(
        translator_->Translate("ChatView.TalkButton", "TALK"),
        this);
    ptt_button_->setToolTip(
        translator_->Translate("ChatView.TalkTooltip", "Click and press to talk.")
    );
    ptt_button_->setCheckable(false);
    connect(ptt_button_, &QPushButton::pressed, this, &ChatView::OnPttButtonPressed);
    connect(ptt_button_, &QPushButton::released, this, &ChatView::OnPttButtonReleased);
    input_layout->addWidget(ptt_button_);

    main_layout->addLayout(input_layout);

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

    connect(input_field_, &QLineEdit::returnPressed, this, &ChatView::SendMessage);
    connect(send_button_, &QPushButton::clicked, this, &ChatView::SendMessage);
    connect(attach_button_, &QPushButton::clicked, this, &ChatView::AttachFile);
    connect(abort_button_, &QPushButton::clicked, this, &ChatView::AbortWavelength);

    const MessageService *message_service = MessageService::GetInstance();
    connect(message_service, &MessageService::progressMessageUpdated,
            this, &ChatView::UpdateProgressMessage);

    const SessionCoordinator *coordinator = SessionCoordinator::GetInstance();
    connect(coordinator, &SessionCoordinator::pttGranted, this, &ChatView::OnPttGranted);
    connect(coordinator, &SessionCoordinator::pttDenied, this, &ChatView::OnPttDenied);
    connect(coordinator, &SessionCoordinator::pttStartReceiving, this,
            &ChatView::OnPttStartReceiving);
    connect(coordinator, &SessionCoordinator::pttStopReceiving, this,
            &ChatView::OnPttStopReceiving);
    connect(coordinator, &SessionCoordinator::audioDataReceived, this,
            &ChatView::OnAudioDataReceived);

    const auto glitch_timer = new QTimer(this);
    connect(glitch_timer, &QTimer::timeout, this, &ChatView::TriggerVisualEffect);
    glitch_timer->start(5000 + QRandomGenerator::global()->bounded(5000));

    QWidget::setVisible(false);
}

ChatView::~ChatView() {
    StopAudioInput();
    StopAudioOutput();
    delete audio_input_;
    delete audio_output_;
}

void ChatView::SetScanlineOpacity(const double opacity) {
    scanline_opacity_ = opacity;
    update();
}

void ChatView::SetWavelength(const QString &frequency, const QString &name) {
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
    if (ptt_state_ == Transmitting) {
        OnPttButtonReleased();
    }
    ptt_state_ = Idle;
    UpdatePttButtonState();
}

void ChatView::OnMessageReceived(const QString &frequency, const QString &message) {
    if (frequency != current_frequency_) return;
    QTimer::singleShot(0, this, [this, message] {
        message_area_->AddMessage(message, QString(), StreamMessage::MessageType::kReceived);
        TriggerActivityEffect();
    });
}

void ChatView::OnMessageSent(const QString &frequency, const QString &message) const {
    if (frequency != current_frequency_) return;
    message_area_->AddMessage(message, QString(), StreamMessage::MessageType::kTransmitted);
}

void ChatView::OnWavelengthClosed(const QString &frequency) {
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

    QTimer::singleShot(2000, this, [this] {
        Clear();
        emit wavelengthAborted();
    });
}

void ChatView::Clear() {
    current_frequency_ = -1;
    message_area_->Clear();
    header_label_->clear();
    input_field_->clear();
    setVisible(false);
}

void ChatView::AttachFile() {
    const QString file_path = QFileDialog::getOpenFileName(this,
                                                           translator_->Translate(
                                                               "ChatView.SelectFileToAttach", "Select File to Attach"),
                                                           QString(),
                                                           "Media Files (*.jpg *.jpeg *.png *.gif *.mp3 *.mp4 *.wav);;All Files (*)");

    if (file_path.isEmpty()) {
        return;
    }

    const QFileInfo file_info(file_path);
    const QString filename = file_info.fileName();

    const QString progress_message_id = "file_progress_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    const QString processing_message = QString("<span style=\"color:#888888;\">%1: %2...</span>")
            .arg(translator_->Translate("ChatView.SendingFile", "Sending file"))
            .arg(filename);

    message_area_->AddMessage(processing_message, progress_message_id, StreamMessage::MessageType::kTransmitted);

    MessageService *service = MessageService::GetInstance();
    const bool started = service->SendFile(file_path, progress_message_id);

    if (!started) {
        message_area_->AddMessage(progress_message_id,
                                  QString("<span style=\"color:#ff5555;\">%1</span>")
                                  .arg(translator_->Translate("ChatView.FailedToStartFileProcessing",
                                                              "Failed to start file processing.")),
                                  StreamMessage::MessageType::kTransmitted);
    }
}

void ChatView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
}

void ChatView::OnPttButtonPressed() {
    if (ptt_on_sound_->isLoaded()) {
        ptt_on_sound_->play();
    } else {
        qWarning() << "[CHAT VIEW] PTT On sound not loaded!";
    }

    if (current_frequency_ == "-1.0" || ptt_state_ != Idle) {
        return;
    }
    qDebug() << "[CHAT VIEW] PTT Button Pressed - Requesting PTT for" << current_frequency_;
    ptt_state_ = Requesting;
    UpdatePttButtonState();
    MessageService::GetInstance()->SendPttRequest(current_frequency_);
    ptt_button_->setStyleSheet("background-color: yellow; color: black;");
}

void ChatView::OnPttButtonReleased() {
    if (ptt_off_sound_->isLoaded()) {
        ptt_off_sound_->play();
    } else {
        qWarning() << "[CHAT VIEW] PTT Off sound not loaded!";
    }

    if (ptt_state_ == Transmitting) {
        qDebug() << "[CHAT VIEW] PTT Button Released - Stopping Transmission for" << current_frequency_;
        StopAudioInput();
        MessageService::GetInstance()->SendPttRelease(current_frequency_);
        message_area_->ClearTransmittingUser();
        message_area_->SetAudioAmplitude(0.0);
    } else if (ptt_state_ == Requesting) {
        qDebug() << "[CHAT VIEW] PTT Button Released - Cancelling Request for" << current_frequency_;
    }
    ptt_state_ = Idle;
    UpdatePttButtonState();
    ptt_button_->setStyleSheet("");
}

void ChatView::OnPttGranted(const QString &frequency) {
    if (frequency == current_frequency_ && ptt_state_ == Requesting) {
        qDebug() << "[CHAT VIEW] PTT Granted for" << frequency;
        ptt_state_ = Transmitting;
        UpdatePttButtonState();
        StartAudioInput();
        ptt_button_->setStyleSheet("background-color: red; color: white;");
        message_area_->SetTransmittingUser(
            translator_->Translate("ChatView.PttYou", "You")
        );
    } else if (ptt_state_ == Requesting) {
        qDebug() << "[CHAT VIEW] Received PTT grant for wrong frequency or state:" << frequency << ptt_state_;
        OnPttButtonReleased();
    }
}

void ChatView::OnPttDenied(const QString &frequency, const QString &reason) {
    if (frequency == current_frequency_ && ptt_state_ == Requesting) {
        qDebug() << "[CHAT VIEW] PTT Denied for" << frequency << ":" << reason;
        message_area_->AddMessage(QString("<span style='color:#ffcc00;'>%1: %2</span>")
                                  .arg(translator_->Translate("ChatView.PttDenied",
                                                              "[SYSTEM] Cannot start broadcasting"))
                                  .arg(reason),
                                  "", StreamMessage::kSystem);

        ptt_state_ = Idle;
        UpdatePttButtonState();
        ptt_button_->setStyleSheet("");
    }
}

void ChatView::OnPttStartReceiving(const QString &frequency, const QString &sender_id) {
    if (frequency == current_frequency_ && ptt_state_ == Idle) {
        qDebug() << "[CHAT VIEW] Starting to receive PTT audio from" << sender_id << "on" << frequency;
        ptt_state_ = Receiving;
        UpdatePttButtonState();
        StartAudioOutput();
        current_transmitter_id_ = sender_id;
        message_area_->SetTransmittingUser(sender_id);
        if (audio_output_) {
            qDebug() << "[CHAT VIEW] Audio Output State after start request:" << audio_output_->state() << "Error:" <<
                    audio_output_
                    ->error();
        }
    }
}

void ChatView::OnPttStopReceiving(const QString &frequency) {
    if (frequency == current_frequency_ && ptt_state_ == Receiving) {
        qDebug() << "[CHAT VIEW] Stopping PTT audio reception on" << frequency;
        StopAudioOutput();
        ptt_state_ = Idle;
        UpdatePttButtonState();
        current_transmitter_id_ = "";
        message_area_->ClearTransmittingUser();
        message_area_->SetAudioAmplitude(0.0);
    }
}

void ChatView::OnAudioDataReceived(const QString &frequency, const QByteArray &audio_data) const {
    if (frequency == current_frequency_ && ptt_state_ == Receiving) {
        if (audio_output_) {
            qDebug() << "[CHAT VIEW][CLIENT] onAudioDataReceived: Current Audio Output State before write:" <<
                    audio_output_->
                    state();
        } else {
            qWarning() << "[CHAT VIEW][CLIENT] onAudioDataReceived: m_audioOutput is null!";
            return;
        }

        if (output_device_) {
            const qint64 bytes_written = output_device_->write(audio_data);

            if (bytes_written < 0) {
                qWarning() << "[CHAT VIEW] Audio Output Error writing data:" << audio_output_->error();
            } else if (bytes_written != audio_data.size()) {
                qWarning() << "[CHAT VIEW] Audio Output: Wrote fewer bytes than received:" << bytes_written << "/" <<
                        audio_data.
                        size();
            }

            if (bytes_written > 0) {
                const qreal amplitude = CalculateAmplitude(audio_data.left(bytes_written));
                if (message_area_) {
                    message_area_->SetAudioAmplitude(amplitude);
                }
            }
        } else {
            qWarning() << "[CHAT VIEW] Audio Output: Received audio data but m_outputDevice is null!";
        }
    }
}

void ChatView::OnReadyReadInput() const {
    // 1. check the overall status of audio_input_ and ptt_state_
    if (ptt_state_ != Transmitting || !audio_input_ || audio_input_->state() == QAudio::StoppedState) {
        qDebug() <<
                "[CHAT VIEW][HOST] onReadyReadInput: Guard failed (ptt_state_ not Transmitting, audio_input_ null or stopped). ptt_state_:"
                << ptt_state_
                << "audio_input_ state:" << (audio_input_ ? QString::number(audio_input_->state()) : "null");
        return;
    }

    // 2. check specifically input_device_
    if (!input_device_) {
        qDebug() << "[CHAT VIEW][HOST] onReadyReadInput: Guard failed (input_device_ is null).";
        return;
    }

    if (!input_device_->isOpen() || !input_device_->isReadable()) {
        qWarning() << "[CHAT VIEW][HOST] onReadyReadInput: input_device_ is not open or not readable. isOpen:" <<
                input_device_->
                isOpen() << "isReadable:" << input_device_->isReadable();
        return;
    }

    const QByteArray buffer = input_device_->readAll();

    if (!buffer.isEmpty()) {
        const qreal amplitude = CalculateAmplitude(buffer);
        if (message_area_) {
            message_area_->SetAudioAmplitude(amplitude * 1.5);
        }
    }
}

void ChatView::UpdateProgressMessage(const QString &message_id, const QString &message) const {
    message_area_->AddMessage(message, message_id, StreamMessage::MessageType::kSystem);
}

void ChatView::SendMessage() const {
    const QString message = input_field_->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    input_field_->clear();

    MessageService *manager = MessageService::GetInstance();
    manager->SendTextMessage(message);
}

void ChatView::AbortWavelength() {
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

    const SessionCoordinator *coordinator = SessionCoordinator::GetInstance();

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

    emit wavelengthAborted();

    QTimer::singleShot(250, this, [this] {
        Clear();
        is_aborting_ = false;
    });
}

void ChatView::TriggerVisualEffect() {
    const auto animation = new QPropertyAnimation(this, "scanlineOpacity");
    animation->setDuration(800);
    animation->setStartValue(scanline_opacity_);
    animation->setKeyValueAt(0.4, 0.3 + QRandomGenerator::global()->bounded(10) / 100.0);
    animation->setEndValue(0.15);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void ChatView::TriggerConnectionEffect() {
    const auto animation = new QPropertyAnimation(this, "scanlineOpacity");
    animation->setDuration(1200);
    animation->setStartValue(0.6);
    animation->setEndValue(0.15);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void ChatView::TriggerActivityEffect() {
    const auto animation = new QPropertyAnimation(this, "scanlineOpacity");
    animation->setDuration(300);
    animation->setStartValue(scanline_opacity_);
    animation->setKeyValueAt(0.3, 0.25);
    animation->setEndValue(0.15);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void ChatView::InitializeAudio() {
    audio_format_.setSampleRate(16000);
    audio_format_.setChannelCount(1); // Mono
    audio_format_.setSampleSize(16); // 16 bits per sample
    audio_format_.setCodec("audio/pcm");
    audio_format_.setByteOrder(QAudioFormat::LittleEndian);
    audio_format_.setSampleType(QAudioFormat::SignedInt);

    const QAudioDeviceInfo input_info = QAudioDeviceInfo::defaultInputDevice();
    if (!input_info.isFormatSupported(audio_format_)) {
        qWarning() << "[CHAT VIEW] Default input format is not supported, I am trying to find the nearest.";
        audio_format_ = input_info.nearestFormat(audio_format_);
    }

    const QAudioDeviceInfo outputInfo = QAudioDeviceInfo::defaultOutputDevice();
    if (!outputInfo.isFormatSupported(audio_format_)) {
        qWarning() << "[CHAT VIEW] Default output format is not supported, I am trying to find the closest one.";
        audio_format_ = outputInfo.nearestFormat(audio_format_);
    }

    audio_input_ = new QAudioInput(input_info, audio_format_, this);
    audio_output_ = new QAudioOutput(outputInfo, audio_format_, this);

    audio_input_->setBufferSize(4096);
    audio_output_->setBufferSize(8192);

    qDebug() << "[CHAT VIEW] Audio initialized with format:" << audio_format_;
}

void ChatView::StartAudioInput() {
    if (!audio_input_) {
        qWarning() << "[CHAT VIEW][HOST] Audio Input: Cannot start, m_audioInput is null!";
        return;
    }
    if (ptt_state_ != Transmitting) {
        qDebug() << "[CHAT VIEW][HOST] Audio Input: Not starting, state is not Transmitting.";
        return;
    }
    if (audio_input_->state() != QAudio::StoppedState) {
        qDebug() << "[CHAT VIEW][HOST] Audio Input: Already running or stopping. State:" << audio_input_->state();
        return;
    }

    qDebug() << "[CHAT VIEW][HOST] Audio Input: Attempting to start...";
    input_device_ = audio_input_->start();
    if (input_device_) {
        connect(input_device_, &QIODevice::readyRead, this, &ChatView::OnReadyReadInput);
        qDebug() << "[CHAT VIEW][HOST] Audio Input: Started successfully. State:" << audio_input_->state() << "Error:"
                << audio_input_->error();
    } else {
        qWarning() << "[CHAT VIEW][HOST] Audio Input: Failed to start! State:" << audio_input_->state() << "Error:"
                << audio_input_->error();
        OnPttButtonReleased();
    }
}

void ChatView::StopAudioInput() {
    if (audio_input_ && (audio_input_->state() == QAudio::ActiveState || audio_input_->state() == QAudio::IdleState)) {
        // 1. disconnect the signal from input_device_, if it exists and is connected.
        if (input_device_) {
            disconnect(input_device_, &QIODevice::readyRead, this,
                       &ChatView::OnReadyReadInput);
        }

        // 2. stop QAudioInput.
        audio_input_->stop();

        // 3. reset the input_device_ pointer.
        input_device_ = nullptr;
    } else if (audio_input_) {
        qDebug() <<
                "[CHAT VIEW][HOST] StopAudioInput: Audio input not in Active or Idle state, or already stopped. Current audio_input_ state:"
                << audio_input_->state();

        if (input_device_ && audio_input_->state() == QAudio::StoppedState) {
            disconnect(input_device_, &QIODevice::readyRead, this, &ChatView::OnReadyReadInput);
            input_device_ = nullptr;
            qDebug() << "[CHAT VIEW][HOST] StopAudioInput: Cleaned up input_device_ for already stopped audio_input.";
        }
    } else {
        qDebug() << "[CHAT VIEW][HOST] StopAudioInput: audio_input_ is null.";
    }

    if (message_area_ && ptt_state_ != Receiving) {
        message_area_->SetAudioAmplitude(0.0);
    }
}

void ChatView::StartAudioOutput() {
    if (!audio_output_) {
        qWarning() << "[CHAT VIEW] Audio Output: Cannot start, m_audioOutput is null!";
        return;
    }
    if (ptt_state_ != Receiving) {
        qDebug() << "[CHAT VIEW] Audio Output: Not starting, state is not Receiving.";
        return;
    }
    if (audio_output_->state() != QAudio::StoppedState && audio_output_->state() != QAudio::IdleState) {
        qDebug() << "[CHAT VIEW] Audio Output: Already running or in an intermediate state:" << audio_output_->state();
        if (!output_device_ && audio_output_->state() == QAudio::ActiveState) {
            qWarning() << "[CHAT VIEW] Audio Output: Active state but m_outputDevice is null. Potential issue.";
        }
        return;
    }

    qDebug() << "[CHAT VIEW] Audio Output: Attempting to start...";
    output_device_ = audio_output_->start();
    if (output_device_) {
        qDebug() << "[CHAT VIEW] Audio Output: Started successfully. State:" << audio_output_->state();
    } else {
        qWarning() << "[CHAT VIEW] Audio Output: Failed to start! State:" << audio_output_->state() << "Error:"
                << audio_output_->error();
        OnPttStopReceiving(current_frequency_);
    }
}

void ChatView::StopAudioOutput() {
    if (audio_output_ && audio_output_->state() != QAudio::StoppedState) {
        qDebug() << "[CHAT VIEW] Audio Output: Stopping... Current state:" << audio_output_->state();
        audio_output_->stop();
        output_device_ = nullptr;
        qDebug() << "[CHAT VIEW] Audio Output: Stopped. State:" << audio_output_->state();
    } else if (audio_output_) {
        qDebug() << "[CHAT VIEW] Audio Output: Already stopped. State:" << audio_output_->state();
    }
    if (message_area_) {
        message_area_->SetGlitchIntensity(0.0);
    }
}

qreal ChatView::CalculateAmplitude(const QByteArray &buffer) const {
    if (buffer.isEmpty() || audio_format_.sampleSize() != 16 || audio_format_.sampleType() != QAudioFormat::SignedInt) {
        return 0.0;
    }

    const auto data = reinterpret_cast<const qint16 *>(buffer.constData());
    const int sample_count = buffer.size() / (audio_format_.sampleSize() / 8);
    if (sample_count == 0) return 0.0;

    double sum_of_squares = 0.0;
    for (int i = 0; i < sample_count; ++i) {
        const double normalized_sample = static_cast<double>(data[i]) / 32767.0;
        sum_of_squares += normalized_sample * normalized_sample;
    }

    const double mean_square = sum_of_squares / sample_count;
    const double rms = std::sqrt(mean_square);

    return rms;
}

void ChatView::UpdatePttButtonState() const {
    switch (ptt_state_) {
        case Idle:
            ptt_button_->setEnabled(true);
            ptt_button_->setText(
                translator_->Translate("ChatView.TalkButton", "TALK")
            );
            input_field_->setEnabled(true);
            send_button_->setEnabled(true);
            attach_button_->setEnabled(true);
            break;
        case Requesting:
            ptt_button_->setEnabled(true);
            ptt_button_->setText("...");
            input_field_->setEnabled(false);
            send_button_->setEnabled(false);
            attach_button_->setEnabled(false);
            break;
        case Transmitting:
            ptt_button_->setEnabled(true);
            ptt_button_->setText(
                translator_->Translate("ChatView.PttTransmitting", "BRDCST")
            );
            input_field_->setEnabled(false);
            send_button_->setEnabled(false);
            attach_button_->setEnabled(false);
            break;
        case Receiving:
            ptt_button_->setEnabled(false);
            ptt_button_->setText(
                translator_->Translate("ChatView.PttReceiving", "RCVNG")
            );
            input_field_->setEnabled(false);
            send_button_->setEnabled(false);
            attach_button_->setEnabled(false);
            break;
    }
}

void ChatView::ResetStatusIndicator() const {
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
