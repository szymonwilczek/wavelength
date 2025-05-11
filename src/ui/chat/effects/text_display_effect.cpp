#include "text_display_effect.h"

#include <QDateTime>
#include <QFontDatabase>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPainter>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QTimer>

TextDisplayEffect::TextDisplayEffect(const QString &text, const TypingSoundType sound_type,
                                     QWidget *parent): QWidget(parent), full_text_(text), revealed_chars_(0),
                                                       glitch_intensity_(0.0), is_fully_revealed_(false),
                                                       has_been_fully_revealed_once_(false),
                                                       media_player_(nullptr), audio_output_(nullptr),
                                                       playlist_(nullptr),
                                                       sound_type_(sound_type) {
    setMinimumWidth(400);
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    const int font_id = QFontDatabase::addApplicationFont(":/fonts/ShareTechMono-Regular.ttf");
    QString font_family = "Consolas"; // fallback font

    if (font_id != -1) {
        const QStringList font_families = QFontDatabase::applicationFontFamilies(font_id);
        if (!font_families.isEmpty()) {
            font_family = font_families.at(0);
        }
    }

    plain_text_ = RemoveHtml(full_text_);

    text_timer_ = new QTimer(this);
    connect(text_timer_, &QTimer::timeout, this, &TextDisplayEffect::RevealNextChar);

    glitch_timer_ = new QTimer(this);
    connect(glitch_timer_, &QTimer::timeout, this, &TextDisplayEffect::RandomGlitch);
    glitch_timer_->start(100);

    font_ = QFont(font_family, 10);
    font_.setStyleHint(QFont::Monospace);

    media_player_ = new QMediaPlayer(this);
    playlist_ = new QMediaPlaylist(this);

    QUrl sound_url;
    if (sound_type_ == kSystemSound) {
        sound_url = QUrl("qrc:/resources/sounds/interface/terminal_typing2.wav");
    } else {
        sound_url = QUrl("qrc:/resources/sounds/interface/terminal_typing1.wav");
    }

    playlist_->addMedia(QMediaContent(sound_url));
    playlist_->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);

    media_player_->setPlaylist(playlist_);


    connect(media_player_, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
            [this](const QMediaPlayer::Error error) {
                qWarning() << "[CYBER TEXT DISPLAY] MediaPlayer Error:" << error << media_player_->errorString();
            });

    connect(media_player_, &QMediaPlayer::mediaStatusChanged, this, [this](const QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            media_player_->setVolume(80);
        }
    });

    connect(this, &TextDisplayEffect::fullTextRevealed, this, &TextDisplayEffect::HandleFullTextRevealed);

    RecalculateHeight();
}

TextDisplayEffect::~TextDisplayEffect() {
    if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
        media_player_->stop();
    }
}

void TextDisplayEffect::StartReveal() {
    if (has_been_fully_revealed_once_) {
        SetRevealedChars(plain_text_.length());
        text_timer_->stop();
        if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
            media_player_->stop();
        }
        return;
    }

    revealed_chars_ = 0;
    is_fully_revealed_ = false;
    text_timer_->stop();
    text_timer_->start(30);

    if (media_player_ && media_player_->mediaStatus() >= QMediaPlayer::LoadedMedia && media_player_->state() !=
        QMediaPlayer::PlayingState) {
        media_player_->play();
    } else if (media_player_ && media_player_->mediaStatus() < QMediaPlayer::LoadedMedia) {
        qWarning() << "[CYBER TEXT DISPLAY] MediaPlayer: Media not loaded yet, cannot play.";
    }

    update();
}

void TextDisplayEffect::SetText(const QString &new_text) {
    full_text_ = new_text;
    plain_text_ = RemoveHtml(full_text_);
    has_been_fully_revealed_once_ = false;
    RecalculateHeight();
    StartReveal();
}

void TextDisplayEffect::SetRevealedChars(const int chars) {
    revealed_chars_ = qMin(chars, plain_text_.length());
    const bool just_revealed = revealed_chars_ >= plain_text_.length() && !is_fully_revealed_;

    update();

    if (just_revealed) {
        is_fully_revealed_ = true;
        if (!has_been_fully_revealed_once_) {
            has_been_fully_revealed_once_ = true;
        }
        emit fullTextRevealed();
    }
}

void TextDisplayEffect::SetGlitchIntensity(const qreal intensity) {
    glitch_intensity_ = intensity;
    update();
}

void TextDisplayEffect::SetGlitchEffectEnabled(const bool enabled) {
    if (enabled && !glitch_timer_->isActive()) {
        glitch_timer_->start(100);
    } else if (!enabled && glitch_timer_->isActive()) {
        glitch_timer_->stop();
        glitch_intensity_ = 0.0;
        update();
    }
}

QSize TextDisplayEffect::sizeHint() const {
    const QFontMetrics font_metrics(font_);
    constexpr int width = 300;
    const int height = font_metrics.lineSpacing() * (plain_text_.count('\n') + 1) + 20;
    return QSize(width, height);
}

void TextDisplayEffect::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient background_gradient(0, 0, width(), height());
    background_gradient.setColorAt(0, QColor(5, 10, 15, 150));
    background_gradient.setColorAt(1, QColor(10, 20, 30, 150));
    painter.fillRect(rect(), background_gradient);

    painter.setFont(font_);
    QString visible_text = plain_text_.left(revealed_chars_);

    QFontMetrics font_metrics(font_);
    int line_height = font_metrics.lineSpacing();
    int top_margin = 10;
    int left_margin = 15;

    QColor base_text_color(0, 255, 200);

    QStringList lines = visible_text.split('\n');
    int y = top_margin + font_metrics.ascent();

    for (int line_index = 0; line_index < lines.size(); ++line_index) {
        QString line = lines[line_index];
        int x = left_margin;

        for (int i = 0; i < line.length(); ++i) {
            // calculating color based on position (distance from cursor)
            int distance_from_cursor = qAbs(i + line_index * 80 - revealed_chars_);
            int brightness = qMax(180, 255 - distance_from_cursor * 3);
            QColor char_color = base_text_color.lighter(brightness);

            if (glitch_intensity_ > 0.01 && QRandomGenerator::global()->bounded(100) < glitch_intensity_ * 100) {
                auto glitched_char = QChar(QRandomGenerator::global()->bounded(33, 126));
                char_color = QColor(0, 255, 255);
                painter.setPen(char_color);
                painter.drawText(x, y, QString(glitched_char));
            } else {
                painter.setPen(char_color);
                painter.drawText(x, y, QString(line[i]));
            }

            x += font_metrics.horizontalAdvance(line[i]);
        }

        y += line_height;
    }

    // blinking cursor
    if (revealed_chars_ < plain_text_.length() ||
        QDateTime::currentMSecsSinceEpoch() % 1000 < 500) {
        int cursor_line = visible_text.count('\n');
        int cursor_position = visible_text.length() - visible_text.lastIndexOf('\n') - 1;

        if (visible_text.isEmpty()) {
            cursor_line = 0;
            cursor_position = 0;
        }

        int cursor_x = left_margin + font_metrics.horizontalAdvance(
                           lines.isEmpty() ? "" : lines.last().left(cursor_position));
        int cursor_y = top_margin + font_metrics.ascent() + cursor_line * line_height;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 255, 200));
        painter.drawRect(cursor_x, cursor_y - font_metrics.ascent() + 2, 8, font_metrics.height() - 2);
    }

    painter.setOpacity(0.1);
    painter.setPen(QPen(QColor(0, 255, 200), 1, Qt::DotLine));

    for (int i = 0; i < height(); i += 4) {
        painter.drawLine(0, i, width(), i);
    }
}

void TextDisplayEffect::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    RecalculateHeight();
}

void TextDisplayEffect::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
        media_player_->stop();
    }
    text_timer_->stop();
    glitch_timer_->stop();
}

void TextDisplayEffect::HandleFullTextRevealed() const {
    if (text_timer_->isActive()) {
        text_timer_->stop();
    }
    if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
        QTimer::singleShot(0, this, [this] {
            if (media_player_ && media_player_->state() == QMediaPlayer::PlayingState) {
                media_player_->stop();
            }
        });
    }
}

void TextDisplayEffect::RevealNextChar() {
    if (revealed_chars_ < plain_text_.length()) {
        SetRevealedChars(revealed_chars_ + 1);

        if (revealed_chars_ < plain_text_.length() &&
            (plain_text_[revealed_chars_] == ' ' || plain_text_[revealed_chars_] == '\n')) {
            text_timer_->setInterval(60);
        } else {
            text_timer_->setInterval(30);
        }

        if (QRandomGenerator::global()->bounded(100) < 5) {
            TriggerGlitch();
        }
    }
}

void TextDisplayEffect::RandomGlitch() {
    if (QRandomGenerator::global()->bounded(100) < 3) {
        TriggerGlitch();
    } else {
        if (glitch_intensity_ > 0.0) {
            SetGlitchIntensity(qMax(0.0, glitch_intensity_ - 0.05));
        }
    }
}

void TextDisplayEffect::TriggerGlitch() {
    const auto glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    glitch_animation->setDuration(200);
    glitch_animation->setStartValue(0.2);
    glitch_animation->setEndValue(0.0);
    glitch_animation->setEasingCurve(QEasingCurve::OutQuad);
    glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QString TextDisplayEffect::RemoveHtml(const QString &html) {
    QString text = html;
    text.remove(QRegExp("<[^>]*>")); // removes html tags
    text = text.simplified(); // removes whitespaces

    // handling special characters
    text.replace("&nbsp;", " ");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&amp;", "&");

    return text;
}

void TextDisplayEffect::RecalculateHeight() {
    const QFontMetrics font_metrics(font_);
    const int text_width = qMax(300, width() - 30);

    if (font_metrics.horizontalAdvance(plain_text_) <= text_width) {
        const int required_height = font_metrics.lineSpacing() + 20;
        setMinimumHeight(required_height);
        return;
    }

    // text wrapping
    QStringList words = plain_text_.split(' ', Qt::SkipEmptyParts);
    QString wrapped_text;
    QString current_line;

    for (const QString &word: words) {
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

    plain_text_ = wrapped_text;

    const int line_count = plain_text_.count('\n') + 1;
    const int height = line_count * font_metrics.lineSpacing() + 20;

    setMinimumHeight(height);
}
