#include "user_info_label.h"

#include <QPainter>
#include <QStyleOption>

UserInfoLabel::UserInfoLabel(QWidget *parent): QLabel(parent),
                                               circle_diameter_(10),
                                               pen_width_(2.0),
                                               glow_layers_(4),
                                               glow_spread_(2.0),
                                               shape_padding_(5) {
    total_shape_size_ = circle_diameter_ + glow_layers_ * glow_spread_ * 2;
}

void UserInfoLabel::SetShapeColor(const QColor &color) {
    shape_color_ = color;
    update();
}

QSize UserInfoLabel::sizeHint() const {
    QSize textSize = QLabel::sizeHint();
    textSize.setWidth(textSize.width() + total_shape_size_ + shape_padding_);
    textSize.setHeight(qMax(textSize.height(), total_shape_size_ + 2));
    return textSize;
}

QSize UserInfoLabel::minimumSizeHint() const {
    QSize textSize = QLabel::minimumSizeHint();
    textSize.setWidth(textSize.width() + total_shape_size_ + shape_padding_);
    textSize.setHeight(qMax(textSize.height(), total_shape_size_ + 2));
    return textSize;
}

void UserInfoLabel::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. background
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // 2. circle with a glow
    if (shape_color_.isValid()) {
        painter.save();

        qreal totalPadding = glow_layers_ * glow_spread_;
        qreal yPos = (height() - total_shape_size_) / 2.0;
        QRectF baseRect(contentsMargins().left() + totalPadding,
                        yPos + totalPadding,
                        circle_diameter_, circle_diameter_);

        for (int i = glow_layers_; i >= 1; --i) {
            qreal currentSpread = i * glow_spread_;
            qreal currentDiameter = circle_diameter_ + currentSpread * 2;
            QRectF glowRect(baseRect.center().x() - currentDiameter / 2.0,
                            baseRect.center().y() - currentDiameter / 2.0,
                            currentDiameter, currentDiameter);

            QColor glowColor = shape_color_;
            int alpha = qMax(0, 50 - i * (50 / glow_layers_));
            glowColor.setAlpha(alpha);

            qreal glowPenWidth = pen_width_ + currentSpread * 0.5;

            QPen glowPen(glowColor, glowPenWidth);
            glowPen.setCapStyle(Qt::RoundCap);
            painter.setPen(glowPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(glowRect);
        }

        QPen mainPen(shape_color_, pen_width_);
        painter.setPen(mainPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(baseRect);

        painter.restore();
    }

    // 3. draw text next to the shape
    QRect textRect = contentsRect();
    textRect.setLeft(textRect.left() + total_shape_size_ + shape_padding_);

    QColor textColor = palette().color(QPalette::WindowText);
    QString styleSheet = this->styleSheet();
    int colorIndex = styleSheet.indexOf("color:");
    if (colorIndex != -1) {
        int endColorIndex = styleSheet.indexOf(';', colorIndex);
        if (endColorIndex != -1) {
            QString colorStr = styleSheet.mid(colorIndex + 6, endColorIndex - (colorIndex + 6)).trimmed();
            QColor ssColor(colorStr);
            if (ssColor.isValid()) {
                textColor = ssColor;
            }
        }
    }
    painter.setPen(textColor);
    Qt::Alignment alignment = this->alignment();
    painter.drawText(textRect, alignment, text());
}
