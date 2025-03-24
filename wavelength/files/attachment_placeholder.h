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

#include "attachment_queue_manager.h"
#include "../messages/formatter/message_formatter.h"
#include "../ui/cyber_attachment_viewer.h"
#include "gif/player/inline_gif_player.h"
#include "image/displayer/image_viewer.h"
#include "video/player/video_player_overlay.h"

class AttachmentPlaceholder : public QWidget {
    Q_OBJECT

public:
     AttachmentPlaceholder(const QString& filename, const QString& type, QWidget* parent = nullptr, bool autoLoad = true)
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

        m_infoLabel = new QLabel(QString("<span style='color:#00ccff;'>%1</span> <span style='color:#aaaaaa; font-size:9pt;'>%2</span>").arg(icon, filename), this);
        m_infoLabel->setTextFormat(Qt::RichText);
        layout->addWidget(m_infoLabel);

        // Przycisk do ładowania załącznika - teraz ukryty domyślnie
        m_loadButton = new QPushButton("PONÓW DEKODOWANIE", this);
        m_loadButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #002b3d;"
            "  color: #00ffff;"
            "  border: 1px solid #00aaff;"
            "  border-radius: 2px;"
            "  padding: 6px;"
            "  font-family: 'Consolas', monospace;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #003e59;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #005580;"
            "}"
        );
        m_loadButton->setVisible(false); // Domyślnie ukryty
        layout->addWidget(m_loadButton);

        // Placeholder dla załącznika
        m_contentContainer = new QWidget(this);
        m_contentContainer->setVisible(false);
        m_contentLayout = new QVBoxLayout(m_contentContainer);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_contentContainer);

        // Status ładowania
        m_progressLabel = new QLabel("<span style='color:#00ccff;'>Inicjowanie sekwencji dekodowania...</span>", this);
        layout->addWidget(m_progressLabel);

        // Połączenia sygnałów
        connect(m_loadButton, &QPushButton::clicked, this, &AttachmentPlaceholder::onLoadButtonClicked);

        // Automatycznie rozpocznij ładowanie po krótkim opóźnieniu
        QTimer::singleShot(300, this, &AttachmentPlaceholder::onLoadButtonClicked);
    }

    void setContent(QWidget* content) {
         m_contentLayout->addWidget(content);
         m_contentContainer->setVisible(true);
         m_loadButton->setVisible(false);
         m_isLoaded = true;
         notifyLoaded(); // Dodajemy wywołanie po ustawieniu zawartości
     }

    void setAttachmentReference(const QString& attachmentId, const QString& mimeType) {
        m_attachmentId = attachmentId;
        m_mimeType = mimeType;
        m_hasReference = true;
    }

    void setBase64Data(const QString& base64Data, const QString& mimeType) {
        m_base64Data = base64Data;
        m_mimeType = mimeType;
    }

    void setLoading(bool loading) {
        if (loading) {
            m_loadButton->setEnabled(false);
            m_loadButton->setText("DEKODOWANIE W TOKU...");
            m_progressLabel->setText("<span style='color:#00ccff;'>Pozyskiwanie zaszyfrowanych danych...</span>");
            m_progressLabel->setVisible(true);
        } else {
            m_loadButton->setEnabled(true);
            m_loadButton->setText("INICJUJ DEKODOWANIE");
            m_progressLabel->setVisible(false);
        }
    }

    void setError(const QString& errorMsg) {
         m_loadButton->setEnabled(true);
         m_loadButton->setText("PONÓW DEKODOWANIE");
         m_loadButton->setVisible(true); // Pokaż przycisk przy błędzie
         m_progressLabel->setText("<span style='color:#ff3333;'>⚠️ BŁĄD: " + errorMsg + "</span>");
         m_progressLabel->setVisible(true);
         m_isLoaded = false;
     }

    signals:
    void attachmentLoaded();

