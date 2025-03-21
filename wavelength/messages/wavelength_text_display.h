#ifndef WAVELENGTH_TEXT_DISPLAY_H
#define WAVELENGTH_TEXT_DISPLAY_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QBuffer>
#include <QByteArray>
#include <QMap>
#include <QUuid>
#include <QScrollBar>
#include <QTimer>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QMessageBox>
#include <QDebug>

#include "../files/attachment_placeholder.h"
#include "../files/video/player/inline_video_player.h"
#include "../files/audio/player/inline_audio_player.h"
#include "../files/gif/player/inline_gif_player.h"
#include "../files/image/displayer/image_viewer.h"


// Główna klasa wyświetlająca czat
class WavelengthTextDisplay : public QWidget {
    Q_OBJECT

public:
    QMap<QString, QLabel*> m_messageLabels;
    WavelengthTextDisplay(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Obszar przewijania
        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setStyleSheet("QScrollArea { background-color: #232323; border: 1px solid #3a3a3a; border-radius: 5px; }");

        // Widget zawartości
        m_contentWidget = new QWidget(m_scrollArea);
        m_contentLayout = new QVBoxLayout(m_contentWidget);
        m_contentLayout->setAlignment(Qt::AlignTop);
        m_contentLayout->setSpacing(5);
        m_contentLayout->setContentsMargins(10, 10, 10, 10);

        m_scrollArea->setWidget(m_contentWidget);
        mainLayout->addWidget(m_scrollArea);
    }

    void appendMessage(const QString& formattedMessage, const QString& messageId = QString()) {
        qDebug() << "Appending message:" << formattedMessage.left(50) << "...";

        // Sprawdzamy czy wiadomość zawiera media
        if (formattedMessage.contains("video-placeholder")) {
            processMessageWithVideo(formattedMessage);
        } else if (formattedMessage.contains("audio-placeholder")) {
            processMessageWithAudio(formattedMessage);
        } else if (formattedMessage.contains("gif-placeholder")) {
            processMessageWithGif(formattedMessage);
        } else if (formattedMessage.contains("image-placeholder")) {
            processMessageWithImage(formattedMessage);
        } else {
            // Standardowa wiadomość tekstowa
            QLabel* messageLabel = new QLabel(formattedMessage, m_contentWidget);
            messageLabel->setTextFormat(Qt::RichText);
            messageLabel->setWordWrap(true);
            messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            messageLabel->setOpenExternalLinks(true);

            // Jeśli podano ID, zapisz referencję do etykiety
            if (!messageId.isEmpty()) {
                // Usuń starą etykietę, jeśli istnieje
                if (m_messageLabels.contains(messageId)) {
                    QLabel* oldLabel = m_messageLabels[messageId];
                    m_contentLayout->removeWidget(oldLabel);
                    delete oldLabel;
                }
                m_messageLabels[messageId] = messageLabel;
            }

            m_contentLayout->addWidget(messageLabel);
        }

        // Przewiń do najnowszej wiadomości
        QTimer::singleShot(0, this, &WavelengthTextDisplay::scrollToBottom);
    }

    // Metoda do zastępowania wiadomości według ID
    void replaceMessage(const QString& messageId, const QString& newMessage) {
        if (messageId.isEmpty() || !m_messageLabels.contains(messageId)) {
            // Jeśli nie istnieje, po prostu dodaj nową wiadomość
            appendMessage(newMessage);
            return;
        }

        QLabel* label = m_messageLabels[messageId];
        label->setText(newMessage);
    }

    // Metoda do usuwania wiadomości według ID
    void removeMessage(const QString& messageId) {
        if (messageId.isEmpty() || !m_messageLabels.contains(messageId)) {
            return;
        }

        QLabel* label = m_messageLabels[messageId];
        m_contentLayout->removeWidget(label);
        m_messageLabels.remove(messageId);
        delete label;
    }

