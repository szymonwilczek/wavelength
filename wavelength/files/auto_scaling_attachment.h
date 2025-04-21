#ifndef AUTO_SCALING_ATTACHMENT_H
#define AUTO_SCALING_ATTACHMENT_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QPalette>
#include <QDebug>
#include <QTimer>
#include <QEvent>

#include "gif/player/inline_gif_player.h"
#include "image/displayer/image_viewer.h"

class AutoScalingAttachment : public QWidget {
    Q_OBJECT

public:
    AutoScalingAttachment(QWidget* content, QWidget* parent = nullptr)
    : QWidget(parent), m_content(content), m_isScaled(false)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Dodaj kontener zawartości
        m_contentContainer = new QWidget(this);
        QVBoxLayout* contentLayout = new QVBoxLayout(m_contentContainer);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(0);
        contentLayout->addWidget(m_content);
        // m_content->setParent(m_contentContainer);

        layout->addWidget(m_contentContainer, 0, Qt::AlignCenter);

        // Etykieta informująca o możliwości powiększenia (pokazywana tylko dla przeskalowanych)
        m_infoLabel = new QLabel("Kliknij, aby powiększyć", this);
        m_infoLabel->setStyleSheet(
            "QLabel {"
            "  color: #00ccff;"
            "  background-color: rgba(0, 24, 34, 180);"
            "  border: 1px solid #00aaff;"
            "  font-family: 'Consolas', monospace;"
            "  font-size: 8pt;"
            "  padding: 2px 6px;"
            "  border-radius: 2px;"
            "}"
        );
        m_infoLabel->setAlignment(Qt::AlignCenter);
        m_infoLabel->hide();
        layout->addWidget(m_infoLabel, 0, Qt::AlignHCenter);

        // Włącz obsługę zdarzeń myszy
        setMouseTracking(true);
        m_contentContainer->setMouseTracking(true);
        // m_content->setMouseTracking(true);
        // m_content->installEventFilter(this);
        m_contentContainer->installEventFilter(this);

        // Ustaw odpowiednią politykę rozmiaru
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        m_contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // Ustaw kursor wskazujący dla skalowanych załączników
        setCursor(Qt::PointingHandCursor);

        InlineImageViewer* imgView = qobject_cast<InlineImageViewer*>(m_content);
        InlineGifPlayer* gifPlay = qobject_cast<InlineGifPlayer*>(m_content);
        if (imgView) {
            connect(imgView, &InlineImageViewer::imageLoaded, this, &AutoScalingAttachment::checkAndScaleContent);
            // W przypadku błędu też spróbujmy ustawić rozmiar
            connect(imgView, &InlineImageViewer::imageInfoReady, this, &AutoScalingAttachment::checkAndScaleContent);
        } else if (gifPlay) {
            connect(gifPlay, &InlineGifPlayer::gifLoaded, this, &AutoScalingAttachment::checkAndScaleContent);
        } else {
            // Dla innych typów zawartości, spróbuj od razu
            QTimer::singleShot(50, this, &AutoScalingAttachment::checkAndScaleContent);
        }
    }

    void setMaxAllowedSize(const QSize& maxSize) {
        m_maxAllowedSize = maxSize;
        checkAndScaleContent();
    }

    QSize contentOriginalSize() const {
        if (m_content) {
            // Używamy sizeHint jako preferowanego oryginalnego rozmiaru
            QSize hint = m_content->sizeHint();
            if (hint.isValid()) {
                return hint;
            }
        }
        return QSize(); // Zwróć nieprawidłowy rozmiar, jeśli nie można określić
    }

    bool isScaled() const {
        return m_isScaled;
    }

    QWidget* content() const {
        return m_content;
    }

    QSize sizeHint() const override {
        QSize currentContentSize = m_contentContainer->size(); // Bierzemy aktualny rozmiar kontenera
        if (!currentContentSize.isValid() || currentContentSize.width() <= 0 || currentContentSize.height() <= 0) {
            // Jeśli kontener nie ma rozmiaru, spróbujmy sizeHint zawartości
            currentContentSize = m_content ? m_content->sizeHint() : QSize();
        }

        if (currentContentSize.isValid() && currentContentSize.width() > 0 && currentContentSize.height() > 0) {
            int extraHeight = m_infoLabel->isVisible() ? m_infoLabel->sizeHint().height() + layout()->spacing() : 0;
            QSize hint(currentContentSize.width(), currentContentSize.height() + extraHeight);
            qDebug() << "AutoScalingAttachment::sizeHint (valid content):" << hint;
            return hint;
        }

        // Zwróć domyślny minimalny rozmiar, jeśli nic innego nie działa
        QSize defaultHint = QWidget::sizeHint().isValid() ? QWidget::sizeHint() : QSize(100,50);
        qDebug() << "AutoScalingAttachment::sizeHint (default):" << defaultHint;
        return defaultHint;
    }