private slots:
    void onLoadButtonClicked() {
        if (m_isLoaded) return;

        setLoading(true);

        // Sprawdź czy mamy referencję czy pełne dane
        if (m_hasReference) {
            // Pobieramy dane z magazynu w wątku roboczym
            AttachmentQueueManager::getInstance()->addTask([this]() {
                QString base64Data = AttachmentDataStore::getInstance()->getAttachmentData(m_attachmentId);

                if (base64Data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, "⚠️ Nie można odnaleźć danych załącznika"));
                    return;
                }

                // Dekodujemy dane i kontynuujemy jak wcześniej
                QByteArray data = QByteArray::fromBase64(base64Data.toUtf8());

                if (data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, "⚠️ Nie można zdekodować danych załącznika"));
                    return;
                }

                // Wybieramy odpowiednią metodę wyświetlania na podstawie MIME type
                if (m_mimeType.startsWith("image/")) {
                    if (m_mimeType == "image/gif") {
                        QMetaObject::invokeMethod(this, "showCyberGif",
                            Qt::QueuedConnection,
                            Q_ARG(QByteArray, data));
                    } else {
                        QMetaObject::invokeMethod(this, "showCyberImage",
                            Qt::QueuedConnection,
                            Q_ARG(QByteArray, data));
                    }
                }
                else if (m_mimeType.startsWith("audio/")) {
                    QMetaObject::invokeMethod(this, "showCyberAudio",
                        Qt::QueuedConnection,
                        Q_ARG(QByteArray, data));
                }
                else if (m_mimeType.startsWith("video/")) {
                    QMetaObject::invokeMethod(this, "showCyberVideo",
                        Qt::QueuedConnection,
                        Q_ARG(QByteArray, data));
                }
            });
        } else {
            // Stare zachowanie dla pełnych danych (dla kompatybilności)
            AttachmentQueueManager::getInstance()->addTask([this]() {
                try {
                    QByteArray data = QByteArray::fromBase64(m_base64Data.toUtf8());

                    if (data.isEmpty()) {
                        QMetaObject::invokeMethod(this, "setError",
                            Qt::QueuedConnection,
                            Q_ARG(QString, "⚠️ Nie można zdekodować danych załącznika"));
                        return;
                    }

                    // Dalsze przetwarzanie jak wcześniej
                    // ...
                } catch (const std::exception& e) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, QString("⚠️ Błąd przetwarzania: %1").arg(e.what())));
                }
            });
        }
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

    void showCyberImage(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z obrazem
    InlineImageViewer* imageViewer = new InlineImageViewer(data, viewer);

    // Opakowujemy w AutoScalingAttachment
    viewer->setContent(imageViewer);

    // Podłączamy sygnał zakończenia
    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        m_loadButton->setText("PONÓW DEKODOWANIE");
        m_loadButton->setVisible(true);
        m_contentContainer->setVisible(false);
        emit attachmentLoaded(); // Emitujemy sygnał również przy zamknięciu
    });

    setContent(viewer);
    setLoading(false);
}

void showCyberGif(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z gifem
    InlineGifPlayer* gifPlayer = new InlineGifPlayer(data, viewer);

    // Opakowujemy w AutoScalingAttachment

    viewer->setContent(gifPlayer);

    // Podłączamy sygnał zakończenia
    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        m_loadButton->setText("Załaduj ponownie");
        m_loadButton->setVisible(true);
        m_contentContainer->setVisible(false);
    });

    setContent(viewer);
    setLoading(false);
}

void showCyberAudio(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z audio
    InlineAudioPlayer* audioPlayer = new InlineAudioPlayer(data, m_mimeType, viewer);

    // Opakowujemy w AutoScalingAttachment

    viewer->setContent(audioPlayer);

    // Podłączamy sygnał zakończenia
    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        m_loadButton->setText("Załaduj ponownie");
        m_loadButton->setVisible(true);
        m_contentContainer->setVisible(false);
    });

    setContent(viewer);
    setLoading(false);
}

