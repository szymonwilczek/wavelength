#include "inline_gif_player.h"

#include <QApplication>
#include <QTimer>
#include <QVBoxLayout>

InlineGifPlayer::InlineGifPlayer(const QByteArray &gif_data, QWidget *parent): QFrame(parent), gif_data_(gif_data) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Obszar wyświetlania GIF-a
    gif_label_ = new QLabel(this);
    gif_label_->setAlignment(Qt::AlignCenter);
    gif_label_->setStyleSheet("background-color: transparent; color: #ffffff;"); // Przezroczyste tło
    gif_label_->setText("Ładowanie GIF...");
    gif_label_->setScaledContents(true); // Kluczowa zmiana: Włącz skalowanie zawartości QLabel
    layout->addWidget(gif_label_);

    // Dekoder GIF
    decoder_ = std::make_shared<GifDecoder>(gif_data, this);

    // Połącz sygnały
    connect(decoder_.get(), &GifDecoder::firstFrameReady, this, &InlineGifPlayer::DisplayThumbnail, Qt::QueuedConnection);
    connect(decoder_.get(), &GifDecoder::frameReady, this, &InlineGifPlayer::UpdateFrame, Qt::QueuedConnection);
    connect(decoder_.get(), &GifDecoder::error, this, &InlineGifPlayer::HandleError, Qt::QueuedConnection); // Dodano QueuedConnection dla bezpieczeństwa
    connect(decoder_.get(), &GifDecoder::gifInfo, this, &InlineGifPlayer::HandleGifInfo, Qt::QueuedConnection);

    // Inicjalizuj dekoder (wyekstrahuje pierwszą klatkę)
    // Wywołanie w osobnym wątku lub przez QTimer, aby nie blokować UI
    QTimer::singleShot(0, this, [this]() {
        if (decoder_) {
            if (!decoder_->Initialize()) {
                qDebug() << "InlineGifPlayer: Decoder initialization failed.";
                // Można tu obsłużyć błąd inicjalizacji
            }
        }
    });

    // Obsługa zamykania aplikacji
    connect(qApp, &QApplication::aboutToQuit, this, &InlineGifPlayer::ReleaseResources); // Uproszczono lambdę

    setMouseTracking(true);
    gif_label_->setMouseTracking(true);
}

void InlineGifPlayer::ReleaseResources() {
    if (decoder_) {
        decoder_->Stop();
        if (decoder_->IsDecoderRunning()) { // Czekaj tylko jeśli wątek działał
            decoder_->wait(500);
        }
        // releaseResources w dekoderze jest wywoływane w jego destruktorze,
        // ale można też wywołać jawnie, jeśli jest potrzeba
        // m_decoder->releaseResources();
        decoder_.reset();
        is_playing_ = false;
    }
}

QSize InlineGifPlayer::sizeHint() const {
    if (gif_width_ > 0 && gif_height_ > 0) {
        return QSize(gif_width_, gif_height_);
    }
    return QFrame::sizeHint(); // Zwróć domyślny, jeśli rozmiar nie jest znany
}

void InlineGifPlayer::StartPlayback() {
    if (decoder_ && !is_playing_) {
        is_playing_ = true;
        decoder_->Resume();
    }
}

void InlineGifPlayer::StopPlayback() {
    if (decoder_ && is_playing_) {
        is_playing_ = false;
        decoder_->Pause();
    }
}

void InlineGifPlayer::enterEvent(QEvent *event) {
    StartPlayback(); // Rozpocznij odtwarzanie przy najechaniu
    QFrame::enterEvent(event);
}

void InlineGifPlayer::leaveEvent(QEvent *event) {
    StopPlayback(); // Zatrzymaj odtwarzanie przy opuszczeniu
    QFrame::leaveEvent(event);
}

void InlineGifPlayer::DisplayThumbnail(const QImage &frame) {
    if (!frame.isNull()) {
        thumbnail_pixmap_ = QPixmap::fromImage(frame); // Zapisz miniaturkę
        gif_label_->setPixmap(thumbnail_pixmap_.scaled(
            gif_label_->size(), // Skaluj do aktualnego rozmiaru labelki
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
        // Nie aktualizuj geometrii tutaj, poczekaj na gifInfo
    } else {
        gif_label_->setText("⚠️ Błąd ładowania miniatury");
    }
}

void InlineGifPlayer::UpdateFrame(const QImage &frame) const {
    // Ta metoda jest teraz wywoływana tylko gdy dekoder aktywnie dekoduje (m_isPlaying == true)
    if (frame.isNull() || !is_playing_) return; // Dodano sprawdzenie m_isPlaying

    // Ustaw pixmapę - setScaledContents zajmie się resztą
    gif_label_->setPixmap(QPixmap::fromImage(frame));

    // Nie ma potrzeby aktualizować geometrii dla każdej klatki,
    // chyba że rozmiar GIFa się zmienia dynamicznie (rzadkie)
}

void InlineGifPlayer::HandleError(const QString &message) {
    qDebug() << "GIF decoder error:" << message;
    gif_label_->setText("⚠️ " + message);
    setMinimumSize(100, 50); // Minimalny rozmiar dla błędu
    updateGeometry();
}

void InlineGifPlayer::HandleGifInfo(const int width, const int height, const double duration, const double frame_rate, int num_of_streams) {
    gif_width_ = width;
    gif_height_ = height;
    gif_duration_ = duration;
    frame_rate_ = frame_rate;

    // Zaktualizuj geometrię po otrzymaniu informacji o rozmiarze
    updateGeometry();
    emit gifLoaded(); // Sygnalizujemy załadowanie informacji o GIFie

    if (!thumbnail_pixmap_.isNull() && !is_playing_) {
        gif_label_->setPixmap(thumbnail_pixmap_.scaled(
            gif_label_->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }
}
