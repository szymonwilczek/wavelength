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
    CyberLongTextDisplay(const QString& text, const QColor& textColor, QWidget* parent = nullptr);

    void setText(const QString& text);

    void setTextColor(const QColor& color);

    QSize sizeHint() const override;

    void setScrollPosition(int position);

protected:
    void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private slots:
    void processTextDelayed();

private:
    void processText();

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