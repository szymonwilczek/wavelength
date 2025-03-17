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

#include "../../audio/player/inline_audio_player.h"
#include "../../video/player/inline_video_player.h"
#include "../decoder/gif_decoder.h"

// Klasa odtwarzacza GIF zintegrowanego z czatem
class InlineGifPlayer : public QFrame {
    Q_OBJECT

public:
    InlineGifPlayer(const QByteArray& gifData, QWidget* parent = nullptr)
        : QFrame(parent), m_gifData(gifData) {

        setFrameStyle(QFrame::StyledPanel);
        setStyleSheet("background-color: #1a1a1a; border: 1px solid #444;");
        
        // Początkowy rozmiar będzie automatycznie dostosowany po otrzymaniu informacji o GIF-ie
        setMaximumSize(800, 800);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Obszar wyświetlania GIF-a
        m_gifLabel = new QLabel(this);
        m_gifLabel->setAlignment(Qt::AlignCenter);
        m_gifLabel->setStyleSheet("background-color: #1a1a1a; color: #ffffff;");
        m_gifLabel->setText("Ładowanie GIF...");
        layout->addWidget(m_gifLabel);

        // Dekoder GIF
        m_decoder = std::make_shared<GifDecoder>(gifData, this);

        // Połącz sygnały
        connect(m_decoder.get(), &GifDecoder::frameReady, this, &InlineGifPlayer::updateFrame, Qt::QueuedConnection);
        connect(m_decoder.get(), &GifDecoder::error, this, &InlineGifPlayer::handleError);
        connect(m_decoder.get(), &GifDecoder::gifInfo, this, &InlineGifPlayer::handleGifInfo);
        connect(m_decoder.get(), &GifDecoder::playbackFinished, this, &InlineGifPlayer::handlePlaybackFinished);

        // Połączenie dla aktualizacji pozycji (opcjonalne dla GIF-ów)
        connect(m_decoder.get(), &GifDecoder::positionChanged, this, &InlineGifPlayer::updatePosition);

        // Inicjalizuj odtwarzacz w osobnym wątku
        QTimer::singleShot(100, this, [this]() {
            m_decoder->start();
        });

        // Obsługa zamykania aplikacji
        connect(qApp, &QApplication::aboutToQuit, this, [this]() {
            if (s_activePlayer == this) {
                s_activePlayer = nullptr;
            }
            releaseResources();
        });
        
        // Umożliwienie kliknięcia, aby zatrzymać/wznowić GIF
        m_gifLabel->installEventFilter(this);
        m_gifLabel->setCursor(Qt::PointingHandCursor);
    }

    static InlineGifPlayer* getActivePlayer() {
        return s_activePlayer;
    }

    ~InlineGifPlayer() {
        releaseResources();
    }

    void releaseResources() {
        if (m_decoder) {
            m_decoder->stop();
            m_decoder->wait(500); // Czekaj max 500ms na zakończenie wątku
            m_decoder->releaseResources();
        }

        if (s_activePlayer == this) {
            s_activePlayer = nullptr;
        }
        m_isActive = false;
    }

    void activate() {
        // Jeśli już jest aktywny, nic nie rób
        if (m_isActive) return;

        // Dezaktywuj aktualnie aktywny odtwarzacz GIF
        if (s_activePlayer && s_activePlayer != this) {
            s_activePlayer->deactivate();
        }

        // Dezaktywuj aktywny odtwarzacz audio, jeśli taki istnieje
        if (InlineAudioPlayer::getActivePlayer() != nullptr) {
            InlineAudioPlayer::getActivePlayer()->deactivate();
        }

        // Dezaktywuj aktywny odtwarzacz wideo, jeśli taki istnieje
        if (InlineVideoPlayer::getActivePlayer() != nullptr) {
            InlineVideoPlayer::getActivePlayer()->deactivate();
        }

        // Aktywuj ten odtwarzacz
        m_isActive = true;
        s_activePlayer = this;

        // Upewnij się, że dekoder jest w odpowiednim stanie
        if (!m_decoder->isRunning()) {
            if (!m_decoder->reinitialize()) {
                qDebug() << "Nie udało się zainicjalizować dekodera GIF";
                return;
            }
            m_decoder->start();
        }

        // Wznów odtwarzanie jeśli było zatrzymane
        if (m_decoder->isPaused()) {
            m_decoder->pause(); // Przełącza stan pauzy
        }
    }

