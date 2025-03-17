//
// Created by szymo on 17.03.2025.
//

#ifndef ATTACHMENT_PLACEHOLDER_H
#define ATTACHMENT_PLACEHOLDER_H
#include <qfuture.h>
#include <QLabel>
#include <QPushButton>
#include <qtconcurrentrun.h>
#include <QVBoxLayout>
#include <QWidget>

#include "gif/player/inline_gif_player.h"
#include "image/displayer/image_viewer.h"

class AttachmentPlaceholder : public QWidget {
    Q_OBJECT

public:
    AttachmentPlaceholder(const QString& filename, const QString& type, QWidget* parent = nullptr, bool autoLoad = false)
    : QWidget(parent), m_filename(filename), m_isLoaded(false) {

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);

        // Ikona i typ załącznika
        QString icon;
        if (type == "image") icon = "🖼️";
        else if (type == "audio") icon = "🎵";
        else if (type == "video") icon = "📹";
        else if (type == "gif") icon = "🎞️";
        else icon = "📎";

        // Etykieta informacyjna
        m_infoLabel = new QLabel(QString("%1 <span style='color:#aaaaaa; font-size:9pt;'>%2</span>").arg(icon, filename), this);
        m_infoLabel->setTextFormat(Qt::RichText);
        layout->addWidget(m_infoLabel);

        // Przycisk do ładowania załącznika
        m_loadButton = new QPushButton("Załaduj załącznik", this);
        m_loadButton->setStyleSheet("QPushButton { background-color: #2c5e9e; color: white; padding: 4px; border-radius: 3px; }");
        layout->addWidget(m_loadButton);

        // Placeholder dla załącznika
        m_contentContainer = new QWidget(this);
        m_contentContainer->setVisible(false);
        m_contentLayout = new QVBoxLayout(m_contentContainer);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_contentContainer);

        // Status ładowania
        m_progressLabel = new QLabel("", this);
        m_progressLabel->setVisible(false);
        layout->addWidget(m_progressLabel);

        // Połączenia sygnałów
        connect(m_loadButton, &QPushButton::clicked, this, &AttachmentPlaceholder::onLoadButtonClicked);

        // Jeśli autoLoad jest true, automatycznie załaduj załącznik
        if (autoLoad) {
            QTimer::singleShot(100, this, &AttachmentPlaceholder::onLoadButtonClicked);
        }
    }

    void setContent(QWidget* content) {
        m_contentLayout->addWidget(content);
        m_contentContainer->setVisible(true);
        m_loadButton->setVisible(false);
        m_isLoaded = true;
    }

    void setBase64Data(const QString& base64Data, const QString& mimeType) {
        m_base64Data = base64Data;
        m_mimeType = mimeType;
    }

    void setError(const QString& errorMessage) {
        m_progressLabel->setText("<span style='color:#ff5555;'>" + errorMessage + "</span>");
        m_progressLabel->setVisible(true);
        m_loadButton->setEnabled(true);
        m_loadButton->setText("Spróbuj ponownie");
    }

    void setLoading(bool isLoading) {
        m_loadButton->setEnabled(!isLoading);
        if (isLoading) {
            m_loadButton->setText("Ładowanie...");
            m_progressLabel->setText("<span style='color:#aaaaaa;'>Trwa przetwarzanie załącznika...</span>");
            m_progressLabel->setVisible(true);
        } else {
            m_progressLabel->setVisible(false);
        }
    }

private slots:
    void onLoadButtonClicked() {
        if (m_isLoaded) return;

        setLoading(true);

        // Uruchamiamy asynchroniczne przetwarzanie w osobnym wątku
        QFuture<void> future = QtConcurrent::run([this]() {
            // Dekodowanie base64 w osobnym wątku
            QByteArray data;
            try {
                data = QByteArray::fromBase64(m_base64Data.toUtf8());

                if (data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, "⚠️ Nie można zdekodować danych załącznika"));
                    return;
                }

                // Przetwarzanie zależne od typu załącznika
                if (m_mimeType.startsWith("image/")) {
                    if (m_mimeType == "image/gif") {
                        processGif(data);
                    } else {
                        processImage(data);
                    }
                }
                else if (m_mimeType.startsWith("audio/")) {
                    processAudio(data);
                }
                else if (m_mimeType.startsWith("video/")) {
                    processVideo(data);
                }
            } catch (const std::exception& e) {
                QMetaObject::invokeMethod(this, "setError",
                    Qt::QueuedConnection,
                    Q_ARG(QString, QString("⚠️ Błąd przetwarzania: %1").arg(e.what())));
            }
        });
    }

    void processImage(const QByteArray& data) {
        // Przetwarzamy obraz w osobnym wątku
        QMetaObject::invokeMethod(this, "showImage",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, data));
    }

    void processGif(const QByteArray& data) {
        // Przetwarzamy GIF w osobnym wątku
        QMetaObject::invokeMethod(this, "showGif",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, data));
    }

    void processAudio(const QByteArray& data) {
        // Przetwarzamy audio w osobnym wątku
        QMetaObject::invokeMethod(this, "showAudio",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, data));
    }

    void processVideo(const QByteArray& data) {
        // Przetwarzamy wideo w osobnym wątku
        QMetaObject::invokeMethod(this, "showVideo",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, data));
    }

public slots:
    void showImage(const QByteArray& data) {
        InlineImageViewer* imageViewer = new InlineImageViewer(data, m_contentContainer);
        setContent(imageViewer);
        setLoading(false);
    }

    void showGif(const QByteArray& data) {
        InlineGifPlayer* gifPlayer = new InlineGifPlayer(data, m_contentContainer);
        setContent(gifPlayer);
        setLoading(false);
    }

    void showAudio(const QByteArray& data) {
        InlineAudioPlayer* audioPlayer = new InlineAudioPlayer(data, m_mimeType, m_contentContainer);
        setContent(audioPlayer);
        setLoading(false);
    }

    void showVideo(const QByteArray& data) {
        InlineVideoPlayer* videoPlayer = new InlineVideoPlayer(data, m_mimeType, m_contentContainer);
        setContent(videoPlayer);
        setLoading(false);
    }

private:
    QString m_filename;
    QString m_base64Data;
    QString m_mimeType;
    QLabel* m_infoLabel;
    QLabel* m_progressLabel;
    QPushButton* m_loadButton;
    QWidget* m_contentContainer;
    QVBoxLayout* m_contentLayout;
    bool m_isLoaded;
};

#endif //ATTACHMENT_PLACEHOLDER_H
