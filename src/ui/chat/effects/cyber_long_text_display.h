#ifndef CYBER_LONG_TEXT_DISPLAY_H
#define CYBER_LONG_TEXT_DISPLAY_H

#include <QPainter>
#include <QDateTime>
#include <QFontMetrics>
#include <QDebug>
#include <QScrollBar>
#include <QTimer>

class CyberLongTextDisplay final : public QWidget {
    Q_OBJECT

public:
    CyberLongTextDisplay(const QString& text, const QColor& text_color, QWidget* parent = nullptr);

    void SetText(const QString& text);

    void SetTextColor(const QColor& color);

    QSize sizeHint() const override;

    void SetScrollPosition(int position);

protected:
    void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private slots:
    void ProcessTextDelayed();

private:
    void ProcessText();

signals:
    void contentHeightChanged(int new_height);

private:
    QString original_text_;
    QStringList processed_lines_;
    QColor text_color_;
    QFont font_;
    QSize size_hint_;
    int scroll_position_;
    bool cached_text_valid_;
    QTimer *update_timer_;
};

#endif // CYBER_LONG_TEXT_DISPLAY_H