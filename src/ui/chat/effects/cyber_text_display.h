#ifndef CYBER_TEXT_DISPLAY_H
#define CYBER_TEXT_DISPLAY_H

#include <QAudioOutput>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QFontDatabase>
#include <QSoundEffect>
#include <QDebug>
#include <QMediaPlayer>
#include <QMediaPlaylist>


class CyberTextDisplay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int revealedChars READ revealedChars WRITE setRevealedChars)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)

public:

    enum TypingSoundType {
        SystemSound, // terminal_typing1.wav
        UserSound    // terminal_typing2.wav
    };
    Q_ENUM(TypingSoundType)

    CyberTextDisplay(const QString& text, TypingSoundType soundType = UserSound, QWidget* parent = nullptr);

    ~CyberTextDisplay() override;

    void startReveal();

    void setText(const QString& newText);

    int revealedChars() const { return m_revealedChars; }
    void setRevealedChars(int chars);

    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity);

    void setGlitchEffectEnabled(bool enabled);

    QSize sizeHint() const override;

signals:
    void fullTextRevealed();

protected:
    void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void hideEvent(QHideEvent *event) override;

private slots:

    void handleFullTextRevealed() const;

    void revealNextChar();

    void randomGlitch();

    void triggerGlitch();

private:
    QString removeHtml(const QString& html);

    void recalculateHeight();

    QString m_fullText;      // Oryginalny tekst z HTML
    QString m_plainText;     // Tekst bez tagów HTML
    int m_revealedChars;     // Liczba widocznych znaków
    qreal m_glitchIntensity; // Intensywność efektu glitch
    bool m_isFullyRevealed;  // Czy cały tekst jest już wyświetlony
    QFont m_font;            // Czcionka tekstu
    QTimer* m_textTimer;     // Timer animacji tekstu
    QTimer* m_glitchTimer;   // Timer dla efektów glitch
    bool m_hasBeenFullyRevealedOnce;
    QMediaPlayer* m_mediaPlayer;   // Dodane
    QAudioOutput* m_audioOutput;   // Dodane
    QMediaPlaylist* m_playlist;
    TypingSoundType m_soundType;
};

#endif // CYBER_TEXT_DISPLAY_H