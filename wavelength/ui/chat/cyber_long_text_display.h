#ifndef CYBER_LONG_TEXT_DISPLAY_H
#define CYBER_LONG_TEXT_DISPLAY_H

#include <QWidget>
#include <QPainter>
#include <QLinearGradient>
#include <QDateTime>
#include <QFontMetrics>
#include <QDebug>

class CyberLongTextDisplay : public QWidget {
    Q_OBJECT

public:
    CyberLongTextDisplay(const QString& text, const QColor& textColor, QWidget* parent = nullptr)
        : QWidget(parent), m_originalText(text), m_textColor(textColor)
    {
        setMinimumWidth(400);
        setMinimumHeight(60);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // Inicjalizacja czcionki (taka sama jak w CyberTextDisplay)
        m_font = QFont("Consolas", 10);
        m_font.setStyleHint(QFont::Monospace);

        // Przetwarzamy tekst - dzielimy na linie respektując oryginalne podziały
        processText();
    }

    void setText(const QString& text) {
        m_originalText = text;
        processText();
        update();
    }

    void setTextColor(const QColor& color) {
        m_textColor = color;
        update();
    }

    QSize sizeHint() const override {
        return m_sizeHint;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Tło z gradientem (takie samo jak w CyberTextDisplay)
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

        // Rysujemy przetworzone linie tekstu
        painter.setPen(m_textColor);
        int y = topMargin + fm.ascent();

        for (const QString& line : m_processedLines) {
            painter.drawText(leftMargin, y, line);
            y += lineHeight;
        }

        // Efekt skanowanych linii (jak w CyberTextDisplay)
        painter.setOpacity(0.1);
        painter.setPen(QPen(m_textColor, 1, Qt::DotLine));

        for (int i = 0; i < height(); i += 4) {
            painter.drawLine(0, i, width(), i);
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        // Przy zmianie rozmiaru przetwarzamy ponownie tekst
        processText();
    }

private:
    void processText() {
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
        setMinimumHeight(requiredHeight);

        // Debug info
        qDebug() << "CyberLongTextDisplay: Processed" << m_processedLines.size()
                 << "lines, required height:" << requiredHeight
                 << "available width:" << availableWidth;
    }

    QString m_originalText;
    QStringList m_processedLines;
    QColor m_textColor;
    QFont m_font;
    QSize m_sizeHint;
};

#endif // CYBER_LONG_TEXT_DISPLAY_H