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
    explicit InlineGifPlayer(const QByteArray& gif_data, QWidget* parent = nullptr);

    // Usunięto getActivePlayer i logikę s_activePlayer

    ~InlineGifPlayer() override {
        ReleaseResources();
    }

    void ReleaseResources();

    // Zwraca oryginalny rozmiar GIF-a jako wskazówkę
    QSize sizeHint() const override;

    void StartPlayback();

    // Metoda do jawnego zatrzymania/pauzowania odtwarzania (np. dla dialogu)
    void StopPlayback();

protected:
    // Przechwytywanie zdarzeń najechania i opuszczenia myszy
    void enterEvent(QEvent *event) override;

    void leaveEvent(QEvent *event) override;

private slots:
    void DisplayThumbnail(const QImage& frame);

    void UpdateFrame(const QImage& frame) const;


    void UpdatePosition(const double position) {
        current_position_ = position;
    }

    void HandleError(const QString& message);

    void HandleGifInfo(int width, int height, double duration, double frame_rate, int num_of_streams);

signals:
    void gifLoaded(); // Nowy sygnał

private:
    QLabel* gif_label_;
    std::shared_ptr<GifDecoder> decoder_;

    QByteArray gif_data_;

    int gif_width_ = 0;
    int gif_height_ = 0;
    double gif_duration_ = 0;
    double frame_rate_ = 0;
    double current_position_ = 0;
    QImage thumbnail_frame_;
    bool is_playing_;
    QPixmap thumbnail_pixmap_;
};

#endif //INLINE_GIF_PLAYER_H