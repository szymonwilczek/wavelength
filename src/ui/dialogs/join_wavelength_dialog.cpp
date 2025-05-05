#include "join_wavelength_dialog.h"

#include <QApplication>
#include <QFormLayout>
#include <QVBoxLayout>

#include "../../app/managers/translation_manager.h"

JoinWavelengthDialog::JoinWavelengthDialog(QWidget *parent): AnimatedDialog(parent, kDigitalMaterialization),
                                                             shadow_size_(10), scanline_opacity_(0.08) {
    // Dodaj flagi optymalizacyjne
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_StaticContents, true);
    setAttribute(Qt::WA_ContentsPropagated, false);

    translator_ = TranslationManager::GetInstance();
    digitalization_progress_ = 0.0;
    animation_started_ = false;

    setWindowTitle(translator_->Translate("JoinWavelengthDialog.Title", "JOIN_WAVELENGTH::CONNECT"));
    setModal(true);
    setFixedSize(450, 350);
    SetAnimationDuration(400);

    auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(12);

    // Nagłówek z tytułem
    auto title_label = new QLabel(translator_->Translate("JoinWavelengthDialog.Header", "JOIN WAVELENGTH"), this);
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
    auto time_label = new QLabel(QString("TS: %1").arg(timestamp), this);
    time_label->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    info_panel_layout->addWidget(session_label);
    info_panel_layout->addStretch();
    info_panel_layout->addWidget(time_label);
    main_layout->addWidget(info_panel);

    // Panel instrukcji
    auto info_label = new QLabel(translator_->Translate("JoinWavelengthDialog.InfoLabel", "Enter the wavelength frequency (130Hz - 180MHz)"), this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    info_label->setAlignment(Qt::AlignLeft);
    info_label->setWordWrap(true);
    main_layout->addWidget(info_label);

    // Formularz
    auto form_layout = new QFormLayout();
    form_layout->setSpacing(12);
    form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form_layout->setFormAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    form_layout->setContentsMargins(0, 15, 0, 15);

    // Etykieta częstotliwości
    auto frequency_title_label = new QLabel(translator_->Translate("JoinWavelengthDialog.FrequencyLabel", "FREQUENCY:"), this);
    frequency_title_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    // Kontener dla pola częstotliwości i selecta jednostki
    auto freq_container = new QWidget(this);
    auto freq_layout = new QHBoxLayout(freq_container);
    freq_layout->setContentsMargins(0, 0, 0, 0);
    freq_layout->setSpacing(5);

    // Pole wprowadzania częstotliwości
    frequency_edit_ = new CyberLineEdit(this);
    auto validator = new QDoubleValidator(130, 180000000.0, 1, this);
    QLocale locale(QLocale::English);
    locale.setNumberOptions(QLocale::RejectGroupSeparator);
    validator->setLocale(locale);
    frequency_edit_->setValidator(validator);
    frequency_edit_->setPlaceholderText(translator_->Translate("JoinWavelengthDialog.FrequencyEdit","ENTER FREQUENCY"));
    freq_layout->addWidget(frequency_edit_, 3);

    // ComboBox z jednostkami
    frequency_unit_combo_ = new QComboBox(this);
    frequency_unit_combo_->addItem("Hz");
    frequency_unit_combo_->addItem("kHz");
    frequency_unit_combo_->addItem("MHz");
    frequency_unit_combo_->setFixedHeight(30);
    frequency_unit_combo_->setStyleSheet(
        "QComboBox {"
        "  background-color: #001822;"
        "  color: #00ccff;"
        "  border: 1px solid #00a0cc;"
        "  border-radius: 3px;"
        "  padding: 5px;"
        "  font-family: Consolas;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 20px;"
        "  border-left: 1px solid #00a0cc;"
        "}"
    );
    freq_layout->addWidget(frequency_unit_combo_, 1);

    form_layout->addRow(frequency_title_label, freq_container);

    // Etykieta informacji o separatorze dziesiętnym
    auto decimal_hint_label = new QLabel(translator_->Translate("JoinWavelengthDialog.DecimalHintLabel", "Use dot (.) as decimal separator (e.g. 98.7)"), this);
    decimal_hint_label->setStyleSheet("color: #008888; background-color: transparent; font-family: Consolas; font-size: 8pt;");
    form_layout->addRow("", decimal_hint_label);

    // Etykieta hasła
    auto password_label = new QLabel(translator_->Translate("JoinWavelengthDialog.PasswordLabel", "PASSWORD:"), this);
    password_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    // Pole hasła
    password_edit_ = new CyberLineEdit(this);
    password_edit_->setEchoMode(QLineEdit::Password);
    password_edit_->setPlaceholderText(translator_->Translate("JoinWavelengthDialog.PasswordEdit", "WAVELENGTH PASSWORD (IF REQUIRED))");
    form_layout->addRow(password_label, password_edit_);

    main_layout->addLayout(form_layout);

    // Etykieta statusu
    status_label_ = new QLabel(this);
    status_label_->setStyleSheet("color: #ff3355; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    status_label_->hide();
    main_layout->addWidget(status_label_);

    // Panel przycisków
    auto button_layout = new QHBoxLayout();
    button_layout->setSpacing(15);

    join_button_ = new CyberButton(translator_->Translate("JoinWavelengthDialog.JoinButton", "JOIN WAVELENGTH"), this, true);
    join_button_->setFixedHeight(35);

    cancel_button_ = new CyberButton(translator_->Translate("JoinWavelengthDialog.CancelButton", "CANCEL"), this, false);
    cancel_button_->setFixedHeight(35);

    button_layout->addWidget(join_button_, 2);
    button_layout->addWidget(cancel_button_, 1);
    main_layout->addLayout(button_layout);

    // Połączenia sygnałów i slotów
    connect(frequency_edit_, &QLineEdit::textChanged, this, &JoinWavelengthDialog::ValidateInput);
    connect(password_edit_, &QLineEdit::textChanged, this, &JoinWavelengthDialog::ValidateInput);
    connect(join_button_, &QPushButton::clicked, this, &JoinWavelengthDialog::TryJoin);
    connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);

    WavelengthJoiner* joiner = WavelengthJoiner::GetInstance();
    connect(joiner, &WavelengthJoiner::wavelengthJoined, this, &JoinWavelengthDialog::accept);
    connect(joiner, &WavelengthJoiner::authenticationFailed, this, &JoinWavelengthDialog::OnAuthFailed);
    connect(joiner, &WavelengthJoiner::connectionError, this, &JoinWavelengthDialog::OnConnectionError);

    ValidateInput();

    // Inicjuj timer odświeżania
    refresh_timer_ = new QTimer(this);
    refresh_timer_->setInterval(16);
    connect(refresh_timer_, &QTimer::timeout, this, [this]() {
        if (digitalization_progress_ > 0.0 && digitalization_progress_ < 1.0) {
            const int scanline_y = height() * digitalization_progress_;

            if (scanline_y != last_scanline_y_) {
                // Odśwież tylko obszar wokół aktualnej i poprzedniej pozycji linii skanującej
                int min_y = qMin(scanline_y, last_scanline_y_) - 15;
                int max_y = qMax(scanline_y, last_scanline_y_) + 15;

                // Ograniczenie zakresu do widocznego obszaru
                min_y = qMax(0, min_y);
                max_y = qMin(height(), max_y);

                update(0, min_y, width(), max_y - min_y);
            }
        } else {
            refresh_timer_->setInterval(33); // Mniej intensywne odświeżanie gdy nie ma animacji
        }
    });
}

JoinWavelengthDialog::~JoinWavelengthDialog() {
    if (refresh_timer_) {
        refresh_timer_->stop();
        delete refresh_timer_;
        refresh_timer_ = nullptr;
    }
}

void JoinWavelengthDialog::SetDigitalizationProgress(const double progress) {
    if (!animation_started_ && progress > 0.01)
        animation_started_ = true;
    digitalization_progress_ = progress;
    update();
}

void JoinWavelengthDialog::SetCornerGlowProgress(const double progress) {
    corner_glow_progress_ = progress;
    update();
}

void JoinWavelengthDialog::SetScanlineOpacity(const double opacity) {
    scanline_opacity_ = opacity;
    update();
}

void JoinWavelengthDialog::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Główne tło dialogu z gradientem (bez zmian)
    QLinearGradient background_gradient(0, 0, 0, height());
    background_gradient.setColorAt(0, QColor(10, 21, 32));
    background_gradient.setColorAt(1, QColor(7, 18, 24));

    // Ścieżka dialogu ze ściętymi rogami (bez zmian)
    QPainterPath dialog_path;
    int clip_size = 20;
    dialog_path.moveTo(clip_size, 0);
    dialog_path.lineTo(width() - clip_size, 0);
    dialog_path.lineTo(width(), clip_size);
    dialog_path.lineTo(width(), height() - clip_size);
    dialog_path.lineTo(width() - clip_size, height());
    dialog_path.lineTo(clip_size, height());
    dialog_path.lineTo(0, height() - clip_size);
    dialog_path.lineTo(0, clip_size);
    dialog_path.closeSubpath();

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
        InitRenderBuffers(); // Inicjalizuje tylko m_scanlineBuffer

        int scanline_y = static_cast<int>(height() * digitalization_progress_);

        if (!scanline_buffer_.isNull()) {
            painter.setClipping(false);
            QPainter::CompositionMode previousMode = painter.compositionMode();
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            // Oblicz szerokość linii skanującej (bez zmian)
            int start_x = 0;
            int end_x = width();
            if (scanline_y < clip_size) { /* ... */ }
            else if (scanline_y > height() - clip_size) { /* ... */ }

            // Rysuj tylko główną linię skanującą (bez zmian)
            int scan_width = end_x - start_x;
            if (scan_width > 0) {
                painter.drawPixmap(start_x, scanline_y - 10, scan_width, 20,
                                   scanline_buffer_,
                                   (width() - scan_width) / 2, 0, scan_width, 20);
            }

            painter.setCompositionMode(previousMode);
        }
    }
    // --- KONIEC ZMIANY ---

    // Podświetlanie rogów
    if (corner_glow_progress_ > 0.0) {
        QColor corner_color(0, 220, 255, 150);
        painter.setPen(QPen(corner_color, 2));

        double step = 1.0 / 4;

        if (corner_glow_progress_ >= step * 0) {
            painter.drawLine(0, clip_size * 1.5, 0, clip_size);
            painter.drawLine(0, clip_size, clip_size, 0);
            painter.drawLine(clip_size, 0, clip_size * 1.5, 0);
        }

        if (corner_glow_progress_ >= step * 1) {
            painter.drawLine(width() - clip_size * 1.5, 0, width() - clip_size, 0);
            painter.drawLine(width() - clip_size, 0, width(), clip_size);
            painter.drawLine(width(), clip_size, width(), clip_size * 1.5);
        }

        if (corner_glow_progress_ >= step * 2) {
            painter.drawLine(width(), height() - clip_size * 1.5, width(), height() - clip_size);
            painter.drawLine(width(), height() - clip_size, width() - clip_size, height());
            painter.drawLine(width() - clip_size, height(), width() - clip_size * 1.5, height());
        }

        if (corner_glow_progress_ >= step * 3) {
            painter.drawLine(clip_size * 1.5, height(), clip_size, height());
            painter.drawLine(clip_size, height(), 0, height() - clip_size);
            painter.drawLine(0, height() - clip_size, 0, height() - clip_size * 1.5);
        }
    }

    // Efekt scanline
    if (scanline_opacity_ > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * scanline_opacity_));

        for (int y = 0; y < height(); y += 3) {
            painter.drawRect(0, y, width(), 1);
        }
    }
}