    void deactivate() {
        if (!m_isActive) return;

        // Zatrzymaj odtwarzanie
        if (m_decoder && !m_decoder->isPaused()) {
            m_decoder->pause();  // Pauzuj odtwarzanie
        }

        // Wyświetl pierwszą klatkę jako nieruchomy obraz
        if (!m_thumbnailFrame.isNull()) {
            displayScaledImage(m_thumbnailFrame);
        }

        // Usuń status aktywnego, jeśli to ten odtwarzacz
        if (s_activePlayer == this) {
            s_activePlayer = nullptr;
        }

        m_isActive = false;
    }

    // Obsługa kliknięcia w GIF (pauza/wznów)
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_gifLabel && event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                togglePlayback();
                return true;
            }
        }
        return QFrame::eventFilter(watched, event);
    }

    // Efekt najechania myszą
    void enterEvent(QEnterEvent *event) {
        // Aktywuj po najechaniu myszą
        activate();
        QFrame::enterEvent(event);
    }

private slots:
    void togglePlayback() {
        if (!m_decoder) return;

        // Aktywuj ten odtwarzacz przed odtwarzaniem
        activate();

        // Przełącz stan pauzy
        m_decoder->pause();

        // Wizualne wskazanie stanu pauzy można dodać jeśli potrzeba
        // np. dodając przezroczysty przycisk play na środku podczas pauzy
    }

    void updateFrame(const QImage& frame) {
        // Jeśli to pierwsza klatka, zapisz ją jako miniaturkę
        if (m_thumbnailFrame.isNull()) {
            m_thumbnailFrame = frame.copy();
        }

        displayScaledImage(frame);
    }

    void displayScaledImage(const QImage& image) {
        // Obliczamy rozmiar skalowanej ramki z zachowaniem proporcji
        // i ograniczeniem do sensownego maksymalnego rozmiaru
        QSize targetSize = image.size();
        int maxWidth = std::min(image.width(), 600); // Maksymalna szerokość GIF-a
        int maxHeight = std::min(image.height(), 600); // Maksymalna wysokość GIF-a
        
        targetSize.scale(maxWidth, maxHeight, Qt::KeepAspectRatio);

        // Skalujemy oryginalną klatkę tylko jeśli trzeba
        QImage scaledImage;
        if (image.width() > maxWidth || image.height() > maxHeight) {
            scaledImage = image.scaled(targetSize.width(), targetSize.height(),
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            scaledImage = image; // Użyj oryginalnego rozmiaru
        }

        // Aktualizujemy rozmiar widgetu do rozmiaru GIF-a
        setFixedSize(scaledImage.width(), scaledImage.height());

        // Wyświetlamy obraz
        m_gifLabel->setPixmap(QPixmap::fromImage(scaledImage));
        m_gifLabel->setFixedSize(scaledImage.size());
    }

    void updatePosition(double position) {
        // Aktualizacja pozycji (opcjonalna dla GIF-ów)
        m_currentPosition = position;
    }

    void handleError(const QString& message) {
        qDebug() << "GIF decoder error:" << message;
        m_gifLabel->setText("⚠️ " + message);
    }

    void handleGifInfo(int width, int height, double duration, double frameRate, int numStreams) {
        m_gifWidth = width;
        m_gifHeight = height;
        m_gifDuration = duration;
        m_frameRate = frameRate;
        
        qDebug() << "GIF info - szerokość:" << width << "wysokość:" << height 
                 << "czas trwania:" << duration << "s, FPS:" << frameRate;

        // Pobierz pierwszą klatkę jako miniaturkę
        QTimer::singleShot(100, this, [this]() {
            m_decoder->extractFirstFrame();
        });
    }

    void handlePlaybackFinished() {
        // GIF-y zazwyczaj automatycznie się zapętlają w dekoderze,
        // więc to zdarzenie może nie być potrzebne
    }

private:
    QLabel* m_gifLabel;
    std::shared_ptr<GifDecoder> m_decoder;

    QByteArray m_gifData;
    
    int m_gifWidth = 0;
    int m_gifHeight = 0;
    double m_gifDuration = 0;
    double m_frameRate = 0;
    double m_currentPosition = 0;
    bool m_isActive = false;
    QImage m_thumbnailFrame;                 // Przechowuje miniaturkę dla zatrzymanego odtwarzacza
    
    inline static InlineGifPlayer* s_activePlayer = nullptr;
};

#endif //INLINE_GIF_PLAYER_H