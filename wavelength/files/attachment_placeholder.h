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
#include "video/player/video_player_overlay.h"

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
            try {
                // Dekodowanie base64 w osobnym wątku
                QByteArray data = QByteArray::fromBase64(m_base64Data.toUtf8());

                if (data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, "⚠️ Nie można zdekodować danych załącznika"));
                    return;
                }

                // Wywołanie odpowiedniej metody przetwarzającej w głównym wątku,
                // ale dane zostały już zdekodowane w wątku roboczym
                if (m_mimeType.startsWith("image/")) {
                    if (m_mimeType == "image/gif") {
                        QMetaObject::invokeMethod(this, "showGif",
                            Qt::QueuedConnection,
                            Q_ARG(QByteArray, data));
                    } else {
                        QMetaObject::invokeMethod(this, "showImage",
                            Qt::QueuedConnection,
                            Q_ARG(QByteArray, data));
                    }
                }
                else if (m_mimeType.startsWith("audio/")) {
                    QMetaObject::invokeMethod(this, "showAudio",
                        Qt::QueuedConnection,
                        Q_ARG(QByteArray, data));
                }
                else if (m_mimeType.startsWith("video/")) {
                    QMetaObject::invokeMethod(this, "showVideo",
                        Qt::QueuedConnection,
                        Q_ARG(QByteArray, data));
                }
            } catch (const std::exception& e) {
                QMetaObject::invokeMethod(this, "setError",
                    Qt::QueuedConnection,
                    Q_ARG(QString, QString("⚠️ Błąd przetwarzania: %1").arg(e.what())));
            }
        });
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
    // Zamiast osadzać odtwarzacz bezpośrednio, tworzymy miniaturkę z przyciskiem odtwarzania
    QWidget* videoPreview = new QWidget(m_contentContainer);
    QVBoxLayout* previewLayout = new QVBoxLayout(videoPreview);

    // Tworzymy miniaturkę (czarny prostokąt z ikoną odtwarzania)
    QLabel* thumbnailLabel = new QLabel(videoPreview);
    thumbnailLabel->setFixedSize(480, 270);
    thumbnailLabel->setAlignment(Qt::AlignCenter);
    thumbnailLabel->setStyleSheet("background-color: #000000; color: white; font-size: 48px;");
    thumbnailLabel->setText("▶");
    thumbnailLabel->setCursor(Qt::PointingHandCursor);

    // Generujemy miniaturkę z pierwszej klatki (asynchronicznie)
    generateThumbnail(data, thumbnailLabel);

    previewLayout->addWidget(thumbnailLabel);

    // Dodajemy przycisk odtwarzania pod miniaturką
    QPushButton* playButton = new QPushButton("Odtwórz wideo", videoPreview);
    playButton->setStyleSheet("QPushButton { background-color: #2c5e9e; color: white; padding: 6px; border-radius: 3px; }");
    previewLayout->addWidget(playButton);

    // Po kliknięciu na miniaturkę lub przycisk, otwórz dialog z odtwarzaczem
        auto openPlayer = [this, data]() {
            // Zachowujemy kopię danych wideo w m_videoData
            m_videoData = data;
            VideoPlayerOverlay* playerOverlay = new VideoPlayerOverlay(data, m_mimeType, nullptr);
            playerOverlay->setAttribute(Qt::WA_DeleteOnClose);
    
            // Ważne - rozłącz połączenie przed zamknięciem
            QMetaObject::Connection conn = connect(playerOverlay, &QDialog::finished,
                [this, playerOverlay]() {
                    // Rozłącz wszystkie połączenia z tym obiektem przed usunięciem
                    disconnect(playerOverlay, nullptr, this, nullptr);
                    // Zwolnij pamięć
                    m_videoData.clear();
                });

            playerOverlay->show();
        };

    connect(thumbnailLabel, &QLabel::linkActivated, this, openPlayer);
    connect(playButton, &QPushButton::clicked, this, openPlayer);

    // Dodatkowe połączenie dla kliknięcia w miniaturkę
    thumbnailLabel->installEventFilter(this);
    m_thumbnailLabel = thumbnailLabel;
    m_clickHandler = openPlayer;

    setContent(videoPreview);
    setLoading(false);
}

