#ifndef STREAM_MESSAGE_H
#define STREAM_MESSAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QGraphicsEffect>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#include "cyber_text_display.h"
#include "../../files/attachment_placeholder.h"
#include "effects/disintegration_effect.h"
#include "effects/electronic_shutdown_effect.h"

// Klasa reprezentująca pojedynczą wiadomość w strumieniu
class StreamMessage : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)
    Q_PROPERTY(qreal disintegrationProgress READ disintegrationProgress WRITE setDisintegrationProgress)
    Q_PROPERTY(qreal shutdownProgress READ shutdownProgress WRITE setShutdownProgress)

public:
    enum MessageType {
        Received,
        Transmitted,
        System
    };

    StreamMessage(const QString& content, const QString& sender, MessageType type, QWidget* parent = nullptr)
    : QWidget(parent), m_content(content), m_sender(sender), m_type(type),
      m_opacity(0.0), m_glowIntensity(0.8), m_isRead(false), m_disintegrationProgress(0.0)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setMinimumWidth(400);
        setMaximumWidth(600);
        setMinimumHeight(120);

        // Podstawowy layout
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(25, 40, 25, 15);
        m_mainLayout->setSpacing(10);

        // Czyścimy treść z tagów HTML
        cleanupContent();

        // Sprawdzamy czy to wiadomość z załącznikiem
        bool hasAttachment = m_content.contains("placeholder");

        // Tworzymy CyberTextDisplay dla tekstu zwykłego
        if (!hasAttachment) {
            m_textDisplay = new CyberTextDisplay(m_cleanContent, this);
            m_mainLayout->addWidget(m_textDisplay);

            // Połączenie sygnału zakończenia animacji tekstu
            connect(m_textDisplay, &CyberTextDisplay::fullTextRevealed, this, [this]() {
                // Możemy tu dodać dodatkowe akcje po wyświetleniu całego tekstu
            });
        } else {
            // Dla załączników używamy QLabel jako w oryginalnym kodzie
            m_contentLabel = new QLabel(this);
            m_contentLabel->setTextFormat(Qt::RichText);
            m_contentLabel->setWordWrap(true);
            m_contentLabel->setStyleSheet(
                "QLabel { color: white; background-color: transparent; font-size: 10pt; }");
            m_contentLabel->setText(m_cleanContent);
            m_mainLayout->addWidget(m_contentLabel);
        }

        // Efekt przeźroczystości
        QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);
        opacity->setOpacity(0.0);
        setGraphicsEffect(opacity);

        // Inicjalizacja przycisków nawigacyjnych
        m_nextButton = new QPushButton(">", this);
        m_nextButton->setFixedSize(40, 40);
        m_nextButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(0, 200, 255, 0.3);"
            "  color: #00ffff;"
            "  border: 1px solid #00ccff;"
            "  border-radius: 20px;"
            "  font-weight: bold;"
            "  font-size: 16px;"
            "}"
            "QPushButton:hover { background-color: rgba(0, 200, 255, 0.5); }"
            "QPushButton:pressed { background-color: rgba(0, 200, 255, 0.7); }");
        m_nextButton->hide();

        m_prevButton = new QPushButton("<", this);
        m_prevButton->setFixedSize(40, 40);
        m_prevButton->setStyleSheet(m_nextButton->styleSheet());
        m_prevButton->hide();

        m_markReadButton = new QPushButton("✓", this);
        m_markReadButton->setFixedSize(40, 40);
        m_markReadButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(0, 255, 150, 0.3);"
            "  color: #00ffaa;"
            "  border: 1px solid #00ffcc;"
            "  border-radius: 20px;"
            "  font-weight: bold;"
            "  font-size: 16px;"
            "}"
            "QPushButton:hover { background-color: rgba(0, 255, 150, 0.5); }"
            "QPushButton:pressed { background-color: rgba(0, 255, 150, 0.7); }");
        m_markReadButton->hide();

        connect(m_markReadButton, &QPushButton::clicked, this, &StreamMessage::markAsRead);

        // Timer dla subtelnych animacji
        m_animationTimer = new QTimer(this);
        connect(m_animationTimer, &QTimer::timeout, this, &StreamMessage::updateAnimation);
        m_animationTimer->start(50);

        // Automatyczne ustawienie focusu na widgecie
        setFocusPolicy(Qt::StrongFocus);
    }
    
    // Właściwości do animacji
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity) {
        m_opacity = opacity;
        if (QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
            effect->setOpacity(opacity);
        }
    }

    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity) {
        m_glowIntensity = intensity;
        update();
    }
    
    qreal disintegrationProgress() const { return m_disintegrationProgress; }
    void setDisintegrationProgress(qreal progress) {
        m_disintegrationProgress = progress;
        if (m_disintegrationEffect) {
            m_disintegrationEffect->setProgress(progress);
        }
    }

    qreal shutdownProgress() const { return m_shutdownProgress; }
    void setShutdownProgress(qreal progress) {
        m_shutdownProgress = progress;
        if (m_shutdownEffect) {
            m_shutdownEffect->setProgress(progress);
        }
    }

    bool isRead() const { return m_isRead; }
    QString sender() const { return m_sender; }
    QString content() const { return m_content; }
    MessageType type() const { return m_type; }

