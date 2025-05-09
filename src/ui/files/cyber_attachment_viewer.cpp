#include "cyber_attachment_viewer.h"

#include <QPainterPath>
#include <QPropertyAnimation>

#include "../../app/managers/translation_manager.h"

CyberAttachmentViewer::CyberAttachmentViewer(QWidget *parent): QWidget(parent), decryption_counter_(0),
                                                               is_scanning_(false), is_decrypted_(false) {
    translator_ = TranslationManager::GetInstance();
    // Główny układ
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(10, 10, 10, 10);
    layout_->setSpacing(10);

    // Label statusu
    status_label_ = new QLabel(
        translator_->Translate("CyberAttachmentViewer.Initializing",
            "INITIALIZING DECYPHERING SEQUENCE..."),
        this);
    status_label_->setStyleSheet(
        "QLabel {"
        "  color: #00ffff;"
        "  background-color: #001822;"
        "  border: 1px solid #00aaff;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 9pt;"
        "  padding: 4px;"
        "  border-radius: 2px;"
        "  font-weight: bold;"
        "}"
    );
    status_label_->setAlignment(Qt::AlignCenter);
    layout_->addWidget(status_label_);

    // Kontener na zawartość
    content_container_ = new QWidget(this);
    content_layout_ = new QVBoxLayout(content_container_);
    content_layout_->setContentsMargins(0, 0, 0, 0); // Zmieniono marginesy na 0
    // m_contentLayout->setAlignment(Qt::AlignCenter); // Usunięto alignment, pozwalamy zawartości wypełnić
    layout_->addWidget(content_container_, 1);

    mask_overlay_ = new MaskOverlay(content_container_);
    mask_overlay_->setVisible(false);


    // Timer dla animacji
    animation_timer_ = new QTimer(this);
    connect(animation_timer_, &QTimer::timeout, this, &CyberAttachmentViewer::UpdateAnimation);

    // Ustaw styl tła
    setStyleSheet(
        "CyberAttachmentViewer {"
        "  background-color: rgba(10, 20, 30, 200);"
        "  border: 1px solid #00aaff;"
        "}"
    );

    // Usuwamy ograniczenia minimum, aby mógł się dostosować do zawartości
    setMinimumSize(0, 0);

    // Ustawiamy dobrą politykę rozmiaru
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // Zmieniono na Preferred
    content_container_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);


    connect(this, &CyberAttachmentViewer::decryptionCounterChanged, mask_overlay_, &MaskOverlay::SetRevealProgress);


    QTimer::singleShot(500, this, &CyberAttachmentViewer::OnActionButtonClicked);
}

CyberAttachmentViewer::~CyberAttachmentViewer() {
    if (content_widget_) {
        content_widget_->setGraphicsEffect(nullptr);
    }
}

void CyberAttachmentViewer::SetDecryptionCounter(const int counter) {
    if (decryption_counter_ != counter) {
        decryption_counter_ = counter;
        UpdateDecryptionStatus();
        emit decryptionCounterChanged(decryption_counter_); // Emituj sygnał
    }
}

void CyberAttachmentViewer::UpdateContentLayout() {
    if (content_widget_) {
        // Wymuszamy aktualizację layoutu
        content_layout_->invalidate();
        content_layout_->activate();

        // Aktualizujemy geometrię
        content_widget_->updateGeometry();
        updateGeometry();

        // Aktualizujemy rozmiar i propagujemy zmianę w górę hierarchii
        QTimer::singleShot(50, this, [this]() {
            // Powiadom rodziców o zmianie rozmiaru
            QEvent event(QEvent::LayoutRequest);
            QApplication::sendEvent(this, &event);

            if (parentWidget()) {
                parentWidget()->updateGeometry();
                QApplication::sendEvent(parentWidget(), &event);
            }
        });
    }
}

void CyberAttachmentViewer::SetContent(QWidget *content) {

    if (content_widget_) {
        content_layout_->removeWidget(content_widget_);
        content_widget_->deleteLater();
        content_widget_ = nullptr;
    }

    content_widget_ = content;
    content_layout_->addWidget(content_widget_);

    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    content->setMinimumSize(0, 0);
    content->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    // *** ZMIANA: Zawartość jest początkowo UKRYTA ***
    content->setVisible(false);
    // *** ZMIANA: Maska jest pokazywana zamiast zawartości ***
    mask_overlay_->setVisible(true);
    mask_overlay_->raise(); // Upewnij się, że maska jest na wierzchu
    mask_overlay_->StartScanning(); // Rozpocznij animację skanowania na masce

    status_label_->setText(translator_->Translate("CyberAttachmentViewer.EncryptedDataDetected",
            "ENCRYPTED DATA DETECTED"));
    is_decrypted_ = false;
    is_scanning_ = false; // Resetuj stan skanowania
    SetDecryptionCounter(0); // Resetuj licznik

    content_layout_->activate();
    updateGeometry(); // Ważne, aby dostosować rozmiar kontenera

    // *** ZMIANA: Wymuś aktualizację rozmiaru maski po krótkim opóźnieniu ***
    QTimer::singleShot(10, this, [this]() {
        if (content_widget_ && mask_overlay_) {
            // Ustaw rozmiar maski na rozmiar kontenera zawartości
            mask_overlay_->setGeometry(content_container_->rect());
            mask_overlay_->raise(); // Ponownie na wszelki wypadek
        }
        // Poinformuj rodzica o potencjalnej zmianie rozmiaru
        if (parentWidget()) {
            parentWidget()->updateGeometry();
            if (parentWidget()->layout()) parentWidget()->layout()->activate();
            QEvent event(QEvent::LayoutRequest);
            QApplication::sendEvent(parentWidget(), &event);
        }
    });
}

