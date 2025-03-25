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
        m_content->setParent(m_contentContainer);

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
        m_content->setMouseTracking(true);
        m_content->installEventFilter(this);
        m_contentContainer->installEventFilter(this);

        // Ustaw odpowiednią politykę rozmiaru
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        // Ustaw kursor wskazujący dla skalowanych załączników
        setCursor(Qt::PointingHandCursor);

        // Sprawdź i dostosuj rozmiar po krótkim opóźnieniu
        QTimer::singleShot(100, this, &AutoScalingAttachment::checkAndScaleContent);
    }

    void setMaxAllowedSize(const QSize& maxSize) {
        m_maxAllowedSize = maxSize;
        checkAndScaleContent();
    }

    QSize contentOriginalSize() const {
        if (m_content) {
            if (m_content->sizeHint().isValid()) {
                return m_content->sizeHint();
            }
        }
        return size();
    }

    bool isScaled() const {
        return m_isScaled;
    }

    QWidget* content() const {
        return m_content;
    }

    QSize sizeHint() const override {
        if (m_content && m_isScaled && m_scaledSize.isValid()) {
            // Dodajemy przestrzeń na etykietę informacyjną
            int extraHeight = m_infoLabel->isVisible() ? m_infoLabel->height() + 4 : 0;
            return QSize(m_scaledSize.width(), m_scaledSize.height() + extraHeight);
        } else if (m_content) {
            QSize contentSize = m_content->sizeHint();
            if (contentSize.isValid()) {
                return contentSize;
            }
        }
        return QWidget::sizeHint();
    }

signals:
    void clicked();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if ((watched == m_content || watched == m_contentContainer) && 
            event->type() == QEvent::MouseButtonRelease) {
            if (m_isScaled) {
                emit clicked();
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (m_isScaled && event->button() == Qt::LeftButton) {
            event->accept();
        } else {
            QWidget::mousePressEvent(event);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (m_isScaled && event->button() == Qt::LeftButton) {
            emit clicked();
            event->accept();
        } else {
            QWidget::mouseReleaseEvent(event);
        }
    }

    void enterEvent(QEvent* event) override {
        if (m_isScaled) {
            // Pokazuj etykietę informacyjną podczas najechania
            m_infoLabel->show();
            updateGeometry();
        }
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (m_isScaled) {
            // Ukryj etykietę po zjechaniu kursorem
            m_infoLabel->hide();
            updateGeometry();
        }
        QWidget::leaveEvent(event);
    }

private slots:
    void checkAndScaleContent() {
        if (!m_content) return;

        // Pobierz oryginalny rozmiar zawartości
        QSize originalSize = m_content->sizeHint();
        if (!originalSize.isValid() || originalSize.width() <= 0 || originalSize.height() <= 0) {
            return;
        }

        // Pobierz maksymalny dostępny rozmiar
        QSize maxSize;
        if (m_maxAllowedSize.isValid()) {
            maxSize = m_maxAllowedSize;
        } else {
            // Jeśli nie ustawiono explicite, użyj dostępnego miejsca na ekranie
            // Pobieramy informacje o ekranie
            QScreen* screen = QApplication::primaryScreen();
            if (parentWidget() && parentWidget()->window()) {
                screen = parentWidget()->window()->screen();
            }
            
            QRect screenRect = screen->availableGeometry();
            
            // Ograniczamy do 80% dostępnego obszaru
            maxSize = QSize(screenRect.width() * 0.8, screenRect.height() * 0.8);
        }

        // Sprawdź czy zawartość jest zbyt duża
        bool needsScaling = originalSize.width() > maxSize.width() ||
                           originalSize.height() > maxSize.height();

        if (needsScaling) {
            // Oblicz skalę zachowującą proporcje
            double scaleX = static_cast<double>(maxSize.width()) / originalSize.width();
            double scaleY = static_cast<double>(maxSize.height()) / originalSize.height();
            double scale = qMin(scaleX, scaleY);

            // Oblicz nowy rozmiar
            m_scaledSize = QSize(originalSize.width() * scale, originalSize.height() * scale);

            // Ustaw stały rozmiar dla kontenera
            m_contentContainer->setFixedSize(m_scaledSize);
            
            qDebug() << "AutoScalingAttachment: Przeskalowano załącznik z" << originalSize 
                     << "do" << m_scaledSize;

            // Ustaw flagę skalowania
            m_isScaled = true;
            
            // Pokaż informację o możliwości kliknięcia
            m_infoLabel->setText("Kliknij, aby powiększyć");
            m_infoLabel->setCursor(Qt::PointingHandCursor);
        } else {
            // Nie ma potrzeby skalowania
            m_contentContainer->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            m_isScaled = false;
            m_infoLabel->hide();
        }

        // Aktualizuj układ
        updateGeometry();
        if (parentWidget()) {
            parentWidget()->updateGeometry();
        }
    }

private:
    QWidget* m_content;
    QWidget* m_contentContainer;
    QLabel* m_infoLabel;
    bool m_isScaled;
    QSize m_scaledSize;
    QSize m_maxAllowedSize;
};

#endif // AUTO_SCALING_ATTACHMENT_H