void showCyberVideo(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z miniaturką wideo
    QWidget* videoPreview = new QWidget(viewer);
    QVBoxLayout* previewLayout = new QVBoxLayout(videoPreview);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    // Tworzymy miniaturkę
    QLabel* thumbnailLabel = new QLabel(videoPreview);
    thumbnailLabel->setFixedSize(480, 270);
    thumbnailLabel->setAlignment(Qt::AlignCenter);
    thumbnailLabel->setStyleSheet("background-color: #000000; color: white; font-size: 48px;");
    thumbnailLabel->setText("▶");
    thumbnailLabel->setCursor(Qt::PointingHandCursor);

    // Generujemy miniaturkę z pierwszej klatki
    generateThumbnail(data, thumbnailLabel);

    previewLayout->addWidget(thumbnailLabel);

    // Dodajemy przycisk odtwarzania pod miniaturką
    QPushButton* playButton = new QPushButton("ODTWÓRZ WIDEO", videoPreview);
    playButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #002b3d; "
        "  color: #00ffff; "
        "  border: 1px solid #00aaff; "
        "  padding: 6px; "
        "  border-radius: 2px; "
        "  font-family: 'Consolas', monospace; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #003e59; }"
    );
    previewLayout->addWidget(playButton);

    // Opakowujemy w AutoScalingAttachment

    viewer->setContent(videoPreview);

    // Po kliknięciu miniaturki lub przycisku, otwórz dialog z odtwarzaczem
    auto openPlayer = [this, data]() {
        m_videoData = data;
        VideoPlayerOverlay* playerOverlay = new VideoPlayerOverlay(data, m_mimeType, nullptr);

        // Dodajemy cyberpunkowy styl do overlay'a
        playerOverlay->setStyleSheet(
            "QDialog { "
            "  background-color: #001520; "
            "  border: 2px solid #00aaff; "
            "}"
        );

        playerOverlay->setAttribute(Qt::WA_DeleteOnClose);

        // Rozłącz połączenia przed zamknięciem
        QMetaObject::Connection conn = connect(playerOverlay, &QDialog::finished,
            [this, playerOverlay]() {
                disconnect(playerOverlay, nullptr, this, nullptr);
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

    // Podłączamy sygnał zakończenia
    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        m_loadButton->setText("Załaduj ponownie");
        m_loadButton->setVisible(true);
        m_contentContainer->setVisible(false);
    });

    setContent(viewer);
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
    // Dodajemy zadanie do menedżera kolejki
    AttachmentQueueManager::getInstance()->addTask([this, videoData, thumbnailLabel]() {
        try {
            // Tworzymy tymczasowy dekoder wideo tylko dla miniatury
            auto tempDecoder = std::make_shared<VideoDecoder>(videoData, nullptr);

            // Łączymy sygnał z dekoderem
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

                    // Dodajemy ikonę odtwarzania
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(255, 255, 255, 180));
                    painter.drawEllipse(QRect(thumbnailLabel->width()/2 - 30,
                                             thumbnailLabel->height()/2 - 30, 60, 60));

                    painter.setBrush(QColor(0, 0, 0, 200));
                    QPolygon triangle;
                    triangle << QPoint(thumbnailLabel->width()/2 - 15, thumbnailLabel->height()/2 - 20);
                    triangle << QPoint(thumbnailLabel->width()/2 + 25, thumbnailLabel->height()/2);
                    triangle << QPoint(thumbnailLabel->width()/2 - 15, thumbnailLabel->height()/2 + 20);
                    painter.drawPolygon(triangle);

                    // Aktualizacja UI w głównym wątku
                    QMetaObject::invokeMethod(thumbnailLabel, "setPixmap",
                        Qt::QueuedConnection,
                        Q_ARG(QPixmap, QPixmap::fromImage(overlayImage)));

                    // Zatrzymujemy dekoder
                    tempDecoder->stop();
                },
                Qt::QueuedConnection);

            // Wyekstrahuj pierwszą klatkę
            tempDecoder->extractFirstFrame();

            // Dajemy czas na przetworzenie klatki
            QThread::msleep(500);

            // Zatrzymujemy dekoder
            tempDecoder->stop();
            tempDecoder->wait(300);

        } catch (...) {
            // W przypadku błędu pozostawiamy domyślną czarną miniaturkę
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
    QString m_attachmentId;
    bool m_hasReference = false;

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_thumbnailLabel && event->type() == QEvent::MouseButtonRelease) {
            if (m_clickHandler) {
                m_clickHandler();
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void notifyLoaded() {
        // Emitujemy sygnał o załadowaniu załącznika bez dodatkowych operacji resize
        emit attachmentLoaded();
    }
};

#endif //ATTACHMENT_PLACEHOLDER_H
