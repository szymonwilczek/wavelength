#include "cyber_long_text_display.h"

CyberLongTextDisplay::CyberLongTextDisplay(const QString &text, const QColor &text_color, QWidget *parent): QWidget(parent), original_text_(text), text_color_(text_color),
    scroll_position_(0), cached_text_valid_(false) {
    setMinimumWidth(400);
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Inicjalizacja czcionki
    font_ = QFont("Consolas", 10);
    font_.setStyleHint(QFont::Monospace);

    // Timer dla opóźnionego przetwarzania tekstu (przeciwdziała zbyt częstemu odświeżaniu)
    update_timer_ = new QTimer(this);
    update_timer_->setSingleShot(true);
    update_timer_->setInterval(50);
    connect(update_timer_, &QTimer::timeout, this, &CyberLongTextDisplay::ProcessTextDelayed);

    // Zainicjuj podstawowe przetwarzanie tekstu
    update_timer_->start();
}

void CyberLongTextDisplay::SetText(const QString &text) {
    original_text_ = text;
    cached_text_valid_ = false;
    update_timer_->start();
    update();
}

void CyberLongTextDisplay::SetTextColor(const QColor &color) {
    text_color_ = color;
    update();
}

QSize CyberLongTextDisplay::sizeHint() const {
    return size_hint_;
}

void CyberLongTextDisplay::SetScrollPosition(const int position) {
    if (scroll_position_ != position) {
        scroll_position_ = position;
        update();
    }
}

void CyberLongTextDisplay::paintEvent(QPaintEvent *event) {
    // Upewnij się, że tekst jest przetworzony
    if (!cached_text_valid_) {
        ProcessText();
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tło z gradientem
    QLinearGradient background_gradient(0, 0, width(), height());
    background_gradient.setColorAt(0, QColor(5, 10, 15, 150));
    background_gradient.setColorAt(1, QColor(10, 20, 30, 150));
    painter.fillRect(rect(), background_gradient);

    // Ustawienie czcionki
    painter.setFont(font_);

    // Podstawowe parametry
    const QFontMetrics font_metrics(font_);
    const int line_height = font_metrics.lineSpacing();

    // Oblicz zakres widocznych linii
    int first_visible_line = scroll_position_ / line_height;
    int last_visible_line = (scroll_position_ + height()) / line_height + 1;

    // Ogranicz do rzeczywistego zakresu
    first_visible_line = qMax(0, first_visible_line);
    last_visible_line = qMin(last_visible_line, processed_lines_.size() - 1);

    // Rysujemy tylko widoczne linie tekstu
    painter.setPen(text_color_);
    for (int i = first_visible_line; i <= last_visible_line; ++i) {
        constexpr int leftMargin = 15;
        constexpr int topMargin = 10;
        const int y = topMargin + font_metrics.ascent() + i * line_height - scroll_position_;
        painter.drawText(leftMargin, y, processed_lines_[i]);
    }

    // Efekt skanowanych linii - tylko w widocznym obszarze
    painter.setOpacity(0.1);
    painter.setPen(QPen(text_color_, 1, Qt::DotLine));

    for (int i = 0; i < height(); i += 4) {
        painter.drawLine(0, i, width(), i);
    }
}

void CyberLongTextDisplay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // Przy zmianie rozmiaru nie przetwarzamy od razu, tylko uruchamiamy timer
    cached_text_valid_ = false;
    update_timer_->start();
}

void CyberLongTextDisplay::ProcessTextDelayed() {
    ProcessText();
    update();
}

void CyberLongTextDisplay::ProcessText() {
    // Jeśli tekst jest już przetworzony - pomijamy
    if (cached_text_valid_) return;

    // Czyścimy poprzednie przetworzone linie
    processed_lines_.clear();

    const QFontMetrics fm(font_);
    const int available_width = width() - 30; // Margines 15px z każdej strony

    // Dzielimy tekst na akapity (zachowując oryginalne podziały na linię)
    QStringList paragraphs = original_text_.split("\n", Qt::KeepEmptyParts);

    for (const QString& paragraph : paragraphs) {
        if (paragraph.isEmpty()) {
            // Pusty paragraf - dodajemy pustą linię
            processed_lines_.append("");
            continue;
        }

        // Dzielimy paragraf na słowa
        QStringList words = paragraph.split(" ", Qt::SkipEmptyParts);
        QString current_line;

        for (const QString& word : words) {
            // Sprawdzamy czy słowo mieści się w aktualnej linii
            QString test_line = current_line.isEmpty() ? word : current_line + " " + word;

            if (fm.horizontalAdvance(test_line) <= available_width) {
                // Słowo mieści się - dodajemy je do aktualnej linii
                current_line = test_line;
            } else {
                // Słowo nie mieści się - dodajemy dotychczasową linię i zaczynamy nową
                if (!current_line.isEmpty()) {
                    processed_lines_.append(current_line);
                }

                // Sprawdzamy czy pojedyncze słowo nie jest za długie
                if (fm.horizontalAdvance(word) > available_width) {
                    // Słowo jest za długie - musimy je podzielić na części
                    QString part;
                    for (const QChar& c : word) {
                        QString test_part = part + c;
                        if (fm.horizontalAdvance(test_part) <= available_width) {
                            part = test_part;
                        } else {
                            processed_lines_.append(part);
                            part = QString(c);
                        }
                    }
                    if (!part.isEmpty()) {
                        current_line = part;
                    } else {
                        current_line.clear();
                    }
                } else {
                    // Słowo mieści się w nowej linii
                    current_line = word;
                }
            }
        }

        // Dodajemy ostatnią linię paragrafu (jeśli istnieje)
        if (!current_line.isEmpty()) {
            processed_lines_.append(current_line);
        }
    }

    // Obliczamy wymaganą wysokość
    const int required_height = processed_lines_.size() * fm.lineSpacing() + 20; // 10px margines góra i dół

    // Aktualizujemy sizeHint
    size_hint_ = QSize(qMax(available_width + 30, 400), required_height);

    // Oznaczamy, że cache jest aktualny
    cached_text_valid_ = true;

    // Informujemy o całkowitej wysokości zawartości
    emit contentHeightChanged(required_height);
}
