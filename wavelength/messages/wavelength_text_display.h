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


// Główna klasa wyświetlająca czat
class WavelengthTextDisplay : public QWidget {
    Q_OBJECT

public:
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

    void appendMessage(const QString& formattedMessage) {
        qDebug() << "Appending message:" << formattedMessage.left(50) << "...";

        // Sprawdzamy czy wiadomość zawiera wideo
        if (formattedMessage.contains("video-placeholder")) {
            processMessageWithVideo(formattedMessage);
        } else {
            // Standardowa wiadomość tekstowa
            QLabel* messageLabel = new QLabel(formattedMessage, m_contentWidget);
            messageLabel->setTextFormat(Qt::RichText);
            messageLabel->setWordWrap(true);
            messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            messageLabel->setOpenExternalLinks(true);

            m_contentLayout->addWidget(messageLabel);
        }

        // Przewiń do najnowszej wiadomości
        QTimer::singleShot(0, this, &WavelengthTextDisplay::scrollToBottom);
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