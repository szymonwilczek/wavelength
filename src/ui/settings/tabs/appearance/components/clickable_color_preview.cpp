#include "clickable_color_preview.h"

#include <QMouseEvent>
#include <QPainter>

ClickableColorPreview::ClickableColorPreview(QWidget *parent): QWidget(parent) {
    setCursor(Qt::PointingHandCursor);
    setFixedSize(24, 24);
    setAutoFillBackground(true);
}

void ClickableColorPreview::SetColor(const QColor &color) {
    QPalette pal = palette();
    if (color.isValid()) {
        pal.setColor(QPalette::Window, color);
    } else {
        pal.setColor(QPalette::Window, Qt::transparent);
    }
    setPalette(pal);

    update();
}

void ClickableColorPreview::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void ClickableColorPreview::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);

    const QColor background_color = palette().color(QPalette::Window);

    if (background_color.isValid() && background_color.alpha() > 0) {
        painter.fillRect(rect(), background_color);
    }

    painter.setPen(QPen(QColor("#555555"), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}