// Dodajemy metodę generującą miniaturkę
void generateThumbnail(const QByteArray& videoData, QLabel* thumbnailLabel) {
    QFuture<void> future = QtConcurrent::run([this, videoData, thumbnailLabel]() {
        try {
            // Tworzymy tymczasowy dekoder, który wyekstrahuje pierwszą klatkę
            auto tempDecoder = std::make_shared<VideoDecoder>(videoData, nullptr);

            // Łączymy sygnał frameReady z funkcją aktualizującą miniaturkę
            QObject::connect(tempDecoder.get(), &VideoDecoder::frameReady,
                thumbnailLabel, [thumbnailLabel, tempDecoder](const QImage& frame) {
                    // Skalujemy klatkę do rozmiaru miniaturki
                    QImage scaledFrame = frame.scaled(thumbnailLabel->width(), thumbnailLabel->height(),
                                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);

                    // Tworzymy obraz z ikoną odtwarzania na środku
                    QImage overlayImage(thumbnailLabel->width(), thumbnailLabel->height(), QImage::Format_RGB32);
                    overlayImage.fill(Qt::black);

                    // Rysujemy przeskalowaną klatkę na środku
                    QPainter painter(&overlayImage);
                    int x = (thumbnailLabel->width() - scaledFrame.width()) / 2;
                    int y = (thumbnailLabel->height() - scaledFrame.height()) / 2;
                    painter.drawImage(x, y, scaledFrame);

                    // Rysujemy półprzezroczystą ikonę odtwarzania
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(255, 255, 255, 180));
                    painter.drawEllipse(QRect(thumbnailLabel->width()/2 - 30, thumbnailLabel->height()/2 - 30, 60, 60));

                    painter.setBrush(QColor(0, 0, 0, 200));
                    QPolygon triangle;
                    triangle << QPoint(thumbnailLabel->width()/2 - 15, thumbnailLabel->height()/2 - 20);
                    triangle << QPoint(thumbnailLabel->width()/2 + 25, thumbnailLabel->height()/2);
                    triangle << QPoint(thumbnailLabel->width()/2 - 15, thumbnailLabel->height()/2 + 20);
                    painter.drawPolygon(triangle);

                    // Ustawiamy obraz jako pixmap etykiety
                    thumbnailLabel->setPixmap(QPixmap::fromImage(overlayImage));

                    // Zatrzymujemy dekoder
                    tempDecoder->stop();
                    tempDecoder->wait(500);
                    tempDecoder->releaseResources();
                },
                Qt::QueuedConnection);

            // Wyekstrahuj pierwszą klatkę
            tempDecoder->start(QThread::LowPriority);
            QThread::msleep(100); // Daj czas na inicjalizację
            tempDecoder->extractFirstFrame();

            // Poczekaj chwilę na przetworzenie pierwszej klatki
            QThread::msleep(500);

            // Zatrzymaj dekoder
            tempDecoder->stop();
            tempDecoder->wait(500);
            tempDecoder->releaseResources();

        } catch (...) {
            // Jeśli wystąpił błąd, ignoruj - miniaturka pozostanie czarna z ikoną odtwarzania
        }
    });
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
    QByteArray m_videoData; // Do przechowywania danych wideo
    QLabel* m_thumbnailLabel = nullptr;
    std::function<void()> m_clickHandler;

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_thumbnailLabel && event->type() == QEvent::MouseButtonRelease) {
            if (m_clickHandler) {
                m_clickHandler();
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }
};

#endif //ATTACHMENT_PLACEHOLDER_H
