#include "cyber_attachment_viewer.h"

#include <QPainterPath>
#include <QPropertyAnimation>

#include "../../app/managers/translation_manager.h"

CyberAttachmentViewer::CyberAttachmentViewer(QWidget *parent): QWidget(parent), decryption_counter_(0),
                                                               is_scanning_(false), is_decrypted_(false) {
    translator_ = TranslationManager::GetInstance();

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(10, 10, 10, 10);
    layout_->setSpacing(10);

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

    content_container_ = new QWidget(this);
    content_layout_ = new QVBoxLayout(content_container_);
    content_layout_->setContentsMargins(0, 0, 0, 0); // Zmieniono marginesy na 0
    layout_->addWidget(content_container_, 1);

    mask_overlay_ = new MaskOverlay(content_container_);
    mask_overlay_->setVisible(false);

    animation_timer_ = new QTimer(this);
    connect(animation_timer_, &QTimer::timeout, this, &CyberAttachmentViewer::UpdateAnimation);

    setStyleSheet(
        "CyberAttachmentViewer {"
        "  background-color: rgba(10, 20, 30, 200);"
        "  border: 1px solid #00aaff;"
        "}"
    );

    setMinimumSize(0, 0);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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
        emit decryptionCounterChanged(decryption_counter_);
    }
}

