#ifndef CYBER_TEXT_DISPLAY_H
#define CYBER_TEXT_DISPLAY_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QFontDatabase>

class CyberTextDisplay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int revealedChars READ revealedChars WRITE setRevealedChars)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    
public:
    CyberTextDisplay(const QString& text, QWidget* parent = nullptr)
        : QWidget(parent), m_fullText(text), m_revealedChars(0), 
          m_glitchIntensity(0.0), m_isFullyRevealed(false), m_hasBeenFullyRevealedOnce(false)
    {
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
        
        // Policz potrzebną wysokość
        recalculateHeight();
    }
    
    void startReveal() {
        if (m_hasBeenFullyRevealedOnce) {
            // Jeśli animacja już się odbyła, pokaż cały tekst od razu
            setRevealedChars(m_plainText.length()); // Użyj istniejącej metody, aby ustawić znaki i zaktualizować
            m_textTimer->stop(); // Upewnij się, że timer jest zatrzymany
            return; // Nie uruchamiaj timera ponownie
        }

        // Resetuj stan przed rozpoczęciem (tylko jeśli animacja nie była jeszcze wykonana)
        m_revealedChars = 0;
        m_isFullyRevealed = false;
        m_textTimer->stop(); // Zatrzymaj, jeśli już działa
        m_textTimer->start(30); // szybkie pisanie
        update(); // Wymuś odświeżenie, aby pokazać pusty stan początkowy
    }

    void setText(const QString& newText) {
        m_fullText = newText;
        m_plainText = removeHtml(m_fullText);
        m_hasBeenFullyRevealedOnce = false; // Resetuj flagę dla nowej treści
        recalculateHeight(); // Przelicz wysokość dla nowego tekstu
        startReveal();       // Rozpocznij animację od nowa
    }
    
    int revealedChars() const { return m_revealedChars; }
    void setRevealedChars(int chars) {
        m_revealedChars = qMin(chars, m_plainText.length());
        update();

        if (m_revealedChars >= m_plainText.length() && !m_isFullyRevealed) {
            m_isFullyRevealed = true;
            if (!m_hasBeenFullyRevealedOnce) { // Ustaw flagę tylko przy pierwszym razie
                m_hasBeenFullyRevealedOnce = true;
            }
            emit fullTextRevealed();
        }
    }
    
    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity) {
        m_glitchIntensity = intensity;
        update();
    }
    
    void setGlitchEffectEnabled(bool enabled) {
        if (enabled && !m_glitchTimer->isActive()) {
            m_glitchTimer->start(100);
        } else if (!enabled && m_glitchTimer->isActive()) {
            m_glitchTimer->stop();
            m_glitchIntensity = 0.0;
            update();
        }
    }
    
    QSize sizeHint() const override {
        QFontMetrics fm(m_font);
        int width = 300; // minimalna szerokość
        int height = fm.lineSpacing() * (m_plainText.count('\n') + 1) + 20; // margines
        return QSize(width, height);
    }
    
signals:
    void fullTextRevealed();
    
protected:
    void paintEvent(QPaintEvent* event) override {
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
    
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        recalculateHeight();
    }
    
private slots:
    void revealNextChar() {
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
        } else {
            m_textTimer->stop();
            // Sygnał fullTextRevealed jest emitowany w setRevealedChars
        }
    }
    
    void randomGlitch() {
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
    
    void triggerGlitch() {
        // Krótkotrwały efekt zakłóceń
        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(200);
        glitchAnim->setStartValue(0.2);
        glitchAnim->setEndValue(0.0);
        glitchAnim->setEasingCurve(QEasingCurve::OutQuad);
        glitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
private:
    QString removeHtml(const QString& html) {
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
    
    void recalculateHeight() {
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
    
    QString m_fullText;      // Oryginalny tekst z HTML
    QString m_plainText;     // Tekst bez tagów HTML
    int m_revealedChars;     // Liczba widocznych znaków
    qreal m_glitchIntensity; // Intensywność efektu glitch
    bool m_isFullyRevealed;  // Czy cały tekst jest już wyświetlony
    QFont m_font;            // Czcionka tekstu
    QTimer* m_textTimer;     // Timer animacji tekstu
    QTimer* m_glitchTimer;   // Timer dla efektów glitch
    bool m_hasBeenFullyRevealedOnce;
};

#endif // CYBER_TEXT_DISPLAY_H