signals:
    void clicked();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        // Zmieniamy watched na m_contentContainer
        if (watched == m_contentContainer && event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                qDebug() << "AutoScalingAttachment: Kliknięto kontener!";
                emit clicked(); // Emituj zawsze po kliknięciu
                return true; // Zawsze przechwytuj kliknięcie na kontenerze
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void enterEvent(QEvent* event) override {
        // Pokazuj etykietę informacyjną tylko jeśli obrazek jest faktycznie przeskalowany
        if (m_isScaled) {
            m_infoLabel->show();
            // updateGeometry(); // Niekoniecznie potrzebne, chyba że etykieta zmienia rozmiar znacząco
        }
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        // Zawsze ukrywaj etykietę po zjechaniu
        m_infoLabel->hide();
        // updateGeometry(); // Niekoniecznie potrzebne
        QWidget::leaveEvent(event);
    }

private slots:
    void checkAndScaleContent() {
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
            // Domyślny maksymalny rozmiar, jeśli nie ustawiono
            maxSize = QSize(400, 300); // Przykładowy domyślny limit
            qDebug() << "AutoScalingAttachment::checkAndScaleContent - Używam domyślnego maxSize:" << maxSize;
        } else {
             qDebug() << "AutoScalingAttachment::checkAndScaleContent - Używam ustawionego maxSize:" << maxSize;
        }


        // Sprawdź czy zawartość jest zbyt duża
        bool needsScaling = originalSize.width() > maxSize.width() ||
                           originalSize.height() > maxSize.height();

        QSize targetSize;
        if (needsScaling) {
            // Oblicz skalę zachowującą proporcje
            qreal scaleX = static_cast<qreal>(maxSize.width()) / originalSize.width();
            qreal scaleY = static_cast<qreal>(maxSize.height()) / originalSize.height();
            qreal scale = qMin(scaleX, scaleY);

            // Oblicz nowy rozmiar
            targetSize = QSize(qMax(1, static_cast<int>(originalSize.width() * scale)),
                               qMax(1, static_cast<int>(originalSize.height() * scale)));

            qDebug() << "AutoScalingAttachment: Skalowanie wymagane. Z" << originalSize << "do" << targetSize;
            m_isScaled = true;
            m_infoLabel->setText("Kliknij, aby powiększyć"); // Pokaż info tylko przy skalowaniu
        } else {
            // Nie ma potrzeby skalowania, użyj oryginalnego rozmiaru
            targetSize = originalSize;
            qDebug() << "AutoScalingAttachment: Skalowanie niewymagane. Rozmiar:" << targetSize;
            m_isScaled = false;
            m_infoLabel->hide(); // Ukryj info, jeśli nie skalujemy
        }

        // Ustaw stały rozmiar dla kontenera zawartości
        m_contentContainer->setFixedSize(targetSize);

        // Upewnij się, że zawartość wewnątrz kontenera też ma odpowiedni rozmiar
        // (QLabel ze scaledContents=true powinien się dostosować)
        // m_content->setFixedSize(targetSize); // Raczej niepotrzebne

        // Aktualizuj układ i geometrię tego widgetu
        m_scaledSize = targetSize; // Zapiszmy obliczony rozmiar na później (np. dla sizeHint)
        updateGeometry(); // Ważne, aby poinformować layout nadrzędny o zmianie preferencji rozmiaru

        // Wymuś aktualizację layoutu rodzica
        if (parentWidget()) {
            QTimer::singleShot(0, parentWidget(), [parent = parentWidget()]() {
                 if (parent->layout()) parent->layout()->activate();
                 parent->updateGeometry();
            });
        }
    }

private:
    QWidget* m_content;
    QWidget* m_contentContainer;
    QLabel* m_infoLabel;
    bool m_isScaled;
    QSize m_scaledSize; // Przechowuje obliczony rozmiar po skalowaniu
    QSize m_maxAllowedSize;
};

#endif // AUTO_SCALING_ATTACHMENT_H