void addAttachment(const QString& html) {
    // Sprawdzamy, jakiego typu załącznik zawiera wiadomość
    QString type, attachmentId, mimeType, filename;

    if (html.contains("video-placeholder")) {
        type = "video";
        attachmentId = extractAttribute(html, "data-attachment-id");
        mimeType = extractAttribute(html, "data-mime-type");
        filename = extractAttribute(html, "data-filename");
    } else if (html.contains("audio-placeholder")) {
        type = "audio";
        attachmentId = extractAttribute(html, "data-attachment-id");
        mimeType = extractAttribute(html, "data-mime-type");
        filename = extractAttribute(html, "data-filename");
    } else if (html.contains("gif-placeholder")) {
        type = "gif";
        attachmentId = extractAttribute(html, "data-attachment-id");
        mimeType = extractAttribute(html, "data-mime-type");
        filename = extractAttribute(html, "data-filename");
    } else if (html.contains("image-placeholder")) {
        type = "image";
        attachmentId = extractAttribute(html, "data-attachment-id");
        mimeType = extractAttribute(html, "data-mime-type");
        filename = extractAttribute(html, "data-filename");
    } else {
        return; // Brak rozpoznanego załącznika
    }

    // Usuwamy poprzedni załącznik jeśli istnieje
    if (m_attachmentWidget) {
        m_mainLayout->removeWidget(m_attachmentWidget);
        delete m_attachmentWidget;
    }

    // KLUCZOWA ZMIANA: Całkowicie usuwamy ograniczenia rozmiaru wiadomości
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);
    setMinimumWidth(0);
    setMaximumWidth(QWIDGETSIZE_MAX);

    // Tworzymy placeholder załącznika i ustawiamy referencję
    AttachmentPlaceholder* attachmentWidget = new AttachmentPlaceholder(
        filename, type, this, false);
    attachmentWidget->setAttachmentReference(attachmentId, mimeType);

    // Ustawiamy nowy załącznik
    m_attachmentWidget = attachmentWidget;
    m_mainLayout->addWidget(m_attachmentWidget);

    // Ustawiamy politykę rozmiaru dla attachmentWidget (Preferred zamiast Expanding)
    attachmentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Oczyszczamy treść HTML z tagów i aktualizujemy tekst
    cleanupContent();
    if (m_contentLabel) {
        m_contentLabel->setText(m_cleanContent);
    }

    // Bezpośrednie połączenie sygnału zmiany rozmiaru załącznika
    connect(attachmentWidget, &AttachmentPlaceholder::attachmentLoaded,
            this, [this]() {
                qDebug() << "StreamMessage: Załącznik załadowany, dostosowuję rozmiar...";

                // Ustanawiamy timer, aby dać widgetowi czas na pełne renderowanie
                QTimer::singleShot(200, this, [this]() {
                    if (m_attachmentWidget) {
                        // Określamy rozmiar widgetu załącznika
                        QSize attachSize = m_attachmentWidget->sizeHint();
                        qDebug() << "Rozmiar załącznika po załadowaniu:" << attachSize;

                        // Znajdujemy wszystkie widgety CyberAttachmentViewer w hierarchii
                        QList<CyberAttachmentViewer*> viewers =
                            m_attachmentWidget->findChildren<CyberAttachmentViewer*>();

                        if (!viewers.isEmpty()) {
                            qDebug() << "Znaleziony CyberAttachmentViewer, rozmiar:"
                                     << viewers.first()->size()
                                     << "sizeHint:" << viewers.first()->sizeHint();

                            // Wymuszamy prawidłowe obliczenie rozmiarów po całkowitym renderowaniu
                            QTimer::singleShot(100, this, [this, viewers]() {
                                // Odświeżamy rozmiar widgetu z załącznikiem
                                viewers.first()->updateGeometry();

                                // Dostosowujemy rozmiar wiadomości do załącznika
                                adjustSizeToContent();
                            });
                        } else {
                            // Jeśli nie ma CyberAttachmentViewer, po prostu dostosowujemy rozmiar
                            adjustSizeToContent();
                        }
                    }
                });
            });

    // Dostosuj początkowy rozmiar wiadomości
    QTimer::singleShot(100, this, [this]() {
        if (m_attachmentWidget) {
            QSize initialSize = m_attachmentWidget->sizeHint();
            setMinimumSize(qMax(initialSize.width() + 60, 500),
                          initialSize.height() + 100);
            qDebug() << "StreamMessage: Początkowy minimalny rozmiar:" << minimumSize();
        }
    });
}

