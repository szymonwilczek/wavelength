#ifndef CYBER_LONG_TEXT_DISPLAY_H
#define CYBER_LONG_TEXT_DISPLAY_H

#include <QWidget>
#include <QPainter>
#include <QLinearGradient>
#include <QDateTime>
#include <QFontMetrics>
#include <QDebug>
#include <QScrollBar>
#include <QTimer>

class CyberLongTextDisplay : public QWidget {
    Q_OBJECT

public:
    CyberLongTextDisplay(const QString& text, const QColor& textColor, QWidget* parent = nullptr)
        : QWidget(parent), m_originalText(text), m_textColor(textColor),
          m_scrollPosition(0), m_cachedTextValid(false)
    {
        setMinimumWidth(400);
        setMinimumHeight(60);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // Inicjalizacja czcionki
        m_font = QFont("Consolas", 10);
        m_font.setStyleHint(QFont::Monospace);

        // Timer dla opóźnionego przetwarzania tekstu (przeciwdziała zbyt częstemu odświeżaniu)
        m_updateTimer = new QTimer(this);
        m_updateTimer->setSingleShot(true);
        m_updateTimer->setInterval(50);
        connect(m_updateTimer, &QTimer::timeout, this, &CyberLongTextDisplay::processTextDelayed);

        // Zainicjuj podstawowe przetwarzanie tekstu
        m_updateTimer->start();
    }

    void setText(const QString& text) {
        m_originalText = text;
        m_cachedTextValid = false;
        m_updateTimer->start();
        update();
    }

    void setTextColor(const QColor& color) {
        m_textColor = color;
        update();
    }

    QSize sizeHint() const override {
        return m_sizeHint;
    }

    void setScrollPosition(int position) {
        if (m_scrollPosition != position) {
            m_scrollPosition = position;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        // Upewnij się, że tekst jest przetworzony
        if (!m_cachedTextValid) {
            processText();
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Tło z gradientem
        QLinearGradient bgGradient(0, 0, width(), height());
        bgGradient.setColorAt(0, QColor(5, 10, 15, 150));
        bgGradient.setColorAt(1, QColor(10, 20, 30, 150));
        painter.fillRect(rect(), bgGradient);

        // Ustawienie czcionki
        painter.setFont(m_font);

        // Podstawowe parametry
        QFontMetrics fm(m_font);
        int lineHeight = fm.lineSpacing();
        int topMargin = 10;
        int leftMargin = 15;

        // Oblicz zakres widocznych linii
        int firstVisibleLine = m_scrollPosition / lineHeight;
        int lastVisibleLine = (m_scrollPosition + height()) / lineHeight + 1;

        // Ogranicz do rzeczywistego zakresu
        firstVisibleLine = qMax(0, firstVisibleLine);
        lastVisibleLine = qMin(lastVisibleLine, m_processedLines.size() - 1);

        // Rysujemy tylko widoczne linie tekstu
        painter.setPen(m_textColor);
        for (int i = firstVisibleLine; i <= lastVisibleLine; ++i) {
            int y = topMargin + fm.ascent() + i * lineHeight - m_scrollPosition;
            painter.drawText(leftMargin, y, m_processedLines[i]);
        }

        // Efekt skanowanych linii - tylko w widocznym obszarze
        painter.setOpacity(0.1);
        painter.setPen(QPen(m_textColor, 1, Qt::DotLine));

        for (int i = 0; i < height(); i += 4) {
            painter.drawLine(0, i, width(), i);
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        // Przy zmianie rozmiaru nie przetwarzamy od razu, tylko uruchamiamy timer
        m_cachedTextValid = false;
        m_updateTimer->start();
    }

private slots:
    void processTextDelayed() {
        processText();
        update();
    }

private:
    void processText() {
        // Jeśli tekst jest już przetworzony - pomijamy
        if (m_cachedTextValid) return;

        // Czyścimy poprzednie przetworzone linie
        m_processedLines.clear();

        QFontMetrics fm(m_font);
        int availableWidth = width() - 30; // Margines 15px z każdej strony

        // Dzielimy tekst na akapity (zachowując oryginalne podziały na linię)
        QStringList paragraphs = m_originalText.split("\n", Qt::KeepEmptyParts);

        for (const QString& paragraph : paragraphs) {
            if (paragraph.isEmpty()) {
                // Pusty paragraf - dodajemy pustą linię
                m_processedLines.append("");
                continue;
            }

            // Dzielimy paragraf na słowa
            QStringList words = paragraph.split(" ", Qt::SkipEmptyParts);
            QString currentLine;

            for (const QString& word : words) {
                // Sprawdzamy czy słowo mieści się w aktualnej linii
                QString testLine = currentLine.isEmpty() ? word : currentLine + " " + word;

                if (fm.horizontalAdvance(testLine) <= availableWidth) {
                    // Słowo mieści się - dodajemy je do aktualnej linii
                    currentLine = testLine;
                } else {
                    // Słowo nie mieści się - dodajemy dotychczasową linię i zaczynamy nową
                    if (!currentLine.isEmpty()) {
                        m_processedLines.append(currentLine);
                    }

                    // Sprawdzamy czy pojedyncze słowo nie jest za długie
                    if (fm.horizontalAdvance(word) > availableWidth) {
                        // Słowo jest za długie - musimy je podzielić na części
                        QString part;
                        for (const QChar& c : word) {
                            QString testPart = part + c;
                            if (fm.horizontalAdvance(testPart) <= availableWidth) {
                                part = testPart;
                            } else {
                                m_processedLines.append(part);
                                part = QString(c);
                            }
                        }
                        if (!part.isEmpty()) {
                            currentLine = part;
                        } else {
                            currentLine.clear();
                        }
                    } else {
                        // Słowo mieści się w nowej linii
                        currentLine = word;
                    }
                }
            }

            // Dodajemy ostatnią linię paragrafu (jeśli istnieje)
            if (!currentLine.isEmpty()) {
                m_processedLines.append(currentLine);
            }
        }

        // Obliczamy wymaganą wysokość
        int requiredHeight = m_processedLines.size() * fm.lineSpacing() + 20; // 10px margines góra i dół

        // Aktualizujemy sizeHint
        m_sizeHint = QSize(qMax(availableWidth + 30, 400), requiredHeight);

        // Oznaczamy, że cache jest aktualny
        m_cachedTextValid = true;

        // Informujemy o całkowitej wysokości zawartości
        emit contentHeightChanged(requiredHeight);
    }

signals:
    void contentHeightChanged(int newHeight);

private:
    QString m_originalText;
    QStringList m_processedLines;
    QColor m_textColor;
    QFont m_font;
    QSize m_sizeHint;
    int m_scrollPosition;
    bool m_cachedTextValid;
    QTimer *m_updateTimer;
};

#endif // CYBER_LONG_TEXT_DISPLAY_H