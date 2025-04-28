#include "inline_gif_player.h"

InlineGifPlayer::InlineGifPlayer(const QByteArray &gifData, QWidget *parent): QFrame(parent), m_gifData(gifData) {

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

void InlineGifPlayer::releaseResources() {
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

QSize InlineGifPlayer::sizeHint() const {
    if (m_gifWidth > 0 && m_gifHeight > 0) {
        return QSize(m_gifWidth, m_gifHeight);
    }
    return QFrame::sizeHint(); // Zwróć domyślny, jeśli rozmiar nie jest znany
}

void InlineGifPlayer::startPlayback() {
    qDebug() << "InlineGifPlayer: startPlayback called.";
    if (m_decoder && !m_isPlaying) {
        m_isPlaying = true;
        m_decoder->resume();
    }
}

void InlineGifPlayer::stopPlayback() {
    qDebug() << "InlineGifPlayer: stopPlayback called.";
    if (m_decoder && m_isPlaying) {
        m_isPlaying = false;
        m_decoder->pause();
    }
}

void InlineGifPlayer::enterEvent(QEvent *event) {
    qDebug() << "InlineGifPlayer: Mouse entered.";
    startPlayback(); // Rozpocznij odtwarzanie przy najechaniu
    QFrame::enterEvent(event);
}

void InlineGifPlayer::leaveEvent(QEvent *event) {
    qDebug() << "InlineGifPlayer: Mouse left.";
    stopPlayback(); // Zatrzymaj odtwarzanie przy opuszczeniu
    QFrame::leaveEvent(event);
}

void InlineGifPlayer::displayThumbnail(const QImage &frame) {
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

void InlineGifPlayer::updateFrame(const QImage &frame) {
    // Ta metoda jest teraz wywoływana tylko gdy dekoder aktywnie dekoduje (m_isPlaying == true)
    if (frame.isNull() || !m_isPlaying) return; // Dodano sprawdzenie m_isPlaying

    // Ustaw pixmapę - setScaledContents zajmie się resztą
    m_gifLabel->setPixmap(QPixmap::fromImage(frame));

    // Nie ma potrzeby aktualizować geometrii dla każdej klatki,
    // chyba że rozmiar GIFa się zmienia dynamicznie (rzadkie)
}

void InlineGifPlayer::handleError(const QString &message) {
    qDebug() << "GIF decoder error:" << message;
    m_gifLabel->setText("⚠️ " + message);
    setMinimumSize(100, 50); // Minimalny rozmiar dla błędu
    updateGeometry();
}

void InlineGifPlayer::handleGifInfo(int width, int height, double duration, double frameRate, int numStreams) {
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