// Nowa metoda do dostosowywania rozmiaru wiadomości
void adjustSizeToContent() {
    qDebug() << "StreamMessage::adjustSizeToContent - Dostosowywanie rozmiaru wiadomości";

    // Wymuszamy recalkulację layoutu
    m_mainLayout->invalidate();
    m_mainLayout->activate();

    // Pobieramy informacje o dostępnym miejscu na ekranie
    QScreen* screen = QApplication::primaryScreen();
    if (window()) {
        screen = window()->screen();
    }

    QRect availableGeometry = screen->availableGeometry();

    // Maksymalne dozwolone wymiary wiadomości (80% dostępnej przestrzeni)
    int maxWidth = availableGeometry.width() * 0.8;
    int maxHeight = availableGeometry.height() * 0.35;

    qDebug() << "Maksymalny dozwolony rozmiar wiadomości:" << maxWidth << "x" << maxHeight;

    // Dajemy czas na poprawne renderowanie zawartości
    QTimer::singleShot(100, this, [this, maxWidth, maxHeight]() {
        if (m_attachmentWidget) {
            // Pobieramy rozmiar załącznika
            QSize attachmentSize = m_attachmentWidget->sizeHint();
            qDebug() << "Rozmiar załącznika (sizeHint):" << attachmentSize;

            // Szukamy wszystkich CyberAttachmentViewer
            QList<CyberAttachmentViewer*> viewers = m_attachmentWidget->findChildren<CyberAttachmentViewer*>();
            if (!viewers.isEmpty()) {
                CyberAttachmentViewer* viewer = viewers.first();

                // Aktualizujemy dane widgetu
                viewer->updateGeometry();
                QSize viewerSize = viewer->sizeHint();

                qDebug() << "CyberAttachmentViewer sizeHint:" << viewerSize;

                // Obliczamy rozmiar, uwzględniając marginesy i dodatkową przestrzeń
                int newWidth = qMin(
                    qMax(viewerSize.width() +
                         m_mainLayout->contentsMargins().left() +
                         m_mainLayout->contentsMargins().right() + 50, 500),
                    maxWidth);

                int newHeight = qMin(
                    viewerSize.height() +
                    m_mainLayout->contentsMargins().top() +
                    m_mainLayout->contentsMargins().bottom() + 80,
                    maxHeight);

                // Ustawiamy nowy rozmiar minimalny
                setMinimumSize(newWidth, newHeight);

                qDebug() << "Ustawiam nowy minimalny rozmiar wiadomości:" << newWidth << "x" << newHeight;

                // Wymuszamy ponowne obliczenie rozmiaru
                updateGeometry();

                // Szukamy AutoScalingAttachment
                QList<AutoScalingAttachment*> scalers = viewer->findChildren<AutoScalingAttachment*>();
                if (!scalers.isEmpty()) {
                    // Ustawiamy maksymalny dozwolony rozmiar dla auto-skalowanej zawartości
                    QSize contentMaxSize(
                        maxWidth - m_mainLayout->contentsMargins().left() - m_mainLayout->contentsMargins().right() - 60,
                        maxHeight - m_mainLayout->contentsMargins().top() - m_mainLayout->contentsMargins().bottom() - 150
                    );

                    scalers.first()->setMaxAllowedSize(contentMaxSize);
                }

                // Wymuszamy aktualizację layoutu rodzica
                if (parentWidget() && parentWidget()->layout()) {
                    parentWidget()->layout()->invalidate();
                    parentWidget()->layout()->activate();
                    parentWidget()->updateGeometry();
                    parentWidget()->update();
                }
            }
        }

        // Aktualizujemy pozycję przycisków
        updateLayout();
        update();
    });
}

    QSize sizeHint() const override {
        QSize baseSize(500, 180);

        // Jeśli mamy załącznik, dostosuj rozmiar na jego podstawie
        if (m_attachmentWidget) {
            QSize attachmentSize = m_attachmentWidget->sizeHint();

            // Szukamy CyberAttachmentViewer w hierarchii
            QList<const CyberAttachmentViewer*> viewers =
                m_attachmentWidget->findChildren<const CyberAttachmentViewer*>();

            if (!viewers.isEmpty()) {
                // Użyj rozmiaru CyberAttachmentViewer zamiast bezpośredniego attachmentSize
                QSize viewerSize = viewers.first()->sizeHint();

                // Dodajemy marginesy i przestrzeń
                int totalHeight = viewerSize.height() +
                                m_mainLayout->contentsMargins().top() +
                                m_mainLayout->contentsMargins().bottom() + 80;

                int totalWidth = qMax(viewerSize.width() +
                                   m_mainLayout->contentsMargins().left() +
                                   m_mainLayout->contentsMargins().right() + 40, baseSize.width());

                return QSize(totalWidth, totalHeight);
            } else {
                // Standardowe zachowanie dla innych załączników
                int totalHeight = attachmentSize.height() +
                                m_mainLayout->contentsMargins().top() +
                                m_mainLayout->contentsMargins().bottom() + 70;

                int totalWidth = qMax(attachmentSize.width() +
                                   m_mainLayout->contentsMargins().left() +
                                   m_mainLayout->contentsMargins().right() + 30, baseSize.width());

                return QSize(totalWidth, totalHeight);
            }
        }

        return baseSize;
    }

    void fadeIn() {
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "opacity");
        opacityAnim->setDuration(800);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        QPropertyAnimation* glowAnim = new QPropertyAnimation(this, "glowIntensity");
        glowAnim->setDuration(1500);
        glowAnim->setStartValue(1.0);
        glowAnim->setKeyValueAt(0.4, 0.7);
        glowAnim->setKeyValueAt(0.7, 0.9);
        glowAnim->setEndValue(0.5);
        glowAnim->setEasingCurve(QEasingCurve::InOutSine);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(opacityAnim);
        group->addAnimation(glowAnim);

        // Uruchamiamy animację wpisywania tekstu po pokazaniu wiadomości
        connect(opacityAnim, &QPropertyAnimation::finished, this, [this]() {
            if (m_textDisplay) {
                m_textDisplay->startReveal();
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
        
        // Ustawiamy focus na tym widgecie
        QTimer::singleShot(100, this, QOverload<>::of(&StreamMessage::setFocus));
    }

    void fadeOut() {
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "opacity");
        opacityAnim->setDuration(500);
        opacityAnim->setStartValue(opacity());
        opacityAnim->setEndValue(0.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        connect(opacityAnim, &QPropertyAnimation::finished, this, [this]() {
            hide();
            emit hidden();
        });

        opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startDisintegrationAnimation() {
        if (graphicsEffect() && graphicsEffect() != m_shutdownEffect) {
            delete graphicsEffect();
        }

        m_shutdownEffect = new ElectronicShutdownEffect(this);
        m_shutdownEffect->setProgress(0.0);
        setGraphicsEffect(m_shutdownEffect);

        QPropertyAnimation* shutdownAnim = new QPropertyAnimation(this, "shutdownProgress");
        shutdownAnim->setDuration(1200); // Nieco szybsza animacja (1.2 sekundy)
        shutdownAnim->setStartValue(0.0);
        shutdownAnim->setEndValue(1.0);
        shutdownAnim->setEasingCurve(QEasingCurve::InQuad);

        connect(shutdownAnim, &QPropertyAnimation::finished, this, [this]() {
            hide();
            emit hidden();
        });

        shutdownAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void showNavigationButtons(bool hasPrev, bool hasNext) {
        // Pozycjonowanie przycisków nawigacyjnych
        m_nextButton->setVisible(hasNext);
        m_prevButton->setVisible(hasPrev);
        m_markReadButton->setVisible(true);

        updateLayout();

        // Frontowanie przycisków
        m_nextButton->raise();
        m_prevButton->raise();
        m_markReadButton->raise();
    }

    QPushButton* nextButton() const { return m_nextButton; }
    QPushButton* prevButton() const { return m_prevButton; }

    // QSize sizeHint() const override {
    //     return QSize(500, 180);
    // }

signals:
    void messageRead();
    void hidden();

public slots:
    void markAsRead() {
        if (!m_isRead) {
            m_isRead = true;
            
            // Animacja rozpadu zamiast przenikania
            startDisintegrationAnimation();
            
            emit messageRead();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Wybieramy kolory w stylu cyberpunk zależnie od typu wiadomości
        QColor bgColor, borderColor, glowColor, textColor;

        switch (m_type) {
            case Transmitted:
                // Neonowy niebieski dla wysyłanych
                bgColor = QColor(0, 20, 40, 180);
                borderColor = QColor(0, 200, 255);
                glowColor = QColor(0, 150, 255, 80);
                textColor = QColor(0, 220, 255);
                break;
            case Received:
                // Różowo-fioletowy dla przychodzących
                bgColor = QColor(30, 0, 30, 180);
                borderColor = QColor(220, 50, 255);
                glowColor = QColor(180, 0, 255, 80);
                textColor = QColor(240, 150, 255);
                break;
            case System:
                // Żółto-pomarańczowy dla systemowych
                bgColor = QColor(40, 25, 0, 180);
                borderColor = QColor(255, 180, 0);
                glowColor = QColor(255, 150, 0, 80);
                textColor = QColor(255, 200, 0);
                break;
        }

        // Tworzymy ściętą formę geometryczną z większą liczbą ścięć (bardziej futurystyczną)
        QPainterPath path;
        int clipSize = 20; // rozmiar ścięcia rogu
        int notchSize = 10; // rozmiar wcięcia

        // Górna krawędź z wcięciami
        path.moveTo(clipSize, 0);
        path.lineTo(width() / 3 - notchSize, 0);
        path.lineTo(width() / 3, notchSize);
        path.lineTo(width() / 3 + 2 * notchSize, 0);
        path.lineTo(width() * 2/3 - notchSize, 0);
        path.lineTo(width() * 2/3 + notchSize, notchSize);
        path.lineTo(width() - clipSize, 0);

        // Prawy górny róg i prawa krawędź z wcięciem
        path.lineTo(width(), clipSize);
        path.lineTo(width(), height() / 2 - notchSize);
        path.lineTo(width() - notchSize, height() / 2);
        path.lineTo(width(), height() / 2 + notchSize);
        path.lineTo(width(), height() - clipSize);

        // Prawy dolny róg i dolna krawędź
        path.lineTo(width() - clipSize, height());
        path.lineTo(clipSize, height());

        // Lewy dolny róg i lewa krawędź z wcięciem
        path.lineTo(0, height() - clipSize);
        path.lineTo(0, height() / 2 + notchSize);
        path.lineTo(notchSize, height() / 2);
        path.lineTo(0, height() / 2 - notchSize);
        path.lineTo(0, clipSize);

        // Zamknięcie ścieżki
        path.closeSubpath();

        // Tło z gradientem
        QLinearGradient bgGradient(0, 0, width(), height());
        bgGradient.setColorAt(0, bgColor.lighter(110));
        bgGradient.setColorAt(1, bgColor);

        painter.setPen(Qt::NoPen);
        painter.setBrush(bgGradient);
        painter.drawPath(path);

        // Poświata neonu
        if (m_glowIntensity > 0.1) {
            painter.setPen(QPen(glowColor, 6 + m_glowIntensity * 6));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }

        // Neonowe obramowanie
        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Linie dekoracyjne (poziome)
        painter.setPen(QPen(borderColor.lighter(120), 1, Qt::DashLine));
        painter.drawLine(clipSize, 30, width() - clipSize, 30);
        painter.drawLine(40, height() - 25, width() - 40, height() - 25);

        // Linie dekoracyjne (pionowe)
        painter.setPen(QPen(borderColor.lighter(120), 1, Qt::DotLine));
        painter.drawLine(40, 30, 40, height() - 25);
        painter.drawLine(width() - 40, 30, width() - 40, height() - 25);

        // Znaczniki AR w rogach
        int arMarkerSize = 15;
        painter.setPen(QPen(borderColor, 1, Qt::SolidLine));

        // Lewy górny marker
        painter.drawLine(clipSize, 10, clipSize + arMarkerSize, 10);
        painter.drawLine(clipSize, 10, clipSize, 10 + arMarkerSize);

        // Prawy górny marker
        painter.drawLine(width() - clipSize - arMarkerSize, 10, width() - clipSize, 10);
        painter.drawLine(width() - clipSize, 10, width() - clipSize, 10 + arMarkerSize);

        // Prawy dolny marker
        painter.drawLine(width() - clipSize - arMarkerSize, height() - 10, width() - clipSize, height() - 10);
        painter.drawLine(width() - clipSize, height() - 10, width() - clipSize, height() - 10 - arMarkerSize);

        // Lewy dolny marker
        painter.drawLine(clipSize, height() - 10, clipSize + arMarkerSize, height() - 10);
        painter.drawLine(clipSize, height() - 10, clipSize, height() - 10 - arMarkerSize);

        // Tekst nagłówka (nadawca)
        painter.setPen(QPen(textColor, 1));
        painter.setFont(QFont("Consolas", 10, QFont::Bold));
        painter.drawText(QRect(clipSize + 5, 5, width() - 2*clipSize - 10, 22),
                        Qt::AlignLeft | Qt::AlignVCenter, m_sender);

        // Znacznik czasu i lokalizacji w stylu AR
        QDateTime currentTime = QDateTime::currentDateTime();
        QString timeStr = currentTime.toString("HH:mm:ss");

        // Losowy identyfikator lokalizacji w stylu cyberpunk
        QString locId = QString("SEC-%1-Z%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10, 99));

        // Wskaźnik priorytetów i wiarygodności (składa się z cyfr i liter)
        int trustLevel = 60 + QRandomGenerator::global()->bounded(40); // 60-99%
        QString trustIndicator = QString("[%1%]").arg(trustLevel);

        // Dodanie znaczników w prawym górnym rogu
        painter.setFont(QFont("Consolas", 8));
        painter.setPen(QPen(textColor.lighter(120), 1));

        // Timestamp
        painter.drawText(QRect(width() - 150, 8, 120, 12),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString("TS: %1").arg(timeStr));

        // Lokalizacja
        painter.drawText(QRect(width() - 150, 20, 120, 12),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString("LOC: %1").arg(locId));

        // Wskaźnik priorytetu
        QColor priorityColor;
        QString priorityText;

        switch (m_type) {
            case Transmitted:
                priorityText = "OUT";
                priorityColor = QColor(0, 220, 255);
                break;
            case Received:
                priorityText = "IN";
                priorityColor = QColor(255, 50, 240);
                break;
            case System:
                priorityText = "SYS";
                priorityColor = QColor(255, 220, 0);
                break;
        }

        // Rysujemy ramkę wskaźnika priorytetów
        QRect priorityRect(width() - 70, height() - 40, 60, 20);
        painter.setPen(QPen(priorityColor, 1, Qt::SolidLine));
        painter.setBrush(QBrush(priorityColor.darker(600)));
        painter.drawRect(priorityRect);

        // Rysujemy tekst priorytetów
        painter.setPen(QPen(priorityColor, 1));
        painter.setFont(QFont("Consolas", 8, QFont::Bold));
        painter.drawText(priorityRect, Qt::AlignCenter, priorityText);

        // Rysujemy wskaźnik wiarygodności
        QRect trustRect(8, height() - 22, 90, 16);
        painter.setFont(QFont("Consolas", 7));
        painter.setPen(QPen(textColor, 1));

        // Kolor zależny od poziomu wiarygodności
        QColor trustColor = (trustLevel >= 90) ? QColor(0, 255, 0) :
                          (trustLevel >= 75) ? QColor(255, 255, 0) :
                          QColor(255, 100, 0);

        painter.setPen(trustColor);
        painter.drawText(trustRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QString("TRUST: %1").arg(trustIndicator));

        // Skanowanie - efekt pulsującego paska u dołu
        static const qreal scanProgress = [this]() -> qreal {
            return (QDateTime::currentMSecsSinceEpoch() % 2000) / 2000.0;
        }();

        int scanWidth = static_cast<int>(width() * scanProgress);
        QRect scanRect(0, height() - 3, scanWidth, 3);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(textColor.lighter(150)));
        painter.drawRect(scanRect);
    }

    void resizeEvent(QResizeEvent* event) override {
        updateLayout();
        QWidget::resizeEvent(event);
    }
    
    void keyPressEvent(QKeyEvent* event) override {
        // Obsługa klawiszy bezpośrednio w widgecie wiadomości
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            markAsRead();
            event->accept();
        } else if (event->key() == Qt::Key_Right && m_nextButton->isVisible()) {
            m_nextButton->click();
            event->accept();
        } else if (event->key() == Qt::Key_Left && m_prevButton->isVisible()) {
            m_prevButton->click();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
    }
    
    void focusInEvent(QFocusEvent* event) override {
        // Dodajemy subtelny efekt podświetlenia przy focusie
        m_glowIntensity += 0.2;
        update();
        QWidget::focusInEvent(event);
    }
    
    void focusOutEvent(QFocusEvent* event) override {
        // Wracamy do normalnego stanu
        m_glowIntensity -= 0.2;
        update();
        QWidget::focusOutEvent(event);
    }

private slots:
    void updateAnimation() {
        // Subtelna pulsacja poświaty
        qreal pulse = 0.05 * sin(QDateTime::currentMSecsSinceEpoch() * 0.002);
        setGlowIntensity(m_glowIntensity + pulse * (!m_isRead ? 1.0 : 0.3));
    }

private:
    // Wyciąga atrybut z HTML
    QString extractAttribute(const QString& html, const QString& attribute) {
        int attrPos = html.indexOf(attribute + "='");
        if (attrPos >= 0) {
            attrPos += attribute.length() + 2; // przesunięcie za ='
            int endPos = html.indexOf("'", attrPos);
            if (endPos >= 0) {
                return html.mid(attrPos, endPos - attrPos);
            }
        }
        return QString();
    }
    
    void processImageAttachment(const QString& html) {
        QString attachmentId = extractAttribute(html, "data-attachment-id");
        QString mimeType = extractAttribute(html, "data-mime-type");
        QString filename = extractAttribute(html, "data-filename");
        
        // Tworzymy widget zawierający placeholder załącznika
        QWidget* container = new QWidget(this);
        QVBoxLayout* containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(5, 5, 5, 5);

        // Tworzymy placeholder załącznika
        AttachmentPlaceholder* placeholderWidget = new AttachmentPlaceholder(
            filename, "image", container, false);
        placeholderWidget->setAttachmentReference(attachmentId, mimeType);

        containerLayout->addWidget(placeholderWidget);

        // Ustawiamy widget załącznika
        m_attachmentWidget = container;
        m_mainLayout->addWidget(m_attachmentWidget);

        // Zwiększamy rozmiar wiadomości
        setMinimumHeight(150 + placeholderWidget->sizeHint().height());
    }

    void processGifAttachment(const QString& html) {
        QString attachmentId = extractAttribute(html, "data-attachment-id");
        QString mimeType = extractAttribute(html, "data-mime-type");
        QString filename = extractAttribute(html, "data-filename");

        // Tworzymy widget zawierający placeholder załącznika
        QWidget* container = new QWidget(this);
        QVBoxLayout* containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(5, 5, 5, 5);

        // Tworzymy placeholder załącznika
        AttachmentPlaceholder* placeholderWidget = new AttachmentPlaceholder(
            filename, "gif", container, false);
        placeholderWidget->setAttachmentReference(attachmentId, mimeType);

        containerLayout->addWidget(placeholderWidget);

        // Ustawiamy widget załącznika
        m_attachmentWidget = container;
        m_mainLayout->addWidget(m_attachmentWidget);

        // Zwiększamy rozmiar wiadomości
        setMinimumHeight(150 + placeholderWidget->sizeHint().height());
    }

    void processAudioAttachment(const QString& html) {
        QString attachmentId = extractAttribute(html, "data-attachment-id");
        QString mimeType = extractAttribute(html, "data-mime-type");
        QString filename = extractAttribute(html, "data-filename");

        // Tworzymy widget zawierający placeholder załącznika
        QWidget* container = new QWidget(this);
        QVBoxLayout* containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(5, 5, 5, 5);

        // Tworzymy placeholder załącznika
        AttachmentPlaceholder* placeholderWidget = new AttachmentPlaceholder(
            filename, "audio", container, false);
        placeholderWidget->setAttachmentReference(attachmentId, mimeType);

        containerLayout->addWidget(placeholderWidget);

        // Ustawiamy widget załącznika
        m_attachmentWidget = container;
        m_mainLayout->addWidget(m_attachmentWidget);

        // Zwiększamy rozmiar wiadomości
        setMinimumHeight(150 + placeholderWidget->sizeHint().height());
    }

    void processVideoAttachment(const QString& html) {
        QString attachmentId = extractAttribute(html, "data-attachment-id");
        QString mimeType = extractAttribute(html, "data-mime-type");
        QString filename = extractAttribute(html, "data-filename");

        // Tworzymy widget zawierający placeholder załącznika
        QWidget* container = new QWidget(this);
        QVBoxLayout* containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(5, 5, 5, 5);

        // Tworzymy placeholder załącznika
        AttachmentPlaceholder* placeholderWidget = new AttachmentPlaceholder(
            filename, "video", container, false);
        placeholderWidget->setAttachmentReference(attachmentId, mimeType);

        containerLayout->addWidget(placeholderWidget);

        // Ustawiamy widget załącznika
        m_attachmentWidget = container;
        m_mainLayout->addWidget(m_attachmentWidget);

        // Zwiększamy rozmiar wiadomości
        setMinimumHeight(150 + placeholderWidget->sizeHint().height());
    }
    
    // Funkcja do czyszczenia zawartości wiadomości, usuwania HTML i placeholderów
    void cleanupContent() {
        m_cleanContent = m_content;

        // Usuń wszystkie znaczniki HTML
        m_cleanContent.remove(QRegExp("<[^>]*>"));

        // Znajdź i usuń tekst placeholdera załącznika
        if (m_content.contains("placeholder")) {
            int placeholderStart = m_content.indexOf("<div class='");
            if (placeholderStart >= 0) {
                int placeholderEnd = m_content.indexOf("</div>", placeholderStart);
                if (placeholderEnd > 0) {
                    QString before = m_cleanContent.left(placeholderStart);
                    QString after = m_cleanContent.mid(placeholderEnd + 6);
                    m_cleanContent = before.trimmed() + " " + after.trimmed();
                }
            }
        }

        // Usuwamy nadmiarowe whitespace
        m_cleanContent = m_cleanContent.simplified();
    }

    void updateLayout() {
        // Pozycjonowanie przycisków nawigacyjnych
        if (m_nextButton->isVisible()) {
            m_nextButton->move(width() - m_nextButton->width() - 10, height() / 2 - m_nextButton->height() / 2);
        }
        if (m_prevButton->isVisible()) {
            m_prevButton->move(10, height() / 2 - m_prevButton->height() / 2);
        }
        if (m_markReadButton->isVisible()) {
            // Zmieniona pozycja przycisku odczytu - prawy dolny róg
            m_markReadButton->move(width() - m_markReadButton->width() - 10, height() - m_markReadButton->height() - 10);
        }
    }

    QString m_content;
    QString m_cleanContent;
    QString m_sender;
    MessageType m_type;
    qreal m_opacity;
    qreal m_glowIntensity;
    qreal m_disintegrationProgress;
    bool m_isRead;

    QVBoxLayout* m_mainLayout;
    QPushButton* m_nextButton;
    QPushButton* m_prevButton;
    QPushButton* m_markReadButton;
    QTimer* m_animationTimer;
    DisintegrationEffect* m_disintegrationEffect = nullptr;
    QWidget* m_attachmentWidget = nullptr;
    QLabel* m_contentLabel = nullptr;
    CyberTextDisplay* m_textDisplay = nullptr;
    ElectronicShutdownEffect* m_shutdownEffect = nullptr;
    qreal m_shutdownProgress = 0.0;
};

#endif // STREAM_MESSAGE_H