QString JoinWavelengthDialog::GetFrequency() const {
    return frequency_edit_->text();
}

void JoinWavelengthDialog::ValidateInput() const {
    status_label_->hide();
    join_button_->setEnabled(!frequency_edit_->text().isEmpty());
}

void JoinWavelengthDialog::TryJoin() {
    static bool is_joining = false;

    if (is_joining) {
        return;
    }

    is_joining = true;
    status_label_->hide();

    const QString frequency = GetFrequency();
    const QString password = password_edit_->text();

    const auto scanline_animation = new QPropertyAnimation(this, "scanlineOpacity");
    scanline_animation->setDuration(800);
    scanline_animation->setStartValue(0.1);
    scanline_animation->setEndValue(0.4);
    scanline_animation->setKeyValueAt(0.5, 0.6);
    scanline_animation->start(QAbstractAnimation::DeleteWhenStopped);

    WavelengthJoiner* joiner = WavelengthJoiner::GetInstance();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const auto [success, error_reason] = joiner->JoinWavelength(frequency, password);
    QApplication::restoreOverrideCursor();

    if (!success) {
        status_label_->setText(error_reason);
        status_label_->show();

        const auto error_glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
        error_glitch_animation->setDuration(1000);
        error_glitch_animation->setStartValue(0.5);
        error_glitch_animation->setEndValue(0.0);
        error_glitch_animation->setKeyValueAt(0.2, 0.8);
        error_glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    is_joining = false;
}

void JoinWavelengthDialog::OnAuthFailed() {
    status_label_->setText(translator_->Translate("JoinWavelengthDialog.IncorrectPassword", "INCORRECT PASSWORD"));
    status_label_->show();

    const auto error_glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    error_glitch_animation->setDuration(1000);
    error_glitch_animation->setStartValue(0.5);
    error_glitch_animation->setEndValue(0.0);
    error_glitch_animation->setKeyValueAt(0.3, 0.7);
    error_glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void JoinWavelengthDialog::OnConnectionError(const QString &error_message) {
    QString display_message;

    if (error_message.contains("Password required")) {
        display_message = translator_->Translate("JoinWavelengthDialog.PasswordRequired","WAVELENGTH PASSWORD PROTECTED");
    } else if (error_message.contains("Invalid password")) {
        display_message = translator_->Translate("JoinWavelengthDialog.IncorrectPassword","INCORRECT PASSWORD");
    } else {
        display_message = translator_->Translate("JoinWavelengthDialog.WavelengthUnavailable","WAVELENGTH UNAVAILABLE");
    }

    status_label_->setText(display_message);
    status_label_->show();

    const auto error_glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    error_glitch_animation->setDuration(1000);
    error_glitch_animation->setStartValue(0.5);
    error_glitch_animation->setEndValue(0.0);
    error_glitch_animation->setKeyValueAt(0.2, 0.9);
    error_glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void JoinWavelengthDialog::InitRenderBuffers() {
    if (!buffers_initialized_ || height() != previous_height_) {
        scanline_buffer_ = QPixmap(width(), 20);
        scanline_buffer_.fill(Qt::transparent);

        QPainter scan_painter(&scanline_buffer_);
        scan_painter.setRenderHint(QPainter::Antialiasing, false);

        QLinearGradient scan_gradient(0, 0, 0, 20);
        scan_gradient.setColorAt(0, QColor(0, 200, 255, 0));
        scan_gradient.setColorAt(0.5, QColor(0, 220, 255, 180));
        scan_gradient.setColorAt(1, QColor(0, 200, 255, 0));

        scan_painter.setPen(Qt::NoPen);
        scan_painter.setBrush(scan_gradient);
        scan_painter.drawRect(0, 0, width(), 20);

        buffers_initialized_ = true;
        previous_height_ = height();
        last_scanline_y_ = -1;
    }
}