QSize CyberAttachmentViewer::sizeHint() const {
    QSize hint;
    int extra_height = status_label_->sizeHint().height() + layout_->spacing();
    const int extra_width = layout_->contentsMargins().left() + layout_->contentsMargins().right();
    extra_height += layout_->contentsMargins().top() + layout_->contentsMargins().bottom();

    if (content_widget_) {
        // Pobieramy sizeHint zawartości (czyli AutoScalingAttachment)
        const QSize content_hint = content_widget_->sizeHint();
        if (content_hint.isValid()) {
            hint.setWidth(content_hint.width() + extra_width);
            hint.setHeight(content_hint.height() + extra_height);
            return hint;
        }
        // Jeśli contentHint jest nieprawidłowy, spróbujmy rozmiar widgetu
        const QSize content_size = content_widget_->size();
        if (content_size.isValid() && content_size.width() > 0) {
            hint.setWidth(content_size.width() + extra_width);
            hint.setHeight(content_size.height() + extra_height);
            return hint;
        }
    }

    // Podstawowy rozmiar, jeśli nie mamy zawartości lub jej rozmiar jest nieznany
    constexpr QSize default_size(200, 100); // Zmniejszony domyślny rozmiar
    hint.setWidth(default_size.width() + extra_width);
    hint.setHeight(default_size.height() + extra_height);
    return hint;
}

void CyberAttachmentViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // Dopasuj rozmiar i pozycję maski do kontenera zawartości
    if (content_container_ && mask_overlay_) {
        mask_overlay_->setGeometry(content_container_->rect());
        mask_overlay_->raise(); // Upewnij się, że jest na wierzchu
    }
}

void CyberAttachmentViewer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Ramki w stylu AR
    constexpr QColor borderColor(0, 200, 255);
    painter.setPen(QPen(borderColor, 1));

    // Technologiczna ramka
    QPainterPath frame;
    constexpr int clip_size = 15;

    // Górna krawędź
    frame.moveTo(clip_size, 0);
    frame.lineTo(width() - clip_size, 0);

    // Prawy górny róg
    frame.lineTo(width(), clip_size);

    // Prawa krawędź
    frame.lineTo(width(), height() - clip_size);

    // Prawy dolny róg
    frame.lineTo(width() - clip_size, height());

    // Dolna krawędź
    frame.lineTo(clip_size, height());

    // Lewy dolny róg
    frame.lineTo(0, height() - clip_size);

    // Lewa krawędź
    frame.lineTo(0, clip_size);

    // Lewy górny róg
    frame.lineTo(clip_size, 0);

    painter.drawPath(frame);

    // Znaczniki AR w rogach
    painter.setPen(QPen(borderColor, 1, Qt::SolidLine));
    constexpr int marker_size = 8;

    // Prawy dolny
    painter.drawLine(width() - clip_size - marker_size, height() - 5, width() - clip_size, height() - 5);
    painter.drawLine(width() - clip_size, height() - 5, width() - clip_size, height() - 5 - marker_size);

    // Lewy dolny
    painter.drawLine(clip_size, height() - 5, clip_size + marker_size, height() - 5);
    painter.drawLine(clip_size, height() - 5, clip_size, height() - 5 - marker_size);

    // Techniczne dane w rogach
    painter.setPen(borderColor);
    painter.setFont(QFont("Consolas", 7));

    // Lewy dolny - poziom zabezpieczeń
    const int security_level = is_decrypted_ ? 0 : QRandomGenerator::global()->bounded(1, 6);
    painter.drawText(20, height() - 10, QString("SEC:LVL%1").arg(security_level));

    // Prawy dolny - status
    const QString status = is_decrypted_ ? translator_->Translate("CyberAttachmentViewer.Unlocked",
    "UNLOCKED") : translator_->Translate("CyberAttachmentViewer.Locked",
    "LOCKED");
    painter.drawText(width() - 60, height() - 10, status);
}

void CyberAttachmentViewer::OnActionButtonClicked() {
    // Logika pozostaje podobna, ale operuje na stanie i masce
    if (!is_decrypted_) {
        if (!is_scanning_) {
            StartScanningAnimation(); // Rozpoczyna tylko wizualne skanowanie na masce
        }
        // Nie ma już potrzeby ręcznego klikania, aby rozpocząć deszyfrację,
        // startDecryptionAnimation jest wywoływane automatycznie po skanowaniu.
    } else {
        CloseViewer();
    }
}