    // Funkcje przetwarzające różne typy wiadomości z załącznikami

// Przetwarzanie wiadomości z obrazkiem
void processMessageWithImage(const QString& formattedMessage, const QString& messageId = QString()) {
    // Sprawdź czy wiadomość zawiera placeholder obrazu
    int placeholderStart = formattedMessage.indexOf("<div class='image-placeholder'");
    if (placeholderStart >= 0) {
        // Znajdujemy koniec placeholdera
        int placeholderEnd = formattedMessage.indexOf("</div>", placeholderStart);
        if (placeholderEnd < 0) placeholderEnd = formattedMessage.length();

        // Wyciągamy atrybuty placeholdera
        QString imageId = extractAttribute(formattedMessage, "data-image-id");
        QString mimeType = extractAttribute(formattedMessage, "data-mime-type");
        QString attachmentId = extractAttribute(formattedMessage, "data-attachment-id");
        QString filename = extractAttribute(formattedMessage, "data-filename");

        // Tworzymy etykietę dla tekstu wiadomości (część przed placeholderem)
        QLabel* messageLabel = new QLabel(formattedMessage.left(placeholderStart));
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }

        // Tworzymy placeholder załącznika
        AttachmentPlaceholder* imagePlaceholder = new AttachmentPlaceholder(
            filename, "image", this, false);

        // Używamy referencji zamiast pełnych danych
        imagePlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(imagePlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobną etykietę
        if (placeholderEnd < formattedMessage.length()) {
            QLabel* afterLabel = new QLabel(formattedMessage.mid(placeholderEnd + 6)); // +6 dla "</div>"
            afterLabel->setWordWrap(true);
            afterLabel->setTextFormat(Qt::RichText);
            afterLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            afterLabel->setOpenExternalLinks(true);
            afterLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
            m_contentLayout->addWidget(afterLabel);
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        QLabel* messageLabel = new QLabel(formattedMessage);
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Przetwarzanie wiadomości z animacją GIF
void processMessageWithGif(const QString& formattedMessage, const QString& messageId = QString()) {
    // Sprawdź czy wiadomość zawiera placeholder GIF-a
    int placeholderStart = formattedMessage.indexOf("<div class='gif-placeholder'");
    if (placeholderStart >= 0) {
        // Znajdujemy koniec placeholdera
        int placeholderEnd = formattedMessage.indexOf("</div>", placeholderStart);
        if (placeholderEnd < 0) placeholderEnd = formattedMessage.length();

        // Wyciągamy atrybuty placeholdera
        QString gifId = extractAttribute(formattedMessage, "data-gif-id");
        QString mimeType = extractAttribute(formattedMessage, "data-mime-type");
        QString attachmentId = extractAttribute(formattedMessage, "data-attachment-id");
        QString filename = extractAttribute(formattedMessage, "data-filename");

        // Tworzymy etykietę dla tekstu wiadomości (część przed placeholderem)
        QLabel* messageLabel = new QLabel(formattedMessage.left(placeholderStart));
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }

        // Tworzymy placeholder załącznika GIF
        AttachmentPlaceholder* gifPlaceholder = new AttachmentPlaceholder(
            filename, "gif", this, false);

        // Używamy referencji zamiast pełnych danych
        gifPlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(gifPlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobną etykietę
        if (placeholderEnd < formattedMessage.length()) {
            QLabel* afterLabel = new QLabel(formattedMessage.mid(placeholderEnd + 6)); // +6 dla "</div>"
            afterLabel->setWordWrap(true);
            afterLabel->setTextFormat(Qt::RichText);
            afterLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            afterLabel->setOpenExternalLinks(true);
            afterLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
            m_contentLayout->addWidget(afterLabel);
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        QLabel* messageLabel = new QLabel(formattedMessage);
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Przetwarzanie wiadomości z plikiem audio
void processMessageWithAudio(const QString& formattedMessage, const QString& messageId = QString()) {
    // Sprawdź czy wiadomość zawiera placeholder audio
    int placeholderStart = formattedMessage.indexOf("<div class='audio-placeholder'");
    if (placeholderStart >= 0) {
        // Znajdujemy koniec placeholdera
        int placeholderEnd = formattedMessage.indexOf("</div>", placeholderStart);
        if (placeholderEnd < 0) placeholderEnd = formattedMessage.length();

        // Wyciągamy atrybuty placeholdera
        QString audioId = extractAttribute(formattedMessage, "data-audio-id");
        QString mimeType = extractAttribute(formattedMessage, "data-mime-type");
        QString attachmentId = extractAttribute(formattedMessage, "data-attachment-id");
        QString filename = extractAttribute(formattedMessage, "data-filename");

        // Tworzymy etykietę dla tekstu wiadomości (część przed placeholderem)
        QLabel* messageLabel = new QLabel(formattedMessage.left(placeholderStart));
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }

        // Tworzymy placeholder załącznika audio
        AttachmentPlaceholder* audioPlaceholder = new AttachmentPlaceholder(
            filename, "audio", this, false);

        // Używamy referencji zamiast pełnych danych
        audioPlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(audioPlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobną etykietę
        if (placeholderEnd < formattedMessage.length()) {
            QLabel* afterLabel = new QLabel(formattedMessage.mid(placeholderEnd + 6)); // +6 dla "</div>"
            afterLabel->setWordWrap(true);
            afterLabel->setTextFormat(Qt::RichText);
            afterLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            afterLabel->setOpenExternalLinks(true);
            afterLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
            m_contentLayout->addWidget(afterLabel);
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        QLabel* messageLabel = new QLabel(formattedMessage);
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Przetwarzanie wiadomości z filmem wideo
void processMessageWithVideo(const QString& formattedMessage, const QString& messageId = QString()) {
    // Sprawdź czy wiadomość zawiera placeholder wideo
    int placeholderStart = formattedMessage.indexOf("<div class='video-placeholder'");
    if (placeholderStart >= 0) {
        // Znajdujemy koniec placeholdera
        int placeholderEnd = formattedMessage.indexOf("</div>", placeholderStart);
        if (placeholderEnd < 0) placeholderEnd = formattedMessage.length();

        // Wyciągamy atrybuty placeholdera
        QString videoId = extractAttribute(formattedMessage, "data-video-id");
        QString mimeType = extractAttribute(formattedMessage, "data-mime-type");
        QString attachmentId = extractAttribute(formattedMessage, "data-attachment-id");
        QString filename = extractAttribute(formattedMessage, "data-filename");

        // Tworzymy etykietę dla tekstu wiadomości (część przed placeholderem)
        QLabel* messageLabel = new QLabel(formattedMessage.left(placeholderStart));
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }

        // Tworzymy placeholder załącznika wideo
        AttachmentPlaceholder* videoPlaceholder = new AttachmentPlaceholder(
            filename, "video", this, false);

        // Używamy referencji zamiast pełnych danych
        videoPlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(videoPlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobną etykietę
        if (placeholderEnd < formattedMessage.length()) {
            QLabel* afterLabel = new QLabel(formattedMessage.mid(placeholderEnd + 6)); // +6 dla "</div>"
            afterLabel->setWordWrap(true);
            afterLabel->setTextFormat(Qt::RichText);
            afterLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            afterLabel->setOpenExternalLinks(true);
            afterLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
            m_contentLayout->addWidget(afterLabel);
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        QLabel* messageLabel = new QLabel(formattedMessage);
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setStyleSheet("background-color: transparent; color: #e0e0e0;");
        m_contentLayout->addWidget(messageLabel);

        // Jeśli przekazano messageId, zapisujemy etykietę w mapie
        if (!messageId.isEmpty()) {
            m_messageLabels[messageId] = messageLabel;
        }
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Metoda pomocnicza do wyciągania atrybutów z HTML
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

    void clear() {
        qDebug() << "Clearing all messages";
        QLayoutItem* item;
        while ((item = m_contentLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

public slots:
    void scrollToBottom() {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum()
        );
    }

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
};

#endif // WAVELENGTH_TEXT_DISPLAY_H