void CyberAttachmentViewer::UpdateContentLayout() {
    if (content_widget_) {
        content_layout_->invalidate();
        content_layout_->activate();

        content_widget_->updateGeometry();
        updateGeometry();

        QTimer::singleShot(50, this, [this]() {
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

    content->setVisible(false);
    mask_overlay_->setVisible(true);
    mask_overlay_->raise();
    mask_overlay_->StartScanning();

    status_label_->setText(
        translator_->Translate("CyberAttachmentViewer.EncryptedDataDetected", "ENCRYPTED DATA DETECTED"));
    is_decrypted_ = false;
    is_scanning_ = false;
    SetDecryptionCounter(0);

    content_layout_->activate();
    updateGeometry();

    QTimer::singleShot(10, this, [this]() {
        if (content_widget_ && mask_overlay_) {
            mask_overlay_->setGeometry(content_container_->rect());
            mask_overlay_->raise();
        }
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
        const QSize content_hint = content_widget_->sizeHint();
        if (content_hint.isValid()) {
            hint.setWidth(content_hint.width() + extra_width);
            hint.setHeight(content_hint.height() + extra_height);
            return hint;
        }

        const QSize content_size = content_widget_->size();
        if (content_size.isValid() && content_size.width() > 0) {
            hint.setWidth(content_size.width() + extra_width);
            hint.setHeight(content_size.height() + extra_height);
            return hint;
        }
    }

    constexpr QSize default_size(200, 100);
    hint.setWidth(default_size.width() + extra_width);
    hint.setHeight(default_size.height() + extra_height);
    return hint;
}

void CyberAttachmentViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (content_container_ && mask_overlay_) {
        mask_overlay_->setGeometry(content_container_->rect());
        mask_overlay_->raise();
    }
}

void CyberAttachmentViewer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    constexpr QColor borderColor(0, 200, 255);
    painter.setPen(QPen(borderColor, 1));

    QPainterPath frame;
    constexpr int clip_size = 15;

    // top edge
    frame.moveTo(clip_size, 0);
    frame.lineTo(width() - clip_size, 0);

    // right-top corner
    frame.lineTo(width(), clip_size);

    // right edge
    frame.lineTo(width(), height() - clip_size);

    // right-bottom corner
    frame.lineTo(width() - clip_size, height());

    // bottom edge
    frame.lineTo(clip_size, height());

    // left-bottom corner
    frame.lineTo(0, height() - clip_size);

    // left edge
    frame.lineTo(0, clip_size);

    // left-top corner
    frame.lineTo(clip_size, 0);

    painter.drawPath(frame);

    painter.setPen(QPen(borderColor, 1, Qt::SolidLine));
    constexpr int marker_size = 8;

    // right bottom
    painter.drawLine(width() - clip_size - marker_size, height() - 5, width() - clip_size, height() - 5);
    painter.drawLine(width() - clip_size, height() - 5, width() - clip_size, height() - 5 - marker_size);

    // left bottom
    painter.drawLine(clip_size, height() - 5, clip_size + marker_size, height() - 5);
    painter.drawLine(clip_size, height() - 5, clip_size, height() - 5 - marker_size);

    painter.setPen(borderColor);
    painter.setFont(QFont("Consolas", 7));

    // left bottom: security level
    const int security_level = is_decrypted_ ? 0 : QRandomGenerator::global()->bounded(1, 6);
    painter.drawText(20, height() - 10, QString("SEC:LVL%1").arg(security_level));

    // right bottom: locked/unlocked status
    const QString status = is_decrypted_
                               ? translator_->Translate("CyberAttachmentViewer.Unlocked",
                                                        "UNLOCKED")
                               : translator_->Translate("CyberAttachmentViewer.Locked",
                                                        "LOCKED");
    painter.drawText(width() - 60, height() - 10, status);
}

void CyberAttachmentViewer::OnActionButtonClicked() {
    if (!is_decrypted_) {
        if (!is_scanning_) {
            StartScanningAnimation();
        }
    } else {
        CloseViewer();
    }
}

void CyberAttachmentViewer::StartScanningAnimation() {
    if (!content_widget_) return;

    is_scanning_ = true;
    is_decrypted_ = false;
    SetDecryptionCounter(0);
    status_label_->setText(
        translator_->Translate("CyberAttachmentViewer.Scanning", "SECURITY SCANNING..."));

    mask_overlay_->setGeometry(content_container_->rect());
    mask_overlay_->raise();
    mask_overlay_->StartScanning();

    QTimer::singleShot(2000, this, [this]() {
        if (is_scanning_) {
            status_label_->setText(
                translator_->Translate("CyberAttachmentViewer.ScanningCompleted",
                                       "SECURITY SCAN COMPLETED. DECRYPTING..."));
            QTimer::singleShot(800, this, &CyberAttachmentViewer::StartDecryptionAnimation);
        }
    });

    update();
}

void CyberAttachmentViewer::StartDecryptionAnimation() {
    if (!content_widget_) return;
    if (is_decrypted_) return;

    is_scanning_ = false;
    status_label_->setText(
        translator_->Translate("CyberAttachmentViewer.StartDecrypting",
                               "STARTING DECRYPTION... 0%"));

    mask_overlay_->setVisible(true);
    mask_overlay_->raise();

    const auto decryption_animation = new QPropertyAnimation(this, "decryptionCounter");
    decryption_animation->setDuration(6000);
    decryption_animation->setStartValue(0);
    decryption_animation->setEndValue(100);
    decryption_animation->setEasingCurve(QEasingCurve::OutQuad);

    connect(decryption_animation, &QPropertyAnimation::finished, this, &CyberAttachmentViewer::FinishDecryption);
    decryption_animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void CyberAttachmentViewer::UpdateAnimation() const {
    if (!is_decrypted_) {
        if (QRandomGenerator::global()->bounded(100) < 30) {
            QString base_text = status_label_->text();
            if (base_text.contains("%")) {
                base_text = QString("%1 %2%")
                        .arg(translator_->Translate("CyberAttachmentViewer.Decrypting", "DECRYPTING..."))
                        .arg(decryption_counter_);
            }

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
    status_label_->setText(QString("%1 %2%")
        .arg(translator_->Translate("CyberAttachmentViewer.Decrypting", "DECRYPTING..."))
        .arg(decryption_counter_));
}

void CyberAttachmentViewer::FinishDecryption() {
    is_decrypted_ = true;
    is_scanning_ = false;

    status_label_->setText(
        translator_->Translate("CyberAttachmentViewer.DecryptingCompleted",
                               "DECRYPTION COMPLETED - ACCESS GRANTED"));

    mask_overlay_->StopScanning();
    if (content_widget_) {
        content_widget_->setVisible(true);
    }

    update();
}

void CyberAttachmentViewer::CloseViewer() {
    emit viewingFinished();
}
