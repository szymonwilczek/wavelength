#include "cyber_text_display.h"

#include <QPropertyAnimation>

CyberTextDisplay::CyberTextDisplay(const QString &text, const TypingSoundType sound_type, QWidget *parent): QWidget(parent), full_text_(text), revealed_chars_(0),
                                                                                                     glitch_intensity_(0.0), is_fully_revealed_(false), has_been_fully_revealed_once_(false),
                                                                                                     media_player_(nullptr), audio_output_(nullptr), playlist_(nullptr), sound_type_(sound_type) {
    setMinimumWidth(400);
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Ładujemy albo upewniamy się, że mamy dobrą czcionkę dla efektu terminala
    const int font_id = QFontDatabase::addApplicationFont(":/fonts/ShareTechMono-Regular.ttf");
    QString font_family = "Consolas"; // fallback

    if (font_id != -1) {
        const QStringList font_families = QFontDatabase::applicationFontFamilies(font_id);
        if (!font_families.isEmpty()) {
            font_family = font_families.at(0);
        }
    }

    // Usuwamy HTML z tekstu
    plain_text_ = RemoveHtml(full_text_);

    // Timer animacji tekstu
    text_timer_ = new QTimer(this);
    connect(text_timer_, &QTimer::timeout, this, &CyberTextDisplay::RevealNextChar);

    // Timer dla efektów glitch
    glitch_timer_ = new QTimer(this);
    connect(glitch_timer_, &QTimer::timeout, this, &CyberTextDisplay::RandomGlitch);
    glitch_timer_->start(100); // szybkie aktualizacje dla lepszego efektu

    // Inicjujemy czcionkę
    font_ = QFont(font_family, 10);
    font_.setStyleHint(QFont::Monospace);

    // --- INICJALIZACJA DŹWIĘKU PISANIA (QMediaPlayer z QMediaPlaylist) ---
    media_player_ = new QMediaPlayer(this);
    playlist_ = new QMediaPlaylist(this); // Utwórz playlistę

    QUrl sound_url;
    if (sound_type_ == kSystemSound) {
        qDebug() << "Using system sound for typing effect.";
        sound_url = QUrl("qrc:/resources/sounds/interface/terminal_typing2.wav");
    } else {
        sound_url = QUrl("qrc:/resources/sounds/interface/terminal_typing1.wav");
    }

    // Dodaj wybrany plik dźwiękowy do playlisty
    playlist_->addMedia(QMediaContent(sound_url));
    playlist_->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop); // Ustaw zapętlanie bieżącego elementu

    // Przypisz playlistę do odtwarzacza
    media_player_->setPlaylist(playlist_);


    connect(media_player_, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, [this](const QMediaPlayer::Error error){
        qWarning() << "MediaPlayer Error:" << error << media_player_->errorString();
        // Można też sprawdzić błąd playlisty: m_playlist->errorString()
    });

    // Można nadal monitorować status odtwarzacza, np. aby ustawić głośność po załadowaniu
    connect(media_player_, &QMediaPlayer::mediaStatusChanged, this, [this](const QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::LoadedMedia) {
            media_player_->setVolume(80); // Ustaw głośność (0-100)
        }
        // Można też monitorować status playlisty: connect(m_playlist, ...)
    });
    // --- KONIEC INICJALIZACJI DŹWIĘKU ---

    connect(this, &CyberTextDisplay::fullTextRevealed, this, &CyberTextDisplay::HandleFullTextRevealed);

    // Policz potrzebną wysokość
    RecalculateHeight();
}

CyberTextDisplay::~CyberTextDisplay() {
    // Nie ma potrzeby jawnego zatrzymywania, QObject powinien posprzątać
    // Ale jeśli chcesz być pewien:
    if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
        media_player_->stop();
    }
}

void CyberTextDisplay::StartReveal() {
    if (has_been_fully_revealed_once_) {
        SetRevealedChars(plain_text_.length());
        text_timer_->stop();
        if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
            media_player_->stop(); // Zatrzymaj, jeśli gra
        }
        return;
    }

    revealed_chars_ = 0;
    is_fully_revealed_ = false;
    text_timer_->stop();
    text_timer_->start(30);

    // --- ROZPOCZNIJ ODTWARZANIE DŹWIĘKU ---
    // Sprawdź status medium przed odtworzeniem
    if (media_player_ && media_player_->mediaStatus() >= QMediaPlayer::LoadedMedia && media_player_->state() != QMediaPlayer::PlayingState) {
        media_player_->play();
    } else if (media_player_ && media_player_->mediaStatus() < QMediaPlayer::LoadedMedia) {
        qWarning() << "MediaPlayer: Media not loaded yet, cannot play.";
        // Można spróbować odtworzyć po załadowaniu, łącząc się z mediaStatusChanged
    }
    // --- KONIEC ODTWARZANIA ---

    update();
}

void CyberTextDisplay::SetText(const QString &new_text) {
    full_text_ = new_text;
    plain_text_ = RemoveHtml(full_text_);
    has_been_fully_revealed_once_ = false; // Resetuj flagę dla nowej treści
    RecalculateHeight(); // Przelicz wysokość dla nowego tekstu
    StartReveal();       // Rozpocznij animację od nowa
}

void CyberTextDisplay::SetRevealedChars(const int chars) {
    revealed_chars_ = qMin(chars, plain_text_.length());
    const bool just_revealed = (revealed_chars_ >= plain_text_.length() && !is_fully_revealed_);

    update(); // Zaplanuj odświeżenie interfejsu *przed* emisją sygnału

    if (just_revealed) {
        is_fully_revealed_ = true;
        if (!has_been_fully_revealed_once_) { // Ustaw flagę tylko przy pierwszym razie
            has_been_fully_revealed_once_ = true;
        }
        emit fullTextRevealed(); // Emituj sygnał *po* update()
    }
}

void CyberTextDisplay::SetGlitchIntensity(const qreal intensity) {
    glitch_intensity_ = intensity;
    update();
}

void CyberTextDisplay::SetGlitchEffectEnabled(const bool enabled) {
    if (enabled && !glitch_timer_->isActive()) {
        glitch_timer_->start(100);
    } else if (!enabled && glitch_timer_->isActive()) {
        glitch_timer_->stop();
        glitch_intensity_ = 0.0;
        update();
    }
}

QSize CyberTextDisplay::sizeHint() const {
    const QFontMetrics font_metrics(font_);
    constexpr int width = 300; // minimalna szerokość
    const int height = font_metrics.lineSpacing() * (plain_text_.count('\n') + 1) + 20; // margines
    return QSize(width, height);
}

void CyberTextDisplay::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tło z subtelnym gradientem
    QLinearGradient background_gradient(0, 0, width(), height());
    background_gradient.setColorAt(0, QColor(5, 10, 15, 150));
    background_gradient.setColorAt(1, QColor(10, 20, 30, 150));
    painter.fillRect(rect(), background_gradient);

    // Ustawienie czcionki
    painter.setFont(font_);

    // Rysujemy tekst znak po znaku, aby zastosować efekty
    QString visible_text = plain_text_.left(revealed_chars_);

    // Podstawowe parametry
    QFontMetrics font_metrics(font_);
    int line_height = font_metrics.lineSpacing();
    int top_margin = 10;
    int left_margin = 15;

    // Kolor tekstu - neonowy zielono-cyjanowy
    QColor base_text_color(0, 255, 200);

    // Rysujemy linię po linii, aby zastosować efekty
    QStringList lines = visible_text.split('\n');
    int y = top_margin + font_metrics.ascent();

    for (int line_index = 0; line_index < lines.size(); ++line_index) {
        QString line = lines[line_index];
        int x = left_margin;

        for (int i = 0; i < line.length(); ++i) {
            // Obliczamy kolor na podstawie pozycji (odległość od kursora)
            int distance_from_cursor = qAbs(i + line_index * 80 - revealed_chars_);
            int brightness = qMax(180, 255 - distance_from_cursor * 3);
            QColor char_color = base_text_color.lighter(brightness);

            // Dodajemy losowe glitche dla niektórych znaków
            if (glitch_intensity_ > 0.01 && QRandomGenerator::global()->bounded(100) < glitch_intensity_ * 100) {
                // Zamieniamy znak na losowy
                auto glitched_char = QChar(QRandomGenerator::global()->bounded(33, 126));
                char_color = QColor(0, 255, 255); // Błękitny dla glitchów
                painter.setPen(char_color);
                painter.drawText(x, y, QString(glitched_char));
            } else {
                // Normalny znak
                painter.setPen(char_color);
                painter.drawText(x, y, QString(line[i]));
            }

            x += font_metrics.horizontalAdvance(line[i]);
        }

        y += line_height;
    }

    // Migający kursor terminala
    if (revealed_chars_ < plain_text_.length() ||
        (QDateTime::currentMSecsSinceEpoch() % 1000) < 500) {

        // Znajdź pozycję ostatniego znaku
        int cursor_line = visible_text.count('\n');
        int cursor_position = visible_text.length() - visible_text.lastIndexOf('\n') - 1;

        // Jeśli nie ma żadnego znaku jeszcze, kursor jest na początku
        if (visible_text.isEmpty()) {
            cursor_line = 0;
            cursor_position = 0;
        }

        // Rysujemy kursor
        int cursor_x = left_margin + font_metrics.horizontalAdvance(lines.isEmpty() ? "" : lines.last().left(cursor_position));
        int cursor_y = top_margin + font_metrics.ascent() + cursor_line * line_height;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 255, 200));
        painter.drawRect(cursor_x, cursor_y - font_metrics.ascent() + 2, 8, font_metrics.height() - 2);
    }

    // Skanline lines - subtelne poziome linie
    painter.setOpacity(0.1);
    painter.setPen(QPen(QColor(0, 255, 200), 1, Qt::DotLine));

    for (int i = 0; i < height(); i += 4) {
        painter.drawLine(0, i, width(), i);
    }
}

void CyberTextDisplay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    RecalculateHeight();
}

void CyberTextDisplay::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    // Zatrzymaj dźwięk, gdy widget jest ukrywany
    if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
        media_player_->stop();
    }
    // Zatrzymaj też timery na wszelki wypadek
    text_timer_->stop();
    glitch_timer_->stop();
}

void CyberTextDisplay::HandleFullTextRevealed() const {
    if (text_timer_->isActive()) {
        text_timer_->stop();
    }
    // Zatrzymaj dźwięk w następnej iteracji pętli zdarzeń
    if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
        QTimer::singleShot(0, this, [this]() {
            // Sprawdź ponownie stan i wskaźnik przed zatrzymaniem
            if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
                media_player_->stop();
            }
        });
    }
}

void CyberTextDisplay::RevealNextChar() {
    if (revealed_chars_ < plain_text_.length()) {
        // Zwiększamy liczbę widocznych znaków
        SetRevealedChars(revealed_chars_ + 1);

        // Spowalniamy tempo przy końcu słów dla lepszego efektu
        if (revealed_chars_ < plain_text_.length() &&
            (plain_text_[revealed_chars_] == ' ' || plain_text_[revealed_chars_] == '\n')) {
            text_timer_->setInterval(60); // przerwa przy końcu słowa
        } else {
            text_timer_->setInterval(30); // normalne tempo
        }

        // Dodajemy losowe zakłócenia
        if (QRandomGenerator::global()->bounded(100) < 5) {
            TriggerGlitch();
        }
    }
}

void CyberTextDisplay::RandomGlitch() {
    // Losowo aktywuje efekt glitch
    if (QRandomGenerator::global()->bounded(100) < 3) {
        TriggerGlitch();
    } else {
        // Stopniowo zmniejszamy intensywność
        if (glitch_intensity_ > 0.0) {
            SetGlitchIntensity(qMax(0.0, glitch_intensity_ - 0.05));
        }
    }
}

void CyberTextDisplay::TriggerGlitch() {
    // Krótkotrwały efekt zakłóceń
    const auto glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    glitch_animation->setDuration(200);
    glitch_animation->setStartValue(0.2);
    glitch_animation->setEndValue(0.0);
    glitch_animation->setEasingCurve(QEasingCurve::OutQuad);
    glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QString CyberTextDisplay::RemoveHtml(const QString &html) {
    QString text = html;
    text.remove(QRegExp("<[^>]*>"));   // Usuwa tagi HTML
    text = text.simplified();          // Usuwa nadmiarowe whitespace

    // Poprawki dla znaków specjalnych
    text.replace("&nbsp;", " ");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&amp;", "&");

    return text;
}

void CyberTextDisplay::RecalculateHeight() {
    const QFontMetrics font_metrics(font_);
    const int text_width = qMax(300, width() - 30); // Minimalna szerokość 300px

    // Zachowaj oryginalny tekst jeśli się mieści w jednej linii
    if (font_metrics.horizontalAdvance(plain_text_) <= text_width) {
        // Tekst mieści się, nie trzeba nic robić
        const int required_height = font_metrics.lineSpacing() + 20;
        setMinimumHeight(required_height);
        return;
    }

    // Wrapowanie tekstu
    QStringList words = plain_text_.split(' ', Qt::SkipEmptyParts);
    QString wrapped_text;
    QString current_line;

    for (const QString& word : words) {
        QString test_line = current_line.isEmpty() ? word : current_line + ' ' + word;
        if (font_metrics.horizontalAdvance(test_line) <= text_width) {
            current_line = test_line;
        } else {
            wrapped_text += current_line + '\n';
            current_line = word;
        }
    }

    if (!current_line.isEmpty()) {
        wrapped_text += current_line;
    }

    // Zapisujemy przeformatowany tekst i aktualizujemy wysokość
    plain_text_ = wrapped_text;

    const int line_count = plain_text_.count('\n') + 1;
    const int height = line_count * font_metrics.lineSpacing() + 20; // marginesy

    setMinimumHeight(height);
}
