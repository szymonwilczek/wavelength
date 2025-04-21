#ifndef INLINE_GIF_PLAYER_H
#define INLINE_GIF_PLAYER_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <memory>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug> // Dodano dla logowania

#include "../../audio/player/inline_audio_player.h"
#include "../../video/player/inline_video_player.h"
#include "../decoder/gif_decoder.h"

// Klasa odtwarzacza GIF zintegrowanego z czatem
class InlineGifPlayer : public QFrame {
    Q_OBJECT

public:
    InlineGifPlayer(const QByteArray& gifData, QWidget* parent = nullptr)
        : QFrame(parent), m_gifData(gifData) {

        // Usunięto setFrameStyle i setStyleSheet
        // Usunięto setMaximumSize

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Obszar wyświetlania GIF-a
        m_gifLabel = new QLabel(this);
        m_gifLabel->setAlignment(Qt::AlignCenter);
        m_gifLabel->setStyleSheet("background-color: transparent; color: #ffffff;"); // Przezroczyste tło
        m_gifLabel->setText("Ładowanie GIF...");
        m_gifLabel->setScaledContents(true); // Kluczowa zmiana: Włącz skalowanie zawartości QLabel
        layout->addWidget(m_gifLabel);

        // Dekoder GIF
        m_decoder = std::make_shared<GifDecoder>(gifData, this);

        // Połącz sygnały
        connect(m_decoder.get(), &GifDecoder::frameReady, this, &InlineGifPlayer::updateFrame, Qt::QueuedConnection);
        connect(m_decoder.get(), &GifDecoder::error, this, &InlineGifPlayer::handleError);
        connect(m_decoder.get(), &GifDecoder::gifInfo, this, &InlineGifPlayer::handleGifInfo);
        connect(m_decoder.get(), &GifDecoder::playbackFinished, this, &InlineGifPlayer::handlePlaybackFinished);
        connect(m_decoder.get(), &GifDecoder::positionChanged, this, &InlineGifPlayer::updatePosition);

        // Inicjalizuj odtwarzacz w osobnym wątku
        QTimer::singleShot(100, this, [this]() {
            if (m_decoder) m_decoder->start();
        });

        // Obsługa zamykania aplikacji
        connect(qApp, &QApplication::aboutToQuit, this, &InlineGifPlayer::releaseResources); // Uproszczono lambdę

        // Usunięto instalację filtra zdarzeń i kursor
    }

    // Usunięto getActivePlayer i logikę s_activePlayer

    ~InlineGifPlayer() {
        releaseResources();
    }

    void releaseResources() {
        if (m_decoder) {
            m_decoder->stop();
            m_decoder->wait(500);
            m_decoder->releaseResources();
            m_decoder.reset(); // Zwalniamy wskaźnik
        }
        // Usunięto logikę s_activePlayer
        // Usunięto m_isActive
    }

    // Zwraca oryginalny rozmiar GIF-a jako wskazówkę
    QSize sizeHint() const override {
        if (m_gifWidth > 0 && m_gifHeight > 0) {
            return QSize(m_gifWidth, m_gifHeight);
        }
        return QFrame::sizeHint(); // Zwróć domyślny, jeśli rozmiar nie jest znany
    }

    // Usunięto activate
    // Usunięto deactivate
    // Usunięto eventFilter
    // Usunięto enterEvent

private slots:
    // Usunięto togglePlayback

    void updateFrame(const QImage& frame) {
        if (frame.isNull()) return;

        // Jeśli to pierwsza klatka, zapisz ją jako miniaturkę (może być przydatne)
        if (m_thumbnailFrame.isNull()) {
            m_thumbnailFrame = frame.copy();
        }

        // Ustaw pixmapę na QLabel - skalowanie załatwi setScaledContents(true)
        m_gifLabel->setPixmap(QPixmap::fromImage(frame));

        // Nie ustawiamy już setFixedSize
        // setFixedSize(frame.width(), frame.height()); // Usunięto
        // m_gifLabel->setFixedSize(frame.size()); // Usunięto

        // Jeśli to pierwsza klatka, zaktualizuj geometrię
        if (m_gifLabel->size() != frame.size()) {
             updateGeometry(); // Informujemy layout o potencjalnej zmianie rozmiaru
        }
    }

    // Usunięto displayScaledImage

    void updatePosition(double position) {
        m_currentPosition = position;
    }

    void handleError(const QString& message) {
        qDebug() << "GIF decoder error:" << message;
        m_gifLabel->setText("⚠️ " + message);
        setMinimumSize(100, 50); // Minimalny rozmiar dla błędu
        updateGeometry();
    }

    void handleGifInfo(int width, int height, double duration, double frameRate, int numStreams) {
        m_gifWidth = width;
        m_gifHeight = height;
        m_gifDuration = duration;
        m_frameRate = frameRate;

        qDebug() << "GIF info - szerokość:" << width << "wysokość:" << height
                 << "czas trwania:" << duration << "s, FPS:" << frameRate;

        // Zaktualizuj geometrię po otrzymaniu informacji o rozmiarze
        updateGeometry();
        emit gifLoaded(); // Sygnalizujemy załadowanie informacji o GIFie

        // Usunięto pobieranie pierwszej klatki jako miniaturki tutaj
    }

    void handlePlaybackFinished() {
        // GIF-y zazwyczaj automatycznie się zapętlają w dekoderze
    }

signals:
    void gifLoaded(); // Nowy sygnał

private:
    QLabel* m_gifLabel;
    std::shared_ptr<GifDecoder> m_decoder;

    QByteArray m_gifData;

    int m_gifWidth = 0;
    int m_gifHeight = 0;
    double m_gifDuration = 0;
    double m_frameRate = 0;
    double m_currentPosition = 0;
    // Usunięto m_isActive
    QImage m_thumbnailFrame;

    // Usunięto s_activePlayer
};

#endif //INLINE_GIF_PLAYER_H