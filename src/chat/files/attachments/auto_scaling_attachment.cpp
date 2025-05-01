#include "auto_scaling_attachment.h"

#include <QVBoxLayout>

AutoScalingAttachment::AutoScalingAttachment(QWidget *content, QWidget *parent): QWidget(parent), content_(content), is_scaled_(false) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Dodaj kontener zawartości
    content_container_ = new QWidget(this);
    const auto content_layout = new QVBoxLayout(content_container_);
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(0);
    content_layout->addWidget(content_);
    layout->addWidget(content_container_, 0, Qt::AlignCenter);

    // Etykieta informująca o możliwości powiększenia (pokazywana tylko dla przeskalowanych)
    info_label_ = new QLabel("Kliknij, aby powiększyć", this); // Rodzicem jest AutoScalingAttachment
    info_label_->setStyleSheet(
        "QLabel {"
        "  color: #00ccff;"
        "  background-color: rgba(0, 24, 34, 200);" // Zwiększona nieprzezroczystość tła
        "  border: 1px solid #00aaff;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 8pt;"
        "  padding: 3px 8px;" // Zwiększony padding
        "  border-radius: 3px;" // Zaokrąglenie
        "}"
    );
    info_label_->setAlignment(Qt::AlignCenter);
    info_label_->adjustSize(); // Dopasuj rozmiar do tekstu
    info_label_->hide();       // Ukryj na początku
    info_label_->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Włącz obsługę zdarzeń myszy
    setMouseTracking(true);
    content_container_->setMouseTracking(true);
    content_container_->installEventFilter(this);

    // Ustaw odpowiednią politykę rozmiaru
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // Ustaw politykę rozmiaru dla kontenera, aby mógł się rozszerzać
    content_container_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Ustaw politykę rozmiaru dla zawartości, aby wypełniała kontener
    content_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Ustaw kursor wskazujący dla skalowanych załączników
    setCursor(Qt::PointingHandCursor);
    content_container_->setCursor(Qt::PointingHandCursor);

    const auto image_viewer = qobject_cast<InlineImageViewer*>(content_);
    const auto gif_player = qobject_cast<InlineGifPlayer*>(content_);
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
        // Używamy sizeHint jako preferowanego oryginalnego rozmiaru
        const QSize hint = content_->sizeHint();
        if (hint.isValid()) {
            return hint;
        }
    }
    return QSize(); // Zwróć nieprawidłowy rozmiar, jeśli nie można określić
}

QSize AutoScalingAttachment::sizeHint() const {
    // sizeHint nie musi uwzględniać etykiety, bo jest pozycjonowana absolutnie
    QSize content_size = content_container_->size();
    if (!content_size.isValid() || content_size.width() <= 0 || content_size.height() <= 0) {
        content_size = content_ ? content_->sizeHint() : QSize();
    }

    if (content_size.isValid() && content_size.width() > 0 && content_size.height() > 0) {
        // Zwracamy tylko rozmiar kontenera
        return content_size;
    }

    // Zwróć domyślny minimalny rozmiar
    return QWidget::sizeHint().isValid() ? QWidget::sizeHint() : QSize(100,50);
}

bool AutoScalingAttachment::eventFilter(QObject *watched, QEvent *event) {
    // Zmieniamy watched na m_contentContainer
    if (watched == content_container_ && event->type() == QEvent::MouseButtonRelease) {
        const auto me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            qDebug() << "AutoScalingAttachment: Kliknięto kontener!";
            emit clicked(); // Emituj zawsze po kliknięciu
            return true; // Zawsze przechwytuj kliknięcie na kontenerze
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AutoScalingAttachment::enterEvent(QEvent *event) {
    if (is_scaled_) {
        UpdateInfoLabelPosition(); // Ustaw pozycję
        info_label_->raise();      // Upewnij się, że jest na wierzchu
        info_label_->show();       // Pokaż
    }
    QWidget::enterEvent(event);
}

void AutoScalingAttachment::leaveEvent(QEvent *event) {
    info_label_->hide(); // Ukryj po zjechaniu
    QWidget::leaveEvent(event);
}

void AutoScalingAttachment::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // Aktualizuj pozycję etykiety, jeśli jest widoczna
    if (info_label_->isVisible()) {
        UpdateInfoLabelPosition();
    }
}

void AutoScalingAttachment::CheckAndScaleContent() {
    if (!content_) {
        qDebug() << "AutoScalingAttachment::checkAndScaleContent - Brak zawartości.";
        return;
    }

    // Pobierz oryginalny rozmiar zawartości z sizeHint
    QSize original_size = content_->sizeHint();
    if (!original_size.isValid() || original_size.width() <= 0 || original_size.height() <= 0) {
        qDebug() << "AutoScalingAttachment::checkAndScaleContent - Nieprawidłowy sizeHint zawartości:" << original_size;
        // Spróbuj wymusić aktualizację sizeHint
        content_->updateGeometry();
        original_size = content_->sizeHint();
        if (!original_size.isValid() || original_size.width() <= 0 || original_size.height() <= 0) {
            qDebug() << "AutoScalingAttachment::checkAndScaleContent - Nadal nieprawidłowy sizeHint.";
            // Można spróbować poczekać chwilę i spróbować ponownie
            // QTimer::singleShot(100, this, &AutoScalingAttachment::checkAndScaleContent);
            return;
        }
    }
    qDebug() << "AutoScalingAttachment::checkAndScaleContent - Oryginalny sizeHint zawartości:" << original_size;


    // Pobierz maksymalny dostępny rozmiar
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

    // Aktualizuj pozycję etykiety, jeśli jest widoczna (np. po zmianie rozmiaru)
    if (info_label_->isVisible()) {
        UpdateInfoLabelPosition();
    }

    // Wymuś aktualizację layoutu rodzica
    if (parentWidget()) {
        QTimer::singleShot(0, parentWidget(), [parent = parentWidget()]() {
            if (parent->layout()) parent->layout()->activate();
            parent->updateGeometry();
        });
    }
}

void AutoScalingAttachment::UpdateInfoLabelPosition() const {
    if (!content_container_) return;
    // Pozycjonuj etykietę na dole, na środku kontenera zawartości
    int label_x = (content_container_->width() - info_label_->width()) / 2;
    int label_y = content_container_->height() - info_label_->height() - 5; // 5px od dołu
    // Przesuń względem pozycji kontenera w AutoScalingAttachment
    label_x += content_container_->pos().x();
    label_y += content_container_->pos().y();
    info_label_->move(label_x, label_y);
}
