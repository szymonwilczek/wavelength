#include "wavelength_dialog.h"

#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QNetworkReply>
#include <QVBoxLayout>

#include "../../app/managers/translation_manager.h"

WavelengthDialog::WavelengthDialog(QWidget *parent): AnimatedDialog(parent, AnimatedDialog::kDigitalMaterialization),
                                                     shadow_size_(10), scanline_opacity_(0.08) {

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // Preferuj animacje przez GPU
    setAttribute(Qt::WA_TranslucentBackground);

    setAttribute(Qt::WA_StaticContents, true);

    digitalization_progress_ = 0.0;
    animation_started_ = false;
    translator_ = TranslationManager::GetInstance();

    setWindowTitle(translator_->Translate("CreateWavelengthDialog.Title", "CREATE_WAVELENGTH::NEW_INSTANCE"));
    setModal(true);
    setFixedSize(450, 350);

    SetAnimationDuration(400);

    auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(12);

    // Nagłówek z tytułem
    auto title_label = new QLabel(translator_->Translate("CreateWavelengthDialog.Header", "CREATE WAVELENGTH"), this);
    title_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");
    title_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    title_label->setContentsMargins(0, 0, 0, 3);
    main_layout->addWidget(title_label);

    // Panel informacyjny z ID sesji
    auto info_panel = new QWidget(this);
    auto info_panel_layout = new QHBoxLayout(info_panel);
    info_panel_layout->setContentsMargins(0, 0, 0, 0);
    info_panel_layout->setSpacing(5);

    QString session_id = QString("%1-%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10000, 99999));
    auto session_label = new QLabel(QString("SESSION_ID: %1").arg(session_id), this);
    session_label->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    auto timeLabel = new QLabel(QString("TS: %1").arg(timestamp), this);
    timeLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    info_panel_layout->addWidget(session_label);
    info_panel_layout->addStretch();
    info_panel_layout->addWidget(timeLabel);
    main_layout->addWidget(info_panel);

    // Panel instrukcji - poprawka dla responsywności
    auto info_label = new QLabel(translator_->Translate("CreateWavelengthDialog.InfoLabel", "System automatically assigns the lowest available frequency"), this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    info_label->setAlignment(Qt::AlignLeft);
    info_label->setWordWrap(true); // Włącz zawijanie tekstu
    main_layout->addWidget(info_label);

    // Uprościliśmy panel formularza - bez dodatkowego kontenera
    auto form_layout = new QFormLayout();
    form_layout->setSpacing(12); // Zwiększ odstępy
    form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form_layout->setFormAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // Wyśrodkuj w pionie
    form_layout->setContentsMargins(0, 15, 0, 15); // Dodaj więcej przestrzeni w pionie

    // Etykiety w formularzu
    auto frequency_title_label = new QLabel(translator_->Translate("CreateWavelengthDialog.FrequencyLabel", "ASSIGNED FREQUENCY:"), this);
    frequency_title_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    // Pole wyświetlające częstotliwość
    frequency_label_ = new QLabel(this);
    frequency_label_->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 16pt;");
    form_layout->addRow(frequency_title_label, frequency_label_);

    // Wskaźnik ładowania przy wyszukiwaniu częstotliwości
    loading_indicator_ = new QLabel(translator_->Translate("CreateWavelengthDialog.LoadingIndicator", "SEARCHING FOR AVAILABLE FREQUENCY..."), this);
    loading_indicator_->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    form_layout->addRow("", loading_indicator_);

    // Checkbox zabezpieczenia hasłem
    password_protected_checkbox_ = new CyberCheckBox(translator_->Translate("CreateWavelengthDialog.PasswordCheckbox", "PASSWORD PROTECTED"), this);
    // Aktualizacja stylów dla checkboxa
    form_layout->addRow("", password_protected_checkbox_);

    // Etykieta i pole hasła
    auto password_label = new QLabel(translator_->Translate("CreateWavelengthDialog.PasswordLabel", "PASSWORD:"), this);
    password_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    password_edit_ = new CyberLineEdit(this);
    password_edit_->setEchoMode(QLineEdit::Password);
    password_edit_->setEnabled(false);
    password_edit_->setPlaceholderText(translator_->Translate("CreateWavelengthDialog.PasswordEdit", "ENTER WAVELENGTH PASSWORD"));
    password_edit_->setStyleSheet("font-family: Consolas; font-size: 9pt;");
    password_edit_->setFixedHeight(30); // Wymuszenie wysokości 30px
    password_edit_->setVisible(true);   // Upewnij się, że jest widoczne
    form_layout->addRow(password_label, password_edit_);

    if (!password_protected_checkbox_->isChecked()) {
        password_edit_->setStyleSheet("border: none; background-color: transparent; padding: 6px; font-family: Consolas; font-size: 9pt; color: #005577;");
    }

    main_layout->addLayout(form_layout);

    // Etykieta statusu
    status_label_ = new QLabel(this);
    status_label_->setStyleSheet("color: #ff3355; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    status_label_->hide(); // Ukryj na początku
    main_layout->addWidget(status_label_);

    // Panel przycisków
    auto button_layout = new QHBoxLayout();
    button_layout->setSpacing(15);

    generate_button_ = new CyberButton(translator_->Translate("CreateWavelengthDialog.CreateButton", "CREATE WAVELENGTH"), this, true);
    generate_button_->setFixedHeight(35);
    generate_button_->setEnabled(false); // Disable until frequency is found

    cancel_button_ = new CyberButton(translator_->Translate("CreateWavelengthDialog.CancelButton", "CANCEL"), this, false);
    cancel_button_->setFixedHeight(35);

    button_layout->addWidget(generate_button_, 2);
    button_layout->addWidget(cancel_button_, 1);
    main_layout->addLayout(button_layout);

    // Połączenia sygnałów i slotów
    connect(password_protected_checkbox_, &QCheckBox::toggled, this, [this](const bool checked) {
        password_edit_->setEnabled(checked);

        // Zmiana wyglądu pola w zależności od stanu
        if (checked) {
            password_edit_->setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt; color: #00ccff;");
        } else {
            password_edit_->setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt; color: #005577;");
        }

        // Ukrywamy etykietę statusu przy zmianie stanu checkboxa
        status_label_->hide();
        ValidateInputs();
    });
    connect(password_edit_, &QLineEdit::textChanged, this, &WavelengthDialog::ValidateInputs);
    connect(generate_button_, &QPushButton::clicked, this, &WavelengthDialog::TryGenerate);
    connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);

    frequency_watcher_ = new QFutureWatcher<QString>(this);
    connect(frequency_watcher_, &QFutureWatcher<double>::finished, this, &WavelengthDialog::OnFrequencyFound);

    // Ustawiamy text w etykiecie częstotliwości na domyślną wartość
    frequency_label_->setText("...");

    // Podłączamy sygnał zakończenia animacji do rozpoczęcia wyszukiwania częstotliwości
    connect(this, &AnimatedDialog::showAnimationFinished,
            this, &WavelengthDialog::StartFrequencySearch);

    // Inicjuj timer odświeżania
    refresh_timer_ = new QTimer(this);
    refresh_timer_->setInterval(16); // ~60fps dla płynności animacji
    connect(refresh_timer_, &QTimer::timeout, this, [this]() {
        if (digitalization_progress_ > 0.0 && digitalization_progress_ < 1.0) {
            const int current_scanline_y = static_cast<int>(height() * digitalization_progress_);

            if (current_scanline_y != last_scanline_y_) {
                constexpr int scanline_height = 20;
                // Określ obszar do odświeżenia: obejmuje starą i nową pozycję linii
                int update_y_start = qMin(current_scanline_y, last_scanline_y_) - scanline_height / 2 - 2; // Trochę zapasu
                int update_y_end = qMax(current_scanline_y, last_scanline_y_) + scanline_height / 2 + 2;

                // Ogranicz do granic widgetu
                update_y_start = qMax(0, update_y_start);
                update_y_end = qMin(height(), update_y_end);

                // Odśwież tylko ten prostokątny obszar
                update(0, update_y_start, width(), update_y_end - update_y_start);

                last_scanline_y_ = current_scanline_y; // Zaktualizuj ostatnią pozycję
            }
            refresh_timer_->setInterval(16); // Utrzymaj 60fps podczas animacji
        } else {
            refresh_timer_->setInterval(100); // Znacznie wolniej, gdy nic się nie animuje
        }
    });
}

WavelengthDialog::~WavelengthDialog() {
    if (refresh_timer_) {
        refresh_timer_->stop();
        delete refresh_timer_;
        refresh_timer_ = nullptr;
    }
}

void WavelengthDialog::SetDigitalizationProgress(const double progress) {
    if (!animation_started_ && progress > 0.01)
        animation_started_ = true;
    digitalization_progress_ = progress;
    update();
}

void WavelengthDialog::SetCornerGlowProgress(const double progress) {
    corner_glow_progress_ = progress;
    update();
}

void WavelengthDialog::SetScanlineOpacity(const double opacity) {
    scanline_opacity_ = opacity;
    update();
}

void WavelengthDialog::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Główne tło dialogu z gradientem (bez zmian)
    QLinearGradient background_gradient(0, 0, 0, height());
    background_gradient.setColorAt(0, QColor(10, 21, 32));
    background_gradient.setColorAt(1, QColor(7, 18, 24));

    // Ścieżka dialogu ze ściętymi rogami (bez zmian)
    QPainterPath dialog_path;
    int clip_size = 20; // rozmiar ścięcia
    dialog_path.moveTo(clip_size, 0);
    dialog_path.lineTo(width() - clip_size, 0);
    dialog_path.lineTo(width(), clip_size);
    dialog_path.lineTo(width(), height() - clip_size);
    dialog_path.lineTo(width() - clip_size, height());
    dialog_path.lineTo(clip_size, height());
    dialog_path.lineTo(0, height() - clip_size);
    dialog_path.lineTo(0, clip_size);
    dialog_path.closeSubpath(); // Zamknij ścieżkę

    painter.setPen(Qt::NoPen);
    painter.setBrush(background_gradient);
    painter.drawPath(dialog_path);

    // Obramowanie dialogu (bez zmian)
    QColor border_color(0, 195, 255, 150);
    painter.setPen(QPen(border_color, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(dialog_path);

    // --- ZMIANA: Rysowanie tylko pojedynczej linii skanującej ---
    if (digitalization_progress_ > 0.0 && digitalization_progress_ < 1.0 && animation_started_) {
        // Inicjalizacja bufora linii skanującej (jeśli potrzebna)
        InitRenderBuffers(); // Teraz inicjalizuje tylko m_scanlineBuffer

        int scanline_y = static_cast<int>(height() * digitalization_progress_);

        // Sprawdź, czy bufor linii skanującej jest gotowy
        if (!scanline_buffer_.isNull()) {
            painter.setClipping(false); // Wyłącz clipping na czas rysowania linii
            QPainter::CompositionMode previousMode = painter.compositionMode();
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            int start_x = 0;
            int end_x = width();
            if (scanline_y < clip_size) {
                float ratio = static_cast<float>(scanline_y) / clip_size;
                start_x = clip_size * (1.0f - ratio);
                end_x = width() - clip_size * (1.0f - ratio);
            } else if (scanline_y > height() - clip_size) {
                float ratio = static_cast<float>(height() - scanline_y) / clip_size;
                start_x = clip_size * (1.0f - ratio);
                end_x = width() - clip_size * (1.0f - ratio);
            }

            int scan_width = end_x - start_x;
            if (scan_width > 0) {
                painter.drawPixmap(start_x, scanline_y - 10, scan_width, 20, // Pozycja Y centruje gradient
                                   scanline_buffer_,
                                   (width() - scan_width) / 2, 0, scan_width, 20); // Wybierz odpowiedni fragment bufora
            }

            painter.setCompositionMode(previousMode);
        }
    }


    if (corner_glow_progress_ > 0.0) {
        QColor corner_color(0, 220, 255, 150);
        painter.setPen(QPen(corner_color, 2));
        double step = 1.0 / 4;
        if (corner_glow_progress_ >= step * 0) { /* Lewy górny */ painter.drawLine(0, clip_size * 1.5, 0, clip_size); painter.drawLine(0, clip_size, clip_size, 0); painter.drawLine(clip_size, 0, clip_size * 1.5, 0); }
        if (corner_glow_progress_ >= step * 1) { /* Prawy górny */ painter.drawLine(width() - clip_size * 1.5, 0, width() - clip_size, 0); painter.drawLine(width() - clip_size, 0, width(), clip_size); painter.drawLine(width(), clip_size, width(), clip_size * 1.5); }
        if (corner_glow_progress_ >= step * 2) { /* Prawy dolny */ painter.drawLine(width(), height() - clip_size * 1.5, width(), height() - clip_size); painter.drawLine(width(), height() - clip_size, width() - clip_size, height()); painter.drawLine(width() - clip_size, height(), width() - clip_size * 1.5, height()); }
        if (corner_glow_progress_ >= step * 3) { /* Lewy dolny */ painter.drawLine(clip_size * 1.5, height(), clip_size, height()); painter.drawLine(clip_size, height(), 0, height() - clip_size); painter.drawLine(0, height() - clip_size, 0, height() - clip_size * 1.5); }
    }

    // Linie skanowania (scanlines) - tło (bez zmian)
    if (scanline_opacity_ > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * scanline_opacity_));
        for (int y = 0; y < height(); y += 3) {
            painter.drawRect(0, y, width(), 1);
        }
    }
}

void WavelengthDialog::ValidateInputs() const {
    status_label_->hide();

    bool is_password_valid = true;
    if (password_protected_checkbox_->isChecked()) {
        is_password_valid = !password_edit_->text().isEmpty();
    }

    generate_button_->setEnabled(is_password_valid && frequency_found_);
}

void WavelengthDialog::StartFrequencySearch() {
    loading_indicator_->setText(translator_->Translate("CreateWavelengthDialog.LoadingIndicator", "SEARCHING FOR AVAILABLE FREQUENCY..."));

    const auto timeout_timer = new QTimer(this);
    timeout_timer->setSingleShot(true);
    timeout_timer->setInterval(5000); // 5 sekund limitu na szukanie
    connect(timeout_timer, &QTimer::timeout, [this]() {
        if (frequency_watcher_ && frequency_watcher_->isRunning()) {
            loading_indicator_->setText(translator_->Translate("CreateWavelengthDialog.DefaultIndicator", "USING DEFAULT FREQUENCY..."));
            frequency_ = 130.0;
            frequency_found_ = true;
            OnFrequencyFound();
            qDebug() << "LOG: Timeout podczas szukania częstotliwości - używam wartości domyślnej";
        }
    });
    timeout_timer->start();

    const QFuture<QString> future = QtConcurrent::run(&WavelengthDialog::FindLowestAvailableFrequency);
    frequency_watcher_->setFuture(future);
}

void WavelengthDialog::TryGenerate() {
    static bool is_generating = false;

    if (is_generating) {
        qDebug() << "LOG: Blokada wielokrotnego wywołania tryGenerate()";
        return;
    }

    is_generating = true;

    const bool is_password_protected = password_protected_checkbox_->isChecked();
    const QString password = password_edit_->text();

    if (is_password_protected && password.isEmpty()) {
        status_label_->setText(translator_->Translate("CreateWavelengthDialog.PasswordRequired", "PASSSWORD REQUIRED"));
        status_label_->show();
        is_generating = false;
        return;
    }

    QDialog::accept();

    is_generating = false;
}

void WavelengthDialog::OnFrequencyFound() {
    // Zatrzymujemy timer zabezpieczający jeśli istnieje
    const auto timeout_timer = findChild<QTimer*>();
    if (timeout_timer && timeout_timer->isActive()) {
        timeout_timer->stop();
    }

    if (frequency_watcher_ && frequency_watcher_->isFinished()) {
        frequency_ = frequency_watcher_->result();
    } else {
        frequency_ = 130.0;
    }

    frequency_found_ = true;

    QString frequency_text = frequency_;

    const auto loader_slide_animation = new QPropertyAnimation(loading_indicator_, "maximumHeight");
    loader_slide_animation->setDuration(400);
    loader_slide_animation->setStartValue(loading_indicator_->sizeHint().height());
    loader_slide_animation->setEndValue(0);
    loader_slide_animation->setEasingCurve(QEasingCurve::OutQuint);

    const auto opacity_effect = new QGraphicsOpacityEffect(frequency_label_);
    frequency_label_->setGraphicsEffect(opacity_effect);
    opacity_effect->setOpacity(0.0); // Początkowo niewidoczny

    auto frequency_animation = new QPropertyAnimation(opacity_effect, "opacity");
    frequency_animation->setDuration(600);
    frequency_animation->setStartValue(0.0);
    frequency_animation->setEndValue(1.0);
    frequency_animation->setEasingCurve(QEasingCurve::InQuad);

    const auto animation_group = new QParallelAnimationGroup(this);
    animation_group->addAnimation(loader_slide_animation);

    connect(loader_slide_animation, &QPropertyAnimation::finished, [this, frequency_text]() {
        loading_indicator_->hide();
        frequency_label_->setText(frequency_text);
    });

    connect(loader_slide_animation, &QPropertyAnimation::finished, [this, frequency_animation]() {
        frequency_animation->start(QAbstractAnimation::DeleteWhenStopped);
    });

    loading_indicator_->setAttribute(Qt::WA_OpaquePaintEvent, false);
    frequency_label_->setAttribute(Qt::WA_OpaquePaintEvent, false);

    frequency_animation->setKeyValueAt(0.2, 0.3);
    frequency_animation->setKeyValueAt(0.4, 0.6);
    frequency_animation->setKeyValueAt(0.6, 0.8);
    frequency_animation->setKeyValueAt(0.8, 0.95);

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);

    ValidateInputs();

    const auto scanline_animation = new QPropertyAnimation(this, "scanlineOpacity");
    scanline_animation->setDuration(1500);
    scanline_animation->setStartValue(0.3);
    scanline_animation->setEndValue(0.08);
    scanline_animation->setKeyValueAt(0.3, 0.4);
    scanline_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QString WavelengthDialog::FormatFrequencyText(const double frequency) {
    QString unit_text;
    double display_value;

    if (frequency >= 1000000) {
        display_value = frequency / 1000000.0;
        unit_text = " MHz";
    } else if (frequency >= 1000) {
        display_value = frequency / 1000.0;
        unit_text = " kHz";
    } else {
        display_value = frequency;
        unit_text = " Hz";
    }

    return QString::number(display_value, 'f', 1) + unit_text;
}

QString WavelengthDialog::FindLowestAvailableFrequency() {

    const WavelengthConfig* config = WavelengthConfig::GetInstance();
    const QString preferred_frequency = config->GetPreferredStartFrequency();

    QNetworkAccessManager manager;
    QEventLoop loop;

    const QString server_address = config->GetRelayServerAddress();
    const int server_port = config->GetRelayServerPort();

    const QString base_url_string = QString("http://%1:%2/api/next-available-frequency").arg(server_address).arg(server_port);
    QUrl url(base_url_string);

    QUrlQuery query;
    query.addQueryItem("preferredStartFrequency", preferred_frequency);
    url.setQuery(query);

    const QNetworkRequest request(url);

    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    loop.exec();

    QString result_frequency = "130.0"; // Domyślna wartość w razie błędu

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray response_data = reply->readAll();
        const QJsonDocument document = QJsonDocument::fromJson(response_data);
        QJsonObject obj = document.object();

        if (obj.contains("frequency") && obj["frequency"].isString()) {
            result_frequency = obj["frequency"].toString();
        } else {
            qDebug() << "LOG: Błąd parsowania odpowiedzi JSON lub brak klucza 'frequency'. Odpowiedź:" << response_data;
        }
    } else {
        qDebug() << "LOG: Błąd podczas komunikacji z serwerem:" << reply->errorString();
    }

    reply->deleteLater();

    return result_frequency;
}

