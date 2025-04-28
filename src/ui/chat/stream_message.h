#ifndef STREAM_MESSAGE_H
#define STREAM_MESSAGE_H

#include <QWidget>
#include <QSvgRenderer>
#include <QBitmap>
#include <qtimeline.h>
#include <utility>

#include "../chat/effects/cyber_long_text_display.h"
#include "../chat/effects/cyber_text_display.h"
#include "../../chat/files/attachments/attachment_placeholder.h"
#include "effects/disintegration_effect.h"
#include "effects/electronic_shutdown_effect.h"



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
    QPushButton* m_markReadButton;


    static QIcon createColoredSvgIcon(const QString& svgPath, const QColor& color, const QSize& size) {
        QSvgRenderer renderer;
        if (!renderer.load(svgPath)) {
            qWarning() << "Nie można załadować SVG:" << svgPath;
            return QIcon(); // Zwróć pustą ikonę w razie błędu
        }

        // Utwórz pixmapę o żądanym rozmiarze z przezroczystym tłem
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        // Narysuj SVG na pixmapie
        QPainter painter(&pixmap);
        renderer.render(&painter);
        painter.end(); // Zakończ malowanie na oryginalnej pixmapie

        // Utwórz maskę z kanału alfa oryginalnej pixmapy
        QBitmap mask = pixmap.createMaskFromColor(Qt::transparent);

        // Utwórz nową pixmapę wypełnioną docelowym kolorem
        QPixmap coloredPixmap(size);
        coloredPixmap.fill(color);

        // Zastosuj maskę do kolorowej pixmapy
        coloredPixmap.setMask(mask);

        return QIcon(coloredPixmap);
    }

    StreamMessage(QString  content, QString  sender, MessageType type, QString messageId = QString(), QWidget* parent = nullptr);

    void updateContent(const QString& newContent);

    // Właściwości do animacji
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity);

    qreal disintegrationProgress() const { return m_disintegrationProgress; }
    void setDisintegrationProgress(qreal progress);

    qreal shutdownProgress() const { return m_shutdownProgress; }
    void setShutdownProgress(qreal progress);

    bool isRead() const { return m_isRead; }
    QString sender() const { return m_sender; }
    QString content() const { return m_content; }
    MessageType type() const { return m_type; }
    QString messageId() const {return m_messageId; }

void addAttachment(const QString& html);

    // Nowa metoda do dostosowywania rozmiaru wiadomości
void adjustSizeToContent();

    QSize sizeHint() const override;

    // Zoptymalizowane metody fadeIn i fadeOut

void fadeIn();

    void fadeOut();

    void updateScrollAreaMaxHeight();

    void startDisintegrationAnimation();

    void showNavigationButtons(bool hasPrev, bool hasNext);

    QPushButton* nextButton() const { return m_nextButton; }
    QPushButton* prevButton() const { return m_prevButton; }


signals:
    void messageRead();
    void hidden();

public slots:
    void markAsRead();

protected:

    void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void updateAnimation();

    void adjustScrollAreaStyle();

private:
    // Wyciąga atrybut z HTML
    static QString extractAttribute(const QString& html, const QString& attribute);

    void startLongMessageClosingAnimation();

    void processImageAttachment(const QString& html);

    void processGifAttachment(const QString& html);

    void processAudioAttachment(const QString& html);

    void processVideoAttachment(const QString& html);

    // Funkcja do czyszczenia zawartości wiadomości, usuwania HTML i placeholderów
    void cleanupContent();

    void updateLayout() const;

    QString m_messageId;
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
    QTimer* m_animationTimer;
    DisintegrationEffect* m_disintegrationEffect = nullptr;
    QWidget* m_attachmentWidget = nullptr;
    QLabel* m_contentLabel = nullptr;
    CyberTextDisplay* m_textDisplay = nullptr;
    ElectronicShutdownEffect* m_shutdownEffect = nullptr;
    qreal m_shutdownProgress = 0.0;
    QScrollArea* m_scrollArea = nullptr;
    CyberLongTextDisplay* m_longTextDisplay = nullptr;
};

#endif // STREAM_MESSAGE_H