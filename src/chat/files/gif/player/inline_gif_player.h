#ifndef INLINE_GIF_PLAYER_H
#define INLINE_GIF_PLAYER_H

#include <memory>
#include <QMouseEvent>

#include "../../audio/player/inline_audio_player.h"
#include "../decoder/gif_decoder.h"

// Klasa odtwarzacza GIF zintegrowanego z czatem
class InlineGifPlayer final : public QFrame {
    Q_OBJECT

public:
    explicit InlineGifPlayer(const QByteArray& gifData, QWidget* parent = nullptr);

    // Usunięto getActivePlayer i logikę s_activePlayer

    ~InlineGifPlayer() override {
        releaseResources();
    }

    void releaseResources();

    // Zwraca oryginalny rozmiar GIF-a jako wskazówkę
    QSize sizeHint() const override;

    void startPlayback();

    // Metoda do jawnego zatrzymania/pauzowania odtwarzania (np. dla dialogu)
    void stopPlayback();

protected:
    // Przechwytywanie zdarzeń najechania i opuszczenia myszy
    void enterEvent(QEvent *event) override;

    void leaveEvent(QEvent *event) override;

private slots:
    void displayThumbnail(const QImage& frame);

    void updateFrame(const QImage& frame) const;


    void updatePosition(const double position) {
        m_currentPosition = position;
    }

    void handleError(const QString& message);

    void handleGifInfo(int width, int height, double duration, double frameRate, int numStreams);

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