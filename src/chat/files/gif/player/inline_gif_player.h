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
        connect(m_decoder.get(), &GifDecoder::firstFrameReady, this, &InlineGifPlayer::displayThumbnail, Qt::QueuedConnection);
        connect(m_decoder.get(), &GifDecoder::frameReady, this, &InlineGifPlayer::updateFrame, Qt::QueuedConnection);
        connect(m_decoder.get(), &GifDecoder::error, this, &InlineGifPlayer::handleError, Qt::QueuedConnection); // Dodano QueuedConnection dla bezpieczeństwa
        connect(m_decoder.get(), &GifDecoder::gifInfo, this, &InlineGifPlayer::handleGifInfo, Qt::QueuedConnection);

        // Inicjalizuj dekoder (wyekstrahuje pierwszą klatkę)
        // Wywołanie w osobnym wątku lub przez QTimer, aby nie blokować UI
        QTimer::singleShot(0, this, [this]() {
            if (m_decoder) {
                if (!m_decoder->initialize()) {
                     qDebug() << "InlineGifPlayer: Decoder initialization failed.";
                     // Można tu obsłużyć błąd inicjalizacji
                } else {
                     qDebug() << "InlineGifPlayer: Decoder initialized successfully.";
                }
            }
        });

        // Obsługa zamykania aplikacji
        connect(qApp, &QApplication::aboutToQuit, this, &InlineGifPlayer::releaseResources); // Uproszczono lambdę

        setMouseTracking(true);
        m_gifLabel->setMouseTracking(true);
    }

    // Usunięto getActivePlayer i logikę s_activePlayer

    ~InlineGifPlayer() {
        releaseResources();
    }

    void releaseResources() {
        if (m_decoder) {
            m_decoder->stop();
            if (m_decoder->isRunning()) { // Czekaj tylko jeśli wątek działał
                m_decoder->wait(500);
            }
            // releaseResources w dekoderze jest wywoływane w jego destruktorze,
            // ale można też wywołać jawnie, jeśli jest potrzeba
            // m_decoder->releaseResources();
            m_decoder.reset();
            m_isPlaying = false;
            qDebug() << "InlineGifPlayer: Resources released.";
        }
    }

    // Zwraca oryginalny rozmiar GIF-a jako wskazówkę
    QSize sizeHint() const override {
        if (m_gifWidth > 0 && m_gifHeight > 0) {
            return QSize(m_gifWidth, m_gifHeight);
        }
        return QFrame::sizeHint(); // Zwróć domyślny, jeśli rozmiar nie jest znany
    }

    void startPlayback() {
        qDebug() << "InlineGifPlayer: startPlayback called.";
        if (m_decoder && !m_isPlaying) {
            m_isPlaying = true;
            m_decoder->resume();
        }
    }

    // Metoda do jawnego zatrzymania/pauzowania odtwarzania (np. dla dialogu)
    void stopPlayback() {
        qDebug() << "InlineGifPlayer: stopPlayback called.";
        if (m_decoder && m_isPlaying) {
            m_isPlaying = false;
            m_decoder->pause();
        }
    }

protected:
    // Przechwytywanie zdarzeń najechania i opuszczenia myszy
    void enterEvent(QEvent *event) override {
        qDebug() << "InlineGifPlayer: Mouse entered.";
        startPlayback(); // Rozpocznij odtwarzanie przy najechaniu
        QFrame::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        qDebug() << "InlineGifPlayer: Mouse left.";
        stopPlayback(); // Zatrzymaj odtwarzanie przy opuszczeniu
        QFrame::leaveEvent(event);
    }

private slots:
    void displayThumbnail(const QImage& frame) {
        if (!frame.isNull()) {
            qDebug() << "InlineGifPlayer: Displaying thumbnail.";
            m_thumbnailPixmap = QPixmap::fromImage(frame); // Zapisz miniaturkę
            m_gifLabel->setPixmap(m_thumbnailPixmap.scaled(
                m_gifLabel->size(), // Skaluj do aktualnego rozmiaru labelki
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            ));
            // Nie aktualizuj geometrii tutaj, poczekaj na gifInfo
        } else {
            qDebug() << "InlineGifPlayer: Received null thumbnail.";
            m_gifLabel->setText("⚠️ Błąd ładowania miniatury");
        }
    }

    void updateFrame(const QImage& frame) {
        // Ta metoda jest teraz wywoływana tylko gdy dekoder aktywnie dekoduje (m_isPlaying == true)
        if (frame.isNull() || !m_isPlaying) return; // Dodano sprawdzenie m_isPlaying

        // Ustaw pixmapę - setScaledContents zajmie się resztą
        m_gifLabel->setPixmap(QPixmap::fromImage(frame));

        // Nie ma potrzeby aktualizować geometrii dla każdej klatki,
        // chyba że rozmiar GIFa się zmienia dynamicznie (rzadkie)
    }


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

        if (!m_thumbnailPixmap.isNull() && !m_isPlaying) {
            m_gifLabel->setPixmap(m_thumbnailPixmap.scaled(
                m_gifLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            ));
        }
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
    bool m_isPlaying; // Flaga śledząca, czy aktywnie odtwarzamy
    QPixmap m_thumbnailPixmap;

    // Usunięto s_activePlayer
};

#endif //INLINE_GIF_PLAYER_H