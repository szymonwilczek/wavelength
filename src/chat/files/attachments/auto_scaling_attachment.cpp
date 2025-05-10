#include "auto_scaling_attachment.h"

#include <QVBoxLayout>

#include "../../../app/managers/translation_manager.h"
#include "../gif/player/inline_gif_player.h"
#include "../image/displayer/image_viewer.h"

AutoScalingAttachment::AutoScalingAttachment(QWidget *content, QWidget *parent): QWidget(parent), content_(content),
    is_scaled_(false) {
    const TranslationManager *translator = TranslationManager::GetInstance();

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    content_container_ = new QWidget(this);
    const auto content_layout = new QVBoxLayout(content_container_);
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(0);
    content_layout->addWidget(content_);
    layout->addWidget(content_container_, 0, Qt::AlignCenter);

    info_label_ = new QLabel(translator->Translate("Attachments.Expand", "EXPAND"), this);
    info_label_->setStyleSheet(
        "QLabel {"
        "  color: #00ccff;"
        "  background-color: rgba(0, 24, 34, 200);"
        "  border: 1px solid #00aaff;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 8pt;"
        "  padding: 3px 8px;"
        "  border-radius: 3px;"
        "}"
    );
    info_label_->setAlignment(Qt::AlignCenter);
    info_label_->adjustSize();
    info_label_->hide();
    info_label_->setAttribute(Qt::WA_TransparentForMouseEvents);

    setMouseTracking(true);
    content_container_->setMouseTracking(true);
    content_container_->installEventFilter(this);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    content_container_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    content_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setCursor(Qt::PointingHandCursor);
    content_container_->setCursor(Qt::PointingHandCursor);

    const auto image_viewer = qobject_cast<InlineImageViewer *>(content_);
    const auto gif_player = qobject_cast<InlineGifPlayer *>(content_);
    if (image_viewer) {
        connect(image_viewer, &InlineImageViewer::imageLoaded, this, &AutoScalingAttachment::CheckAndScaleContent);
        connect(image_viewer, &InlineImageViewer::imageInfoReady, this, &AutoScalingAttachment::CheckAndScaleContent);
    } else if (gif_player) {
        connect(gif_player, &InlineGifPlayer::gifLoaded, this, &AutoScalingAttachment::CheckAndScaleContent);
    } else {
        QTimer::singleShot(50, this, &AutoScalingAttachment::CheckAndScaleContent);
    }
}

void AutoScalingAttachment::SetMaxAllowedSize(const QSize &max_size) {
    max_allowed_size_ = max_size;
    CheckAndScaleContent();
}

QSize AutoScalingAttachment::ContentOriginalSize() const {
    if (content_) {
        const QSize hint = content_->sizeHint();
        if (hint.isValid()) {
            return hint;
        }
    }
    return QSize();
}

QSize AutoScalingAttachment::sizeHint() const {
    QSize content_size = content_container_->size();
    if (!content_size.isValid() || content_size.width() <= 0 || content_size.height() <= 0) {
        content_size = content_ ? content_->sizeHint() : QSize();
    }

    if (content_size.isValid() && content_size.width() > 0 && content_size.height() > 0) {
        return content_size;
    }

    return QWidget::sizeHint().isValid() ? QWidget::sizeHint() : QSize(100, 50);
}

bool AutoScalingAttachment::eventFilter(QObject *watched, QEvent *event) {
    if (watched == content_container_ && event->type() == QEvent::MouseButtonRelease) {
        const auto me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            emit clicked();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AutoScalingAttachment::enterEvent(QEvent *event) {
    if (is_scaled_) {
        UpdateInfoLabelPosition();
        info_label_->raise();
        info_label_->show();
    }
    QWidget::enterEvent(event);
}

void AutoScalingAttachment::leaveEvent(QEvent *event) {
    info_label_->hide();
    QWidget::leaveEvent(event);
}

void AutoScalingAttachment::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (info_label_->isVisible()) {
        UpdateInfoLabelPosition();
    }
}

void AutoScalingAttachment::CheckAndScaleContent() {
    if (!content_) {
        qDebug() << "[AUTO SCALING ATTACHMENT]::checkAndScaleContent - Lack of content.";
        return;
    }

    QSize original_size = content_->sizeHint();
    if (!original_size.isValid() || original_size.width() <= 0 || original_size.height() <= 0) {
        qDebug() << "[AUTO SCALING ATTACHMENT]::checkAndScaleContent - Invalid sizeHint of content:" << original_size;
        content_->updateGeometry();
        original_size = content_->sizeHint();
        if (!original_size.isValid() || original_size.width() <= 0 || original_size.height() <= 0) {
            qDebug() << "[AUTO SCALING ATTACHMENT]::checkAndScaleContent - Still invalid sizeHint.";
            return;
        }
    }

    QSize max_size = max_allowed_size_;
    if (!max_size.isValid()) {
        max_size = QSize(400, 300);
    }

    const bool needs_scaling = original_size.width() > max_size.width() ||
                               original_size.height() > max_size.height();

    QSize target_size;
    if (needs_scaling) {
        const qreal scale_x = static_cast<qreal>(max_size.width()) / original_size.width();
        const qreal scale_y = static_cast<qreal>(max_size.height()) / original_size.height();
        const qreal scale = qMin(scale_x, scale_y);
        target_size = QSize(qMax(1, static_cast<int>(original_size.width() * scale)),
                            qMax(1, static_cast<int>(original_size.height() * scale)));
        is_scaled_ = true;
    } else {
        target_size = original_size;
        is_scaled_ = false;
        info_label_->hide();
    }

    content_container_->setFixedSize(target_size);
    scaled_size_ = target_size;
    updateGeometry();

    if (info_label_->isVisible()) {
        UpdateInfoLabelPosition();
    }

    if (parentWidget()) {
        QTimer::singleShot(0, parentWidget(), [parent = parentWidget()]() {
            if (parent->layout()) parent->layout()->activate();
            parent->updateGeometry();
        });
    }
}

void AutoScalingAttachment::UpdateInfoLabelPosition() const {
    if (!content_container_) return;
    int label_x = (content_container_->width() - info_label_->width()) / 2;
    int label_y = content_container_->height() - info_label_->height() - 5;
    label_x += content_container_->pos().x();
    label_y += content_container_->pos().y();
    info_label_->move(label_x, label_y);
}
