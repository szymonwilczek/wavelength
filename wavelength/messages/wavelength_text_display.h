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
#include "../ui/messages/message_bubble.h" // Dodajemy nagłówek nowej klasy

// Główna klasa wyświetlająca czat
class WavelengthTextDisplay : public QWidget {
    Q_OBJECT

public:
    QMap<QString, MessageBubble*> m_messageWidgets; // Zmieniamy z QLabel na MessageBubble

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
        m_contentLayout->setSpacing(12); // Zwiększamy odstęp między wiadomościami
        m_contentLayout->setContentsMargins(15, 15, 15, 15); // Zwiększamy marginesy

        m_scrollArea->setWidget(m_contentWidget);
        mainLayout->addWidget(m_scrollArea);
    }

    void appendMessage(const QString& formattedMessage, const QString& messageId = QString()) {
        qDebug() << "Appending message:" << formattedMessage.left(50) << "...";

        // Określamy typ wiadomości (własna, odebrana, systemowa)
        MessageBubble::BubbleType bubbleType = MessageBubble::ReceivedMessage;

        if (formattedMessage.contains("<span style=\"color:#60ff8a;\">[You]:</span>")) {
            bubbleType = MessageBubble::SentMessage;
        } else if (formattedMessage.contains("<span style=\"color:#ffcc00;\">")) {
            bubbleType = MessageBubble::SystemMessage;
        }

        // Sprawdzamy czy wiadomość zawiera media
        if (formattedMessage.contains("video-placeholder")) {
            processMessageWithVideo(formattedMessage, messageId, bubbleType);
        } else if (formattedMessage.contains("audio-placeholder")) {
            processMessageWithAudio(formattedMessage, messageId, bubbleType);
        } else if (formattedMessage.contains("gif-placeholder")) {
            processMessageWithGif(formattedMessage, messageId, bubbleType);
        } else if (formattedMessage.contains("image-placeholder")) {
            processMessageWithImage(formattedMessage, messageId, bubbleType);
        } else {
            // Standardowa wiadomość tekstowa w dymku
            MessageBubble* messageBubble = new MessageBubble(formattedMessage, bubbleType, m_contentWidget);

            // Jeśli podano ID, zapisz referencję do widgetu
            if (!messageId.isEmpty()) {
                // Usuń stary dymek, jeśli istnieje
                if (m_messageWidgets.contains(messageId)) {
                    MessageBubble* oldBubble = m_messageWidgets[messageId];
                    m_contentLayout->removeWidget(oldBubble);
                    delete oldBubble;
                }
                m_messageWidgets[messageId] = messageBubble;
            }

            m_contentLayout->addWidget(messageBubble);

            // Uruchom animację wejścia
            QTimer::singleShot(50, messageBubble, &MessageBubble::startEntryAnimation);
        }

        // Przewiń do najnowszej wiadomości
        QTimer::singleShot(100, this, &WavelengthTextDisplay::scrollToBottom);
    }

    // Metoda do zastępowania wiadomości według ID
    void replaceMessage(const QString& messageId, const QString& newMessage) {
        if (messageId.isEmpty() || !m_messageWidgets.contains(messageId)) {
            // Jeśli nie istnieje, po prostu dodaj nową wiadomość
            appendMessage(newMessage);
            return;
        }

        MessageBubble* bubble = m_messageWidgets[messageId];

        // Określamy typ wiadomości
        MessageBubble::BubbleType bubbleType = MessageBubble::ReceivedMessage;

        if (newMessage.contains("<span style=\"color:#60ff8a;\">[You]:</span>")) {
            bubbleType = MessageBubble::SentMessage;
        } else if (newMessage.contains("<span style=\"color:#ffcc00;\">")) {
            bubbleType = MessageBubble::SystemMessage;
        }

        // Tworzymy nowy dymek
        MessageBubble* newBubble = new MessageBubble(newMessage, bubbleType, m_contentWidget);

        // Zastępujemy stary dymek nowym
        int index = m_contentLayout->indexOf(bubble);
        m_contentLayout->removeWidget(bubble);
        delete bubble;

        m_contentLayout->insertWidget(index, newBubble);
        m_messageWidgets[messageId] = newBubble;

        // Animacja wejścia
        newBubble->startEntryAnimation();
    }

    // Metoda do usuwania wiadomości według ID
    void removeMessage(const QString& messageId) {
        if (messageId.isEmpty() || !m_messageWidgets.contains(messageId)) {
            return;
        }

        MessageBubble* bubble = m_messageWidgets[messageId];
        m_contentLayout->removeWidget(bubble);
        m_messageWidgets.remove(messageId);
        delete bubble;
    }

    // Funkcje przetwarzające różne typy wiadomości z załącznikami zostają podobne,
    // ale modyfikujemy je tak, by używały MessageBubble:

