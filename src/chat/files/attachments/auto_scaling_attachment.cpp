#include "auto_scaling_attachment.h"

AutoScalingAttachment::AutoScalingAttachment(QWidget *content, QWidget *parent): QWidget(parent), m_content(content), m_isScaled(false) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Dodaj kontener zawartości
    m_contentContainer = new QWidget(this);
    const auto contentLayout = new QVBoxLayout(m_contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(m_content);
    layout->addWidget(m_contentContainer, 0, Qt::AlignCenter);

    // Etykieta informująca o możliwości powiększenia (pokazywana tylko dla przeskalowanych)
    m_infoLabel = new QLabel("Kliknij, aby powiększyć", this); // Rodzicem jest AutoScalingAttachment
    m_infoLabel->setStyleSheet(
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
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->adjustSize(); // Dopasuj rozmiar do tekstu
    m_infoLabel->hide();       // Ukryj na początku
    m_infoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Włącz obsługę zdarzeń myszy
    setMouseTracking(true);
    m_contentContainer->setMouseTracking(true);
    m_contentContainer->installEventFilter(this);

    // Ustaw odpowiednią politykę rozmiaru
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // Ustaw politykę rozmiaru dla kontenera, aby mógł się rozszerzać
    m_contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Ustaw politykę rozmiaru dla zawartości, aby wypełniała kontener
    m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Ustaw kursor wskazujący dla skalowanych załączników
    setCursor(Qt::PointingHandCursor);
    m_contentContainer->setCursor(Qt::PointingHandCursor);

    const auto imgView = qobject_cast<InlineImageViewer*>(m_content);
    const auto gifPlay = qobject_cast<InlineGifPlayer*>(m_content);
    if (imgView) {
        connect(imgView, &InlineImageViewer::imageLoaded, this, &AutoScalingAttachment::checkAndScaleContent);
        connect(imgView, &InlineImageViewer::imageInfoReady, this, &AutoScalingAttachment::checkAndScaleContent);
    } else if (gifPlay) {
        connect(gifPlay, &InlineGifPlayer::gifLoaded, this, &AutoScalingAttachment::checkAndScaleContent);
    } else {
        QTimer::singleShot(50, this, &AutoScalingAttachment::checkAndScaleContent);
    }
}

void AutoScalingAttachment::setMaxAllowedSize(const QSize &maxSize) {
    m_maxAllowedSize = maxSize;
    checkAndScaleContent();
}

QSize AutoScalingAttachment::contentOriginalSize() const {
    if (m_content) {
        // Używamy sizeHint jako preferowanego oryginalnego rozmiaru
        const QSize hint = m_content->sizeHint();
        if (hint.isValid()) {
            return hint;
        }
    }
    return QSize(); // Zwróć nieprawidłowy rozmiar, jeśli nie można określić
}

QSize AutoScalingAttachment::sizeHint() const {
    // sizeHint nie musi uwzględniać etykiety, bo jest pozycjonowana absolutnie
    QSize currentContentSize = m_contentContainer->size();
    if (!currentContentSize.isValid() || currentContentSize.width() <= 0 || currentContentSize.height() <= 0) {
        currentContentSize = m_content ? m_content->sizeHint() : QSize();
    }

    if (currentContentSize.isValid() && currentContentSize.width() > 0 && currentContentSize.height() > 0) {
        // Zwracamy tylko rozmiar kontenera
        return currentContentSize;
    }

    // Zwróć domyślny minimalny rozmiar
    return QWidget::sizeHint().isValid() ? QWidget::sizeHint() : QSize(100,50);
}

bool AutoScalingAttachment::eventFilter(QObject *watched, QEvent *event) {
    // Zmieniamy watched na m_contentContainer
    if (watched == m_contentContainer && event->type() == QEvent::MouseButtonRelease) {
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
    if (m_isScaled) {
        updateInfoLabelPosition(); // Ustaw pozycję
        m_infoLabel->raise();      // Upewnij się, że jest na wierzchu
        m_infoLabel->show();       // Pokaż
    }
    QWidget::enterEvent(event);
}

void AutoScalingAttachment::leaveEvent(QEvent *event) {
    m_infoLabel->hide(); // Ukryj po zjechaniu
    QWidget::leaveEvent(event);
}

void AutoScalingAttachment::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // Aktualizuj pozycję etykiety, jeśli jest widoczna
    if (m_infoLabel->isVisible()) {
        updateInfoLabelPosition();
    }
}

void AutoScalingAttachment::checkAndScaleContent() {
    if (!m_content) {
        qDebug() << "AutoScalingAttachment::checkAndScaleContent - Brak zawartości.";
        return;
    }

    // Pobierz oryginalny rozmiar zawartości z sizeHint
    QSize originalSize = m_content->sizeHint();
    if (!originalSize.isValid() || originalSize.width() <= 0 || originalSize.height() <= 0) {
        qDebug() << "AutoScalingAttachment::checkAndScaleContent - Nieprawidłowy sizeHint zawartości:" << originalSize;
        // Spróbuj wymusić aktualizację sizeHint
        m_content->updateGeometry();
        originalSize = m_content->sizeHint();
        if (!originalSize.isValid() || originalSize.width() <= 0 || originalSize.height() <= 0) {
            qDebug() << "AutoScalingAttachment::checkAndScaleContent - Nadal nieprawidłowy sizeHint.";
            // Można spróbować poczekać chwilę i spróbować ponownie
            // QTimer::singleShot(100, this, &AutoScalingAttachment::checkAndScaleContent);
            return;
        }
    }
    qDebug() << "AutoScalingAttachment::checkAndScaleContent - Oryginalny sizeHint zawartości:" << originalSize;


    // Pobierz maksymalny dostępny rozmiar
    QSize maxSize = m_maxAllowedSize;
    if (!maxSize.isValid()) {
        maxSize = QSize(400, 300);
    }

    const bool needsScaling = originalSize.width() > maxSize.width() ||
                        originalSize.height() > maxSize.height();

    QSize targetSize;
    if (needsScaling) {
        const qreal scaleX = static_cast<qreal>(maxSize.width()) / originalSize.width();
        const qreal scaleY = static_cast<qreal>(maxSize.height()) / originalSize.height();
        const qreal scale = qMin(scaleX, scaleY);
        targetSize = QSize(qMax(1, static_cast<int>(originalSize.width() * scale)),
                           qMax(1, static_cast<int>(originalSize.height() * scale)));
        m_isScaled = true;
        // m_infoLabel->setText("Kliknij, aby powiększyć"); // Tekst jest stały
    } else {
        targetSize = originalSize;
        m_isScaled = false;
        m_infoLabel->hide(); // Ukryj, jeśli nie skalujemy
    }

    m_contentContainer->setFixedSize(targetSize);
    m_scaledSize = targetSize;
    updateGeometry();

    // Aktualizuj pozycję etykiety, jeśli jest widoczna (np. po zmianie rozmiaru)
    if (m_infoLabel->isVisible()) {
        updateInfoLabelPosition();
    }

    // Wymuś aktualizację layoutu rodzica
    if (parentWidget()) {
        QTimer::singleShot(0, parentWidget(), [parent = parentWidget()]() {
            if (parent->layout()) parent->layout()->activate();
            parent->updateGeometry();
        });
    }
}

void AutoScalingAttachment::updateInfoLabelPosition() const {
    if (!m_contentContainer) return;
    // Pozycjonuj etykietę na dole, na środku kontenera zawartości
    int labelX = (m_contentContainer->width() - m_infoLabel->width()) / 2;
    int labelY = m_contentContainer->height() - m_infoLabel->height() - 5; // 5px od dołu
    // Przesuń względem pozycji kontenera w AutoScalingAttachment
    labelX += m_contentContainer->pos().x();
    labelY += m_contentContainer->pos().y();
    m_infoLabel->move(labelX, labelY);
}