void WavelengthDialog::InitRenderBuffers() {
    if (!buffers_initialized_ || height() != previous_height_) {
        // --- ZMIANA: Inicjalizuj tylko bufor głównej linii skanującej ---
        scanline_buffer_ = QPixmap(width(), 20); // Stała wysokość 20px
        scanline_buffer_.fill(Qt::transparent);

        QPainter scan_painter(&scanline_buffer_);
        scan_painter.setRenderHint(QPainter::Antialiasing, false); // Antialiasing niepotrzebny dla gradientu

        // Główna linia skanująca jako gradient (bez zmian)
        QLinearGradient scan_gradient(0, 0, 0, 20); // Gradient w pionie na wysokości 20px
        scan_gradient.setColorAt(0, QColor(0, 200, 255, 0));   // Przezroczysty na górze
        scan_gradient.setColorAt(0.5, QColor(0, 220, 255, 180)); // Najjaśniejszy w środku
        scan_gradient.setColorAt(1, QColor(0, 200, 255, 0));   // Przezroczysty na dole

        scan_painter.setPen(Qt::NoPen);
        scan_painter.setBrush(scan_gradient);
        scan_painter.drawRect(0, 0, width(), 20); // Rysuj gradient na całej szerokości bufora


        buffers_initialized_ = true;
        previous_height_ = height(); // Zapisz wysokość dla ewentualnych przyszłych sprawdzeń
        last_scanline_y_ = -1; // Zresetuj ostatnią pozycję przy reinicjalizacji
    }
}