// Przetwarzanie wiadomości z obrazkiem
void processMessageWithImage(const QString& formattedMessage, const QString& messageId = QString(), MessageBubble::BubbleType bubbleType = MessageBubble::ReceivedMessage) {
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

        // Tworzymy dymek dla tekstu wiadomości (część przed placeholderem)
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage.left(placeholderStart),
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();

        // Tworzymy placeholder załącznika
        AttachmentPlaceholder* imagePlaceholder = new AttachmentPlaceholder(
            filename, "image", this, false);

        // Używamy referencji zamiast pełnych danych
        imagePlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(imagePlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobny dymek
        if (placeholderEnd < formattedMessage.length()) {
            MessageBubble* afterBubble = new MessageBubble(
                formattedMessage.mid(placeholderEnd + 6),  // +6 dla "</div>"
                bubbleType,
                m_contentWidget
            );
            m_contentLayout->addWidget(afterBubble);
            afterBubble->startEntryAnimation();
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage,
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Przetwarzanie wiadomości z animacją GIF
void processMessageWithGif(const QString& formattedMessage, const QString& messageId = QString(), MessageBubble::BubbleType bubbleType = MessageBubble::ReceivedMessage) {
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

        // Tworzymy dymek dla tekstu wiadomości (część przed placeholderem)
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage.left(placeholderStart),
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();

        // Tworzymy placeholder załącznika GIF
        AttachmentPlaceholder* gifPlaceholder = new AttachmentPlaceholder(
            filename, "gif", this, false);

        // Używamy referencji zamiast pełnych danych
        gifPlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(gifPlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobny dymek
        if (placeholderEnd < formattedMessage.length()) {
            MessageBubble* afterBubble = new MessageBubble(
                formattedMessage.mid(placeholderEnd + 6),  // +6 dla "</div>"
                bubbleType,
                m_contentWidget
            );
            m_contentLayout->addWidget(afterBubble);
            afterBubble->startEntryAnimation();
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage,
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Przetwarzanie wiadomości z plikiem audio
void processMessageWithAudio(const QString& formattedMessage, const QString& messageId = QString(), MessageBubble::BubbleType bubbleType = MessageBubble::ReceivedMessage) {
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

        // Tworzymy dymek dla tekstu wiadomości (część przed placeholderem)
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage.left(placeholderStart),
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();

        // Tworzymy placeholder załącznika audio
        AttachmentPlaceholder* audioPlaceholder = new AttachmentPlaceholder(
            filename, "audio", this, false);

        // Używamy referencji zamiast pełnych danych
        audioPlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(audioPlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobny dymek
        if (placeholderEnd < formattedMessage.length()) {
            MessageBubble* afterBubble = new MessageBubble(
                formattedMessage.mid(placeholderEnd + 6),  // +6 dla "</div>"
                bubbleType,
                m_contentWidget
            );
            m_contentLayout->addWidget(afterBubble);
            afterBubble->startEntryAnimation();
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage,
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();
    }

    // Przewijamy obszar przewijania do dołu
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

// Przetwarzanie wiadomości z filmem wideo
void processMessageWithVideo(const QString& formattedMessage, const QString& messageId = QString(), MessageBubble::BubbleType bubbleType = MessageBubble::ReceivedMessage) {
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

        // Tworzymy dymek dla tekstu wiadomości (część przed placeholderem)
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage.left(placeholderStart),
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();

        // Tworzymy placeholder załącznika wideo
        AttachmentPlaceholder* videoPlaceholder = new AttachmentPlaceholder(
            filename, "video", this, false);

        // Używamy referencji zamiast pełnych danych
        videoPlaceholder->setAttachmentReference(attachmentId, mimeType);
        m_contentLayout->addWidget(videoPlaceholder);

        // Jeśli jest tekst po placeholderze, dodajemy go jako osobny dymek
        if (placeholderEnd < formattedMessage.length()) {
            MessageBubble* afterBubble = new MessageBubble(
                formattedMessage.mid(placeholderEnd + 6),  // +6 dla "</div>"
                bubbleType,
                m_contentWidget
            );
            m_contentLayout->addWidget(afterBubble);
            afterBubble->startEntryAnimation();
        }
    } else {
        // Standardowe wyświetlanie wiadomości bez załącznika
        MessageBubble* messageBubble = new MessageBubble(
            formattedMessage,
            bubbleType,
            m_contentWidget
        );

        // Jeśli przekazano messageId, zapisujemy dymek w mapie
        if (!messageId.isEmpty()) {
            m_messageWidgets[messageId] = messageBubble;
        }

        m_contentLayout->addWidget(messageBubble);
        messageBubble->startEntryAnimation();
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
        m_messageWidgets.clear();
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