void CyberAttachmentViewer::StartScanningAnimation() {
    if (!content_widget_) return; // Nie rób nic, jeśli nie ma zawartości

    is_scanning_ = true;
    is_decrypted_ = false;
    SetDecryptionCounter(0); // Resetuj postęp
    status_label_->setText(translator_->Translate("CyberAttachmentViewer.Scanning",
            "SECURITY SCANNING..."));

    // *** ZMIANA: Nie pokazujemy m_contentWidget ***
    // m_contentWidget->setVisible(true); // USUNIĘTE

    // *** ZMIANA: Upewnij się, że maska jest widoczna i animowana ***
    mask_overlay_->setGeometry(content_container_->rect()); // Upewnij się co do rozmiaru
    mask_overlay_->raise();
    mask_overlay_->StartScanning(); // Rozpocznij/kontynuuj animację skanowania

    // Używamy timera do symulacji czasu skanowania przed deszyfracją
    QTimer::singleShot(2000, this, [this]() { // Czas trwania "skanowania"
                           if (is_scanning_) { // Sprawdź, czy nadal jesteśmy w trybie skanowania
                               status_label_->setText(translator_->Translate("CyberAttachmentViewer.ScanningCompleted",
            "SECURITY SCAN COMPLETED. DECRYPTING..."));
                               // Automatycznie rozpocznij dekodowanie po krótkim opóźnieniu
                               QTimer::singleShot(800, this, &CyberAttachmentViewer::StartDecryptionAnimation);
                           }
                       });

    update(); // Przerenderuj ramki itp.
}

void CyberAttachmentViewer::StartDecryptionAnimation() {
    if (!content_widget_) return; // Nie rób nic, jeśli nie ma zawartości
    if (is_decrypted_) return;   // Nie zaczynaj, jeśli już zakończono

    is_scanning_ = false; // Skanowanie zakończone, zaczyna się deszyfracja
    status_label_->setText(translator_->Translate("CyberAttachmentViewer.StartDecrypting",
            "STARTING DECRYPTION... 0%"));

    // *** ZMIANA: Maska nadal jest widoczna, ale zacznie się odsłaniać ***
    mask_overlay_->setVisible(true);
    mask_overlay_->raise();
    // Animacja licznika (która przez sygnał/slot aktualizuje maskę)
    const auto decryption_animation = new QPropertyAnimation(this, "decryptionCounter");
    decryption_animation->setDuration(6000);
    decryption_animation->setStartValue(0);
    decryption_animation->setEndValue(100);
    decryption_animation->setEasingCurve(QEasingCurve::OutQuad);

    connect(decryption_animation, &QPropertyAnimation::finished, this, &CyberAttachmentViewer::FinishDecryption);

    decryption_animation->start(QPropertyAnimation::DeleteWhenStopped);

    // Timer animacji (np. dla glitchy w tekście) nie jest już potrzebny do maski
    // m_animTimer->start(50); // Można zostawić dla innych efektów jeśli są
}

void CyberAttachmentViewer::UpdateAnimation() const {
    // Losowe znaki hackowania w etykiecie statusu
    if (!is_decrypted_) {
        // Aktualizujemy tekst czasami z losowymi zakłóceniami
        if (QRandomGenerator::global()->bounded(100) < 30) {
            QString base_text = status_label_->text();
            if (base_text.contains("%")) {
                base_text = QString("%1 %2%")
                .arg(translator_->Translate("CyberAttachmentViewer.Decrypting", "DECRYPTING..."))
                .arg(decryption_counter_);
            }

            // Dodaj losowe znaki
            const int char_to_glitch = QRandomGenerator::global()->bounded(base_text.length());
            if (char_to_glitch < base_text.length()) {
                const auto glitchChar = QChar(QRandomGenerator::global()->bounded(33, 126));
                base_text[char_to_glitch] = glitchChar;
            }

            status_label_->setText(base_text);
        }
    }
}

void CyberAttachmentViewer::UpdateDecryptionStatus() const {
    // Aktualizuje tylko etykietę tekstową
    status_label_->setText(QString("%1 %2%")
    .arg(translator_->Translate("CyberAttachmentViewer.Decrypting", "DECRYPTING..."))
        .arg(decryption_counter_));
    // Nie trzeba już ręcznie aktualizować maski, robi to sygnał decryptionCounterChanged
}

void CyberAttachmentViewer::FinishDecryption() {
    // m_animTimer->stop(); // Zatrzymaj, jeśli był używany
    is_decrypted_ = true;
    is_scanning_ = false;

    status_label_->setText(translator_->Translate("CyberAttachmentViewer.DecryptingCompleted", "DECRYPTION COMPLETED - ACCESS GRANTED"));

    // *** ZMIANA: Ukryj maskę i pokaż zawartość ***
    mask_overlay_->StopScanning(); // Zatrzymuje timer i ukrywa maskę
    if (content_widget_) {
        content_widget_->setVisible(true); // Pokaż finalną zawartość
    }

    update(); // Przerenderuj stan końcowy (np. status UNLOCKED)
}

void CyberAttachmentViewer::CloseViewer() {
    // Emitujemy sygnał zakończenia
    emit viewingFinished();
}
