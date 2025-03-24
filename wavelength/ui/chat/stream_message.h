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
#include "../../files/attachment_placeholder.h"
#include "effects/disintegration_effect.h"

// Klasa reprezentująca pojedynczą wiadomość w strumieniu
class StreamMessage : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)
    Q_PROPERTY(qreal disintegrationProgress READ disintegrationProgress WRITE setDisintegrationProgress)

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

    // NOWY KOD: Dodajemy label do wyświetlania treści tekstowej
    m_contentLabel = new QLabel(this);
    m_contentLabel->setTextFormat(Qt::RichText);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setStyleSheet(
        "QLabel { color: white; background-color: transparent; font-size: 10pt; }");
    m_mainLayout->addWidget(m_contentLabel);

    // Czyścimy treść z tagów HTML i ustawiamy
    cleanupContent();

    // Jeśli to wiadomość HTML, używamy oryginalnej zawartości
    if (m_content.contains("<") && m_content.contains(">")) {
        m_contentLabel->setText(m_content);
    } else {
        // Jeśli to zwykły tekst, używamy oczyszczonej wersji
        m_contentLabel->setText(m_cleanContent);
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

    // Tworzymy placeholder załącznika
    AttachmentPlaceholder* attachmentWidget = new AttachmentPlaceholder(
        filename, type, this, false);
    attachmentWidget->setAttachmentReference(attachmentId, mimeType);

    // Usuwamy poprzedni załącznik jeśli istnieje
    if (m_attachmentWidget) {
        m_mainLayout->removeWidget(m_attachmentWidget);
        delete m_attachmentWidget;
    }

    // Ustawiamy nowy załącznik
    m_attachmentWidget = attachmentWidget;
    m_mainLayout->addWidget(m_attachmentWidget);

    // Oczyszczamy treść HTML z tagów i aktualizujemy tekst
    cleanupContent();
    m_contentLabel->setText(m_cleanContent);

    // Zwiększamy wysokość wiadomości, żeby pomieścić załącznik
    setMinimumHeight(180);
    updateLayout();
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
        // Usuwamy poprzedni efekt, jeśli istnieje
        if (graphicsEffect() && graphicsEffect() != m_disintegrationEffect) {
            delete graphicsEffect();
        }
        
        // Tworzymy nowy efekt rozpadu
        m_disintegrationEffect = new DisintegrationEffect(this);
        m_disintegrationEffect->setProgress(0.0);
        setGraphicsEffect(m_disintegrationEffect);
        
        // Animacja rozpadu
        QPropertyAnimation* disintegrationAnim = new QPropertyAnimation(this, "disintegrationProgress");
        disintegrationAnim->setDuration(1500); // 1.5 sekund
        disintegrationAnim->setStartValue(0.0);
        disintegrationAnim->setEndValue(1.0);
        disintegrationAnim->setEasingCurve(QEasingCurve::InQuad);
        
        connect(disintegrationAnim, &QPropertyAnimation::finished, this, [this]() {
            hide();
            emit hidden();
        });
        
        disintegrationAnim->start(QAbstractAnimation::DeleteWhenStopped);
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

    QSize sizeHint() const override {
        return QSize(500, 180);
    }

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

        // Tworzymy ściętą formę geometryczną (cyberpunk style)
        QPainterPath path;
        int clipSize = 20; // rozmiar ścięcia rogu

        path.moveTo(clipSize, 0);
        path.lineTo(width() - clipSize, 0);
        path.lineTo(width(), clipSize);
        path.lineTo(width(), height() - clipSize);
        path.lineTo(width() - clipSize, height());
        path.lineTo(clipSize, height());
        path.lineTo(0, height() - clipSize);
        path.lineTo(0, clipSize);
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

        // Linie dekoracyjne
        painter.setPen(QPen(borderColor.lighter(120), 1, Qt::SolidLine));
        painter.drawLine(clipSize, 30, width() - clipSize, 30);

        // Tekst nagłówka (nadawca)
        painter.setPen(QPen(textColor, 1));
        painter.setFont(QFont("Consolas", 10, QFont::Bold));
        painter.drawText(QRect(clipSize + 5, 5, width() - 2*clipSize - 10, 22),
                         Qt::AlignLeft | Qt::AlignVCenter, m_sender);

        // Tekst wiadomości - dzieli się na dwie części: przed załącznikiem i po
        if (!m_content.isEmpty()) {
            painter.setPen(QPen(Qt::white, 1));
            painter.setFont(QFont("Segoe UI", 10));
            
            // Miejsce na tekst - uwzględnia miejsce dla załącznika, jeśli istnieje
            QRect contentRect;
            if (m_attachmentWidget) {
                contentRect = QRect(clipSize + 5, 35, width() - 2*clipSize - 10, 
                                   m_attachmentWidget->y() - 45);
            } else {
                contentRect = QRect(clipSize + 5, 35, width() - 2*clipSize - 10, height() - 45);
            }
            
            painter.drawText(contentRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_cleanContent);
        }
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

        // Dostosuj wysokość, jeśli mamy załącznik
        if (m_attachmentWidget) {
            int minHeight = 120 + m_attachmentWidget->sizeHint().height();
            setMinimumHeight(minHeight);
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
};

#endif // STREAM_MESSAGE_H