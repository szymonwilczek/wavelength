#include "cyber_text_display.h"

#include <QPropertyAnimation>

CyberTextDisplay::CyberTextDisplay(const QString &text, TypingSoundType soundType, QWidget *parent): QWidget(parent), m_fullText(text), m_revealedChars(0),
                                                                                                     m_glitchIntensity(0.0), m_isFullyRevealed(false), m_hasBeenFullyRevealedOnce(false),
                                                                                                     m_mediaPlayer(nullptr), m_audioOutput(nullptr), m_playlist(nullptr), m_soundType(soundType) {
    setMinimumWidth(400);
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Ładujemy albo upewniamy się, że mamy dobrą czcionkę dla efektu terminala
    int fontId = QFontDatabase::addApplicationFont(":/fonts/ShareTechMono-Regular.ttf");
    QString fontFamily = "Consolas"; // fallback

    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            fontFamily = fontFamilies.at(0);
        }
    }

    // Usuwamy HTML z tekstu
    m_plainText = removeHtml(m_fullText);

    // Timer animacji tekstu
    m_textTimer = new QTimer(this);
    connect(m_textTimer, &QTimer::timeout, this, &CyberTextDisplay::revealNextChar);

    // Timer dla efektów glitch
    m_glitchTimer = new QTimer(this);
    connect(m_glitchTimer, &QTimer::timeout, this, &CyberTextDisplay::randomGlitch);
    m_glitchTimer->start(100); // szybkie aktualizacje dla lepszego efektu

    // Inicjujemy czcionkę
    m_font = QFont(fontFamily, 10);
    m_font.setStyleHint(QFont::Monospace);

    // --- INICJALIZACJA DŹWIĘKU PISANIA (QMediaPlayer z QMediaPlaylist) ---
    m_mediaPlayer = new QMediaPlayer(this);
    m_playlist = new QMediaPlaylist(this); // Utwórz playlistę

    QUrl soundUrl;
    if (m_soundType == SystemSound) {
        qDebug() << "Using system sound for typing effect.";
        soundUrl = QUrl("qrc:/resources/sounds/interface/terminal_typing2.wav");
    } else {
        soundUrl = QUrl("qrc:/resources/sounds/interface/terminal_typing1.wav");
    }

    // Dodaj wybrany plik dźwiękowy do playlisty
    m_playlist->addMedia(QMediaContent(soundUrl));
    m_playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop); // Ustaw zapętlanie bieżącego elementu

    // Przypisz playlistę do odtwarzacza
    m_mediaPlayer->setPlaylist(m_playlist);


    connect(m_mediaPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, [this](QMediaPlayer::Error error){
        qWarning() << "MediaPlayer Error:" << error << m_mediaPlayer->errorString();
        // Można też sprawdzić błąd playlisty: m_playlist->errorString()
    });

    // Można nadal monitorować status odtwarzacza, np. aby ustawić głośność po załadowaniu
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::LoadedMedia) {
            m_mediaPlayer->setVolume(80); // Ustaw głośność (0-100)
        }
        // Można też monitorować status playlisty: connect(m_playlist, ...)
    });
    // --- KONIEC INICJALIZACJI DŹWIĘKU ---

    connect(this, &CyberTextDisplay::fullTextRevealed, this, &CyberTextDisplay::handleFullTextRevealed);

    // Policz potrzebną wysokość
    recalculateHeight();
}

CyberTextDisplay::~CyberTextDisplay() {
    // Nie ma potrzeby jawnego zatrzymywania, QObject powinien posprzątać
    // Ale jeśli chcesz być pewien:
    if (m_mediaPlayer && m_mediaPlayer->state() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->stop();
    }
}

void CyberTextDisplay::startReveal() {
    if (m_hasBeenFullyRevealedOnce) {
        setRevealedChars(m_plainText.length());
        m_textTimer->stop();
        if (m_mediaPlayer && m_mediaPlayer->state() == QMediaPlayer::PlayingState) {
            m_mediaPlayer->stop(); // Zatrzymaj, jeśli gra
        }
        return;
    }

    m_revealedChars = 0;
    m_isFullyRevealed = false;
    m_textTimer->stop();
    m_textTimer->start(30);

    // --- ROZPOCZNIJ ODTWARZANIE DŹWIĘKU ---
    // Sprawdź status medium przed odtworzeniem
    if (m_mediaPlayer && m_mediaPlayer->mediaStatus() >= QMediaPlayer::LoadedMedia && m_mediaPlayer->state() != QMediaPlayer::PlayingState) {
        m_mediaPlayer->play();
    } else if (m_mediaPlayer && m_mediaPlayer->mediaStatus() < QMediaPlayer::LoadedMedia) {
        qWarning() << "MediaPlayer: Media not loaded yet, cannot play.";
        // Można spróbować odtworzyć po załadowaniu, łącząc się z mediaStatusChanged
    }
    // --- KONIEC ODTWARZANIA ---

    update();
}

void CyberTextDisplay::setText(const QString &newText) {
    m_fullText = newText;
    m_plainText = removeHtml(m_fullText);
    m_hasBeenFullyRevealedOnce = false; // Resetuj flagę dla nowej treści
    recalculateHeight(); // Przelicz wysokość dla nowego tekstu
    startReveal();       // Rozpocznij animację od nowa
}

void CyberTextDisplay::setRevealedChars(int chars) {
    m_revealedChars = qMin(chars, m_plainText.length());
    bool justRevealed = (m_revealedChars >= m_plainText.length() && !m_isFullyRevealed);

    update(); // Zaplanuj odświeżenie interfejsu *przed* emisją sygnału

    if (justRevealed) {
        m_isFullyRevealed = true;
        if (!m_hasBeenFullyRevealedOnce) { // Ustaw flagę tylko przy pierwszym razie
            m_hasBeenFullyRevealedOnce = true;
        }
        emit fullTextRevealed(); // Emituj sygnał *po* update()
    }
}

void CyberTextDisplay::setGlitchIntensity(qreal intensity) {
    m_glitchIntensity = intensity;
    update();
}

void CyberTextDisplay::setGlitchEffectEnabled(bool enabled) {
    if (enabled && !m_glitchTimer->isActive()) {
        m_glitchTimer->start(100);
    } else if (!enabled && m_glitchTimer->isActive()) {
        m_glitchTimer->stop();
        m_glitchIntensity = 0.0;
        update();
    }
}

QSize CyberTextDisplay::sizeHint() const {
    QFontMetrics fm(m_font);
    int width = 300; // minimalna szerokość
    int height = fm.lineSpacing() * (m_plainText.count('\n') + 1) + 20; // margines
    return QSize(width, height);
}

void CyberTextDisplay::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tło z subtelnym gradientem
    QLinearGradient bgGradient(0, 0, width(), height());
    bgGradient.setColorAt(0, QColor(5, 10, 15, 150));
    bgGradient.setColorAt(1, QColor(10, 20, 30, 150));
    painter.fillRect(rect(), bgGradient);

    // Ustawienie czcionki
    painter.setFont(m_font);

    // Rysujemy tekst znak po znaku, aby zastosować efekty
    QString visibleText = m_plainText.left(m_revealedChars);

    // Podstawowe parametry
    QFontMetrics fm(m_font);
    int lineHeight = fm.lineSpacing();
    int topMargin = 10;
    int leftMargin = 15;

    // Kolor tekstu - neonowy zielono-cyjanowy
    QColor baseTextColor(0, 255, 200);

    // Rysujemy linię po linii, aby zastosować efekty
    QStringList lines = visibleText.split('\n');
    int y = topMargin + fm.ascent();

    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        QString line = lines[lineIndex];
        int x = leftMargin;

        for (int i = 0; i < line.length(); ++i) {
            // Obliczamy kolor na podstawie pozycji (odległość od kursora)
            int distanceFromCursor = qAbs(i + lineIndex * 80 - m_revealedChars);
            int brightness = qMax(180, 255 - distanceFromCursor * 3);
            QColor charColor = baseTextColor.lighter(brightness);

            // Dodajemy losowe glitche dla niektórych znaków
            if (m_glitchIntensity > 0.01 && QRandomGenerator::global()->bounded(100) < m_glitchIntensity * 100) {
                // Zamieniamy znak na losowy
                QChar glitchedChar = QChar(QRandomGenerator::global()->bounded(33, 126));
                charColor = QColor(0, 255, 255); // Błękitny dla glitchów
                painter.setPen(charColor);
                painter.drawText(x, y, QString(glitchedChar));
            } else {
                // Normalny znak
                painter.setPen(charColor);
                painter.drawText(x, y, QString(line[i]));
            }

            x += fm.horizontalAdvance(line[i]);
        }

        y += lineHeight;
    }

    // Migający kursor terminala
    if (m_revealedChars < m_plainText.length() ||
        (QDateTime::currentMSecsSinceEpoch() % 1000) < 500) {

        // Znajdź pozycję ostatniego znaku
        int cursorLine = visibleText.count('\n');
        int cursorPos = visibleText.length() - visibleText.lastIndexOf('\n') - 1;

        // Jeśli nie ma żadnego znaku jeszcze, kursor jest na początku
        if (visibleText.isEmpty()) {
            cursorLine = 0;
            cursorPos = 0;
        }

        // Rysujemy kursor
        int cursorX = leftMargin + fm.horizontalAdvance(lines.isEmpty() ? "" : lines.last().left(cursorPos));
        int cursorY = topMargin + fm.ascent() + cursorLine * lineHeight;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 255, 200));
        painter.drawRect(cursorX, cursorY - fm.ascent() + 2, 8, fm.height() - 2);
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
    recalculateHeight();
}

void CyberTextDisplay::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    // Zatrzymaj dźwięk, gdy widget jest ukrywany
    if (m_mediaPlayer && m_mediaPlayer->state() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->stop();
    }
    // Zatrzymaj też timery na wszelki wypadek
    m_textTimer->stop();
    m_glitchTimer->stop();
}

void CyberTextDisplay::handleFullTextRevealed() const {
    if (m_textTimer->isActive()) {
        m_textTimer->stop();
    }
    // Zatrzymaj dźwięk w następnej iteracji pętli zdarzeń
    if (m_mediaPlayer && m_mediaPlayer->state() == QMediaPlayer::PlayingState) {
        QTimer::singleShot(0, this, [this]() {
            // Sprawdź ponownie stan i wskaźnik przed zatrzymaniem
            if (m_mediaPlayer && m_mediaPlayer->state() == QMediaPlayer::PlayingState) {
                m_mediaPlayer->stop();
            }
        });
    }
}

void CyberTextDisplay::revealNextChar() {
    if (m_revealedChars < m_plainText.length()) {
        // Zwiększamy liczbę widocznych znaków
        setRevealedChars(m_revealedChars + 1);

        // Spowalniamy tempo przy końcu słów dla lepszego efektu
        if (m_revealedChars < m_plainText.length() &&
            (m_plainText[m_revealedChars] == ' ' || m_plainText[m_revealedChars] == '\n')) {
            m_textTimer->setInterval(60); // przerwa przy końcu słowa
        } else {
            m_textTimer->setInterval(30); // normalne tempo
        }

        // Dodajemy losowe zakłócenia
        if (QRandomGenerator::global()->bounded(100) < 5) {
            triggerGlitch();
        }
    }
}

void CyberTextDisplay::randomGlitch() {
    // Losowo aktywuje efekt glitch
    if (QRandomGenerator::global()->bounded(100) < 3) {
        triggerGlitch();
    } else {
        // Stopniowo zmniejszamy intensywność
        if (m_glitchIntensity > 0.0) {
            setGlitchIntensity(qMax(0.0, m_glitchIntensity - 0.05));
        }
    }
}

void CyberTextDisplay::triggerGlitch() {
    // Krótkotrwały efekt zakłóceń
    QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
    glitchAnim->setDuration(200);
    glitchAnim->setStartValue(0.2);
    glitchAnim->setEndValue(0.0);
    glitchAnim->setEasingCurve(QEasingCurve::OutQuad);
    glitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

QString CyberTextDisplay::removeHtml(const QString &html) {
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

void CyberTextDisplay::recalculateHeight() {
    QFontMetrics fm(m_font);
    int textWidth = qMax(300, width() - 30); // Minimalna szerokość 300px

    // Zachowaj oryginalny tekst jeśli się mieści w jednej linii
    if (fm.horizontalAdvance(m_plainText) <= textWidth) {
        // Tekst mieści się, nie trzeba nic robić
        int requiredHeight = fm.lineSpacing() + 20;
        setMinimumHeight(requiredHeight);
        return;
    }

    // Wrapowanie tekstu
    QStringList words = m_plainText.split(' ', Qt::SkipEmptyParts);
    QString wrappedText;
    QString currentLine;

    for (const QString& word : words) {
        QString testLine = currentLine.isEmpty() ? word : currentLine + ' ' + word;
        if (fm.horizontalAdvance(testLine) <= textWidth) {
            currentLine = testLine;
        } else {
            wrappedText += currentLine + '\n';
            currentLine = word;
        }
    }

    if (!currentLine.isEmpty()) {
        wrappedText += currentLine;
    }

    // Zapisujemy przeformatowany tekst i aktualizujemy wysokość
    m_plainText = wrappedText;

    int lineCount = m_plainText.count('\n') + 1;
    int requiredHeight = lineCount * fm.lineSpacing() + 20; // marginesy

    setMinimumHeight(requiredHeight);
}
