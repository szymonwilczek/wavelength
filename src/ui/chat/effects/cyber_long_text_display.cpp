#include "cyber_long_text_display.h"

#include <QPainter>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QDebug>
#include <utility>

CyberLongTextDisplay::CyberLongTextDisplay(QString text, const QColor &text_color, QWidget *parent): QWidget(parent),
    original_text_(std::move(text)), text_color_(text_color),
    scroll_position_(0), cached_text_valid_(false) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    font_ = QFont("Consolas", 10);
    font_.setStyleHint(QFont::Monospace);

    ProcessText();
}

void CyberLongTextDisplay::SetText(const QString &text) {
    if (original_text_ == text) return;
    original_text_ = text;
    cached_text_valid_ = false;
    ProcessText();
    update();
}

void CyberLongTextDisplay::SetTextColor(const QColor &color) {
    if (text_color_ == color) return;
    text_color_ = color;
    update();
}

QSize CyberLongTextDisplay::sizeHint() const {
    if (!cached_text_valid_) {
        const_cast<CyberLongTextDisplay *>(this)->ProcessText();
    }
    return size_hint_;
}

QSize CyberLongTextDisplay::minimumSizeHint() const {
    return sizeHint();
}


void CyberLongTextDisplay::SetScrollPosition(const int position) {
    if (scroll_position_ != position) {
        scroll_position_ = position;
        update();
    }
}

void CyberLongTextDisplay::paintEvent(QPaintEvent *event) {
    if (!cached_text_valid_) {
        ProcessText();
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient background_gradient(0, 0, 0, height());
    background_gradient.setColorAt(0, QColor(5, 10, 15, 150));
    background_gradient.setColorAt(1, QColor(10, 20, 30, 150));
    painter.fillRect(rect(), background_gradient);

    painter.setFont(font_);

    const QFontMetrics font_metrics(font_);
    const int line_height = font_metrics.lineSpacing();
    constexpr int topMargin = 10;

    int first_visible_line = scroll_position_ / line_height;
    int last_visible_line = (scroll_position_ + height() - topMargin) / line_height;

    first_visible_line = qMax(0, first_visible_line);
    last_visible_line = qMin(last_visible_line, processed_lines_.size() - 1);

    painter.setPen(text_color_);
    for (int i = first_visible_line; i <= last_visible_line; ++i) {
        const int y = topMargin + font_metrics.ascent() + i * line_height - scroll_position_;
        if (y > -line_height && y < height() + line_height) {
            constexpr int leftMargin = 15;
            painter.drawText(leftMargin, y, processed_lines_[i]);
        }
    }

    painter.setOpacity(0.1);
    painter.setPen(QPen(text_color_, 1, Qt::DotLine));
    for (int y_scan = 0; y_scan < height(); y_scan += 4) {
        painter.drawLine(0, y_scan, width(), y_scan);
    }
}

void CyberLongTextDisplay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    cached_text_valid_ = false;
    ProcessText();
}

void CyberLongTextDisplay::ProcessText() {
    if (cached_text_valid_) return;

    processed_lines_.clear();

    const QFontMetrics fm(font_);
    const int available_width = qMax(1, width() - 30);
    int max_line_width = 0;

    QStringList paragraphs = original_text_.split("\n", Qt::KeepEmptyParts);

    for (const QString &paragraph: paragraphs) {
        if (paragraph.isEmpty()) {
            processed_lines_.append("");
            continue;
        }

        QStringList words = paragraph.split(" ", Qt::SkipEmptyParts);
        QString current_line;

        for (const QString &word: words) {
            QString test_line = current_line.isEmpty() ? word : current_line + " " + word;
            int current_line_width = fm.horizontalAdvance(test_line);

            if (current_line_width <= available_width) {
                current_line = test_line;
            } else {
                if (!current_line.isEmpty()) {
                    processed_lines_.append(current_line);
                    max_line_width = qMax(max_line_width, fm.horizontalAdvance(current_line));
                }

                const int word_width = fm.horizontalAdvance(word);
                if (word_width > available_width) {
                    QString part;
                    for (const QChar &c: word) {
                        QString test_part = part + c;
                        const int part_width = fm.horizontalAdvance(test_part);
                        if (part_width <= available_width) {
                            part = test_part;
                        } else {
                            processed_lines_.append(part);
                            max_line_width = qMax(max_line_width, fm.horizontalAdvance(part));
                            part = QString(c);
                        }
                    }
                    current_line = part;
                } else {
                    current_line = word;
                }
            }
        }

        if (!current_line.isEmpty()) {
            processed_lines_.append(current_line);
            max_line_width = qMax(max_line_width, fm.horizontalAdvance(current_line));
        }
    }

    const int line_height = fm.lineSpacing();
    const int required_height = processed_lines_.size() * line_height + 20;
    const int required_width = max_line_width + 30;

    const QSize new_size_hint(required_width, required_height);
    if (size_hint_ != new_size_hint) {
        size_hint_ = new_size_hint;
        updateGeometry();
    }

    cached_text_valid_ = true;
    emit contentHeightChanged(required_height);
}
