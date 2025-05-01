#ifndef CYBER_TEXT_DISPLAY_H
#define CYBER_TEXT_DISPLAY_H

#include <QAudioOutput>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QFontDatabase>
#include <QMediaPlayer>
#include <QMediaPlaylist>


class CyberTextDisplay final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int revealedChars READ revealedChars WRITE setRevealedChars)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)

public:

    enum TypingSoundType {
        kSystemSound, // terminal_typing1.wav
        kUserSound    // terminal_typing2.wav
    };
    Q_ENUM(TypingSoundType)

    explicit CyberTextDisplay(const QString& text, TypingSoundType sound_type = kUserSound, QWidget* parent = nullptr);

    ~CyberTextDisplay() override;

    void StartReveal();

    void SetText(const QString& new_text);

    int GetRevealedChars() const { return revealed_chars_; }
    void SetRevealedChars(int chars);

    qreal GetGlitchIntensity() const { return glitch_intensity_; }
    void SetGlitchIntensity(qreal intensity);

    void SetGlitchEffectEnabled(bool enabled);

    QSize sizeHint() const override;

signals:
    void fullTextRevealed();

protected:
    void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void hideEvent(QHideEvent *event) override;

private slots:

    void HandleFullTextRevealed() const;

    void RevealNextChar();

    void RandomGlitch();

    void TriggerGlitch();

private:
    static QString RemoveHtml(const QString& html);

    void RecalculateHeight();

    QString full_text_;      // Oryginalny tekst z HTML
    QString plain_text_;     // Tekst bez tagów HTML
    int revealed_chars_;     // Liczba widocznych znaków
    qreal glitch_intensity_; // Intensywność efektu glitch
    bool is_fully_revealed_;  // Czy cały tekst jest już wyświetlony
    QFont font_;            // Czcionka tekstu
    QTimer* text_timer_;     // Timer animacji tekstu
    QTimer* glitch_timer_;   // Timer dla efektów glitch
    bool has_been_fully_revealed_once_;
    QMediaPlayer* media_player_;   // Dodane
    QAudioOutput* audio_output_;   // Dodane
    QMediaPlaylist* playlist_;
    TypingSoundType sound_type_;
};

#endif // CYBER_TEXT_DISPLAY_H