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

    void processMessageWithImage(const QString& formattedMessage) {
        // Wyodrębnij podstawowy tekst wiadomości (bez znacznika obrazu)
        QString messageText = formattedMessage;
        int imagePos = messageText.indexOf("<div class='image-placeholder'");
        if (imagePos > 0) {
            messageText = messageText.left(imagePos);

            // Dodaj tekst wiadomości, jeśli istnieje
            if (!messageText.trimmed().isEmpty()) {
                QLabel* messageLabel = new QLabel(messageText, m_contentWidget);
                messageLabel->setTextFormat(Qt::RichText);
                messageLabel->setWordWrap(true);
                m_contentLayout->addWidget(messageLabel);
            }
        }

        // Wyodrębnij dane obrazu z wiadomości
        QRegExp imageRegex("data-mime-type='([^']*)'.*data-base64='([^']*)'.*data-filename='([^']*)'");
        imageRegex.setMinimal(true);

        if (imageRegex.indexIn(formattedMessage) != -1) {
            QString mimeType = imageRegex.cap(1);
            QString base64Data = imageRegex.cap(2);
            QString filename = imageRegex.cap(3);

            qDebug() << "Found image:" << filename << "MIME:" << mimeType;
            qDebug() << "Base64 data length:" << base64Data.length();

            // Dodaj informację o pliku obrazu
            QLabel* fileInfoLabel = new QLabel(QString("🖼️ <span style='color:#aaaaaa; font-size:9pt;'>%1</span>").arg(filename), m_contentWidget);
            fileInfoLabel->setTextFormat(Qt::RichText);
            m_contentLayout->addWidget(fileInfoLabel);

            // Dodaj wyświetlacz obrazu
            if (!base64Data.isEmpty() && base64Data.length() > 100) {
                QByteArray imageData = QByteArray::fromBase64(base64Data.toUtf8());

                if (!imageData.isEmpty()) {
                    InlineImageViewer* imageViewer = new InlineImageViewer(imageData, m_contentWidget);
                    m_contentLayout->addWidget(imageViewer);
                    qDebug() << "Added inline image viewer, data size:" << imageData.size();
                } else {
                    qDebug() << "Failed to decode base64 data";
                    QLabel* errorLabel = new QLabel("⚠️ Nie można zdekodować danych obrazu", m_contentWidget);
                    errorLabel->setStyleSheet("color: #ff5555;");
                    m_contentLayout->addWidget(errorLabel);
                }
            } else {
                qDebug() << "Invalid image data, not adding viewer";
                QLabel* errorLabel = new QLabel("⚠️ Nie można wyświetlić obrazu (uszkodzone dane)", m_contentWidget);
                errorLabel->setStyleSheet("color: #ff5555;");
                m_contentLayout->addWidget(errorLabel);
            }
        } else {
            qDebug() << "Failed to extract image data";
        }
    }

    void processMessageWithGif(const QString& formattedMessage) {
        // Wyodrębnij podstawowy tekst wiadomości (bez znacznika gif)
        QString messageText = formattedMessage;
        int gifPos = messageText.indexOf("<div class='gif-placeholder'");
        if (gifPos > 0) {
            messageText = messageText.left(gifPos);

            // Dodaj tekst wiadomości, jeśli istnieje
            if (!messageText.trimmed().isEmpty()) {
                QLabel* messageLabel = new QLabel(messageText, m_contentWidget);
                messageLabel->setTextFormat(Qt::RichText);
                messageLabel->setWordWrap(true);
                m_contentLayout->addWidget(messageLabel);
            }
        }

        // Wyodrębnij dane GIF z wiadomości
        QRegExp gifRegex("data-mime-type='([^']*)'.*data-base64='([^']*)'.*data-filename='([^']*)'");
        gifRegex.setMinimal(true);

        if (gifRegex.indexIn(formattedMessage) != -1) {
            QString mimeType = gifRegex.cap(1);
            QString base64Data = gifRegex.cap(2);
            QString filename = gifRegex.cap(3);

            qDebug() << "Found GIF:" << filename << "MIME:" << mimeType;
            qDebug() << "Base64 data length:" << base64Data.length();

            // Dodaj informację o pliku GIF
            QLabel* fileInfoLabel = new QLabel(QString("🎞️ <span style='color:#aaaaaa; font-size:9pt;'>%1</span>").arg(filename), m_contentWidget);
            fileInfoLabel->setTextFormat(Qt::RichText);
            m_contentLayout->addWidget(fileInfoLabel);

            // Dodaj odtwarzacz GIF
            if (!base64Data.isEmpty() && base64Data.length() > 100) {
                QByteArray gifData = QByteArray::fromBase64(base64Data.toUtf8());

                if (!gifData.isEmpty()) {
                    InlineGifPlayer* gifPlayer = new InlineGifPlayer(gifData, m_contentWidget);
                    m_contentLayout->addWidget(gifPlayer);
                    qDebug() << "Added inline GIF player, data size:" << gifData.size();
                } else {
                    qDebug() << "Failed to decode base64 data";
                    QLabel* errorLabel = new QLabel("⚠️ Nie można zdekodować danych GIF", m_contentWidget);
                    errorLabel->setStyleSheet("color: #ff5555;");
                    m_contentLayout->addWidget(errorLabel);
                }
            } else {
                qDebug() << "Invalid GIF data, not adding player";
                QLabel* errorLabel = new QLabel("⚠️ Nie można wyświetlić GIF (uszkodzone dane)", m_contentWidget);
                errorLabel->setStyleSheet("color: #ff5555;");
                m_contentLayout->addWidget(errorLabel);
            }
        } else {
            qDebug() << "Failed to extract GIF data";
        }
    }

    void processMessageWithAudio(const QString& formattedMessage) {
        // Wyodrębnij podstawowy tekst wiadomości (bez znacznika audio)
        QString messageText = formattedMessage;
        int audioPos = messageText.indexOf("<div class='audio-placeholder'");
        if (audioPos > 0) {
            messageText = messageText.left(audioPos);

            // Dodaj tekst wiadomości, jeśli istnieje
            if (!messageText.trimmed().isEmpty()) {
                QLabel* messageLabel = new QLabel(messageText, m_contentWidget);
                messageLabel->setTextFormat(Qt::RichText);
                messageLabel->setWordWrap(true);
                m_contentLayout->addWidget(messageLabel);
            }
        }

        // Wyodrębnij dane audio z wiadomości
        QRegExp audioRegex("data-mime-type='([^']*)'.*data-base64='([^']*)'.*data-filename='([^']*)'");
        audioRegex.setMinimal(true);

        if (audioRegex.indexIn(formattedMessage) != -1) {
            QString mimeType = audioRegex.cap(1);
            QString base64Data = audioRegex.cap(2);
            QString filename = audioRegex.cap(3);

            qDebug() << "Found audio:" << filename << "MIME:" << mimeType;
            qDebug() << "Base64 data length:" << base64Data.length();

            // Dodaj informację o pliku audio
            QLabel* fileInfoLabel = new QLabel(QString("🎵 <span style='color:#aaaaaa; font-size:9pt;'>%1</span>").arg(filename), m_contentWidget);
            fileInfoLabel->setTextFormat(Qt::RichText);
            m_contentLayout->addWidget(fileInfoLabel);

            // Dodaj odtwarzacz audio
            if (!base64Data.isEmpty() && base64Data.length() > 100) {
                QByteArray audioData = QByteArray::fromBase64(base64Data.toUtf8());

                if (!audioData.isEmpty()) {
                    InlineAudioPlayer* audioPlayer = new InlineAudioPlayer(audioData, mimeType, m_contentWidget);
                    m_contentLayout->addWidget(audioPlayer);
                    qDebug() << "Added inline audio player, data size:" << audioData.size();
                } else {
                    qDebug() << "Failed to decode base64 data";
                    QLabel* errorLabel = new QLabel("⚠️ Nie można zdekodować danych audio", m_contentWidget);
                    errorLabel->setStyleSheet("color: #ff5555;");
                    m_contentLayout->addWidget(errorLabel);
                }
            } else {
                qDebug() << "Invalid audio data, not adding player";
                QLabel* errorLabel = new QLabel("⚠️ Nie można wyświetlić audio (uszkodzone dane)", m_contentWidget);
                errorLabel->setStyleSheet("color: #ff5555;");
                m_contentLayout->addWidget(errorLabel);
            }
        } else {
            qDebug() << "Failed to extract audio data";
        }
    }

    void processMessageWithVideo(const QString& formattedMessage) {
        // Wyodrębnij podstawowy tekst wiadomości (bez znacznika wideo)
        QString messageText = formattedMessage;
        int videoPos = messageText.indexOf("<div class='video-placeholder'");
        if (videoPos > 0) {
            messageText = messageText.left(videoPos);

            // Dodaj tekst wiadomości, jeśli istnieje
            if (!messageText.trimmed().isEmpty()) {
                QLabel* messageLabel = new QLabel(messageText, m_contentWidget);
                messageLabel->setTextFormat(Qt::RichText);
                messageLabel->setWordWrap(true);
                m_contentLayout->addWidget(messageLabel);
            }
        }

        // Wyodrębnij dane wideo z wiadomości
        QRegExp videoRegex("data-mime-type='([^']*)'.*data-base64='([^']*)'.*data-filename='([^']*)'");
        videoRegex.setMinimal(true);

        if (videoRegex.indexIn(formattedMessage) != -1) {
            QString mimeType = videoRegex.cap(1);
            QString base64Data = videoRegex.cap(2);
            QString filename = videoRegex.cap(3);

            qDebug() << "Found video:" << filename << "MIME:" << mimeType;
            qDebug() << "Base64 data length:" << base64Data.length();

            // Dodaj informację o pliku wideo
            QLabel* fileInfoLabel = new QLabel(QString("📹 <span style='color:#aaaaaa; font-size:9pt;'>%1</span>").arg(filename), m_contentWidget);
            fileInfoLabel->setTextFormat(Qt::RichText);
            m_contentLayout->addWidget(fileInfoLabel);

            // Dodaj odtwarzacz wideo
            if (!base64Data.isEmpty() && base64Data.length() > 100) {
                QByteArray videoData = QByteArray::fromBase64(base64Data.toUtf8());

                if (!videoData.isEmpty()) {
                    InlineVideoPlayer* videoPlayer = new InlineVideoPlayer(videoData, mimeType, m_contentWidget);
                    m_contentLayout->addWidget(videoPlayer);
                    qDebug() << "Added inline video player, data size:" << videoData.size();
                } else {
                    qDebug() << "Failed to decode base64 data";
                    QLabel* errorLabel = new QLabel("⚠️ Nie można zdekodować danych wideo", m_contentWidget);
                    errorLabel->setStyleSheet("color: #ff5555;");
                    m_contentLayout->addWidget(errorLabel);
                }
            } else {
                qDebug() << "Invalid video data, not adding player";
                QLabel* errorLabel = new QLabel("⚠️ Nie można wyświetlić wideo (uszkodzone dane)", m_contentWidget);
                errorLabel->setStyleSheet("color: #ff5555;");
                m_contentLayout->addWidget(errorLabel);
            }
        } else {
            qDebug() << "Failed to extract video data";
        }
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