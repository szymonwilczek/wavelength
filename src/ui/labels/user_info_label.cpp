#include "user_info_label.h"

#include <QPainter>
#include <QStyleOption>

UserInfoLabel::UserInfoLabel(QWidget *parent): QLabel(parent),
                                               circle_diameter_(10), // Średnica głównego okręgu
                                               pen_width_(2.0),      // Grubość linii okręgu
                                               glow_layers_(4),      // Liczba warstw poświaty
                                               glow_spread_(2.0),    // Jak bardzo rozszerza się poświata na warstwę
                                               shape_padding_(5) {
    // Oblicz całkowity rozmiar potrzebny na kształt z poświatą
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

    // 1. Narysuj tło (bez zmian)
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // 2. Narysuj okrąg z poświatą
    if (shape_color_.isValid()) {
        painter.save();

        // Wyśrodkuj obszar rysowania kształtu pionowo i umieść po lewej
        qreal totalPadding = glow_layers_ * glow_spread_; // Dodatkowy padding na poświatę
        qreal yPos = (height() - total_shape_size_) / 2.0;
        QRectF baseRect(contentsMargins().left() + totalPadding, // Zostaw miejsce na poświatę po lewej
                        yPos + totalPadding,                   // Zostaw miejsce na poświatę na górze
                        circle_diameter_, circle_diameter_);

        // Rysowanie poświaty (od zewnątrz do wewnątrz)
        for (int i = glow_layers_; i >= 1; --i) {
            qreal currentSpread = i * glow_spread_;
            qreal currentDiameter = circle_diameter_ + currentSpread * 2;
            QRectF glowRect(baseRect.center().x() - currentDiameter / 2.0,
                            baseRect.center().y() - currentDiameter / 2.0,
                            currentDiameter, currentDiameter);

            // Kolor poświaty - ten sam co główny, ale z niską alfą
            QColor glowColor = shape_color_;
            // Alfa maleje im dalej od środka (np. liniowo lub potęgowo)
            int alpha = qMax(0, 50 - i * (50 / glow_layers_)); // Prosty spadek liniowy
            glowColor.setAlpha(alpha);

            // Grubość linii poświaty może być większa
            qreal glowPenWidth = pen_width_ + currentSpread * 0.5; // Grubsza dla większego rozmycia

            QPen glowPen(glowColor, glowPenWidth);
            glowPen.setCapStyle(Qt::RoundCap); // Zaokrąglone końce dla gładszej poświaty
            painter.setPen(glowPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(glowRect);
        }

        // Rysowanie głównego okręgu
        QPen mainPen(shape_color_, pen_width_);
        painter.setPen(mainPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(baseRect);

        painter.restore();
    }

    // 3. Narysuj tekst obok kształtu (bez zmian w logice rysowania tekstu)
    QRect textRect = contentsRect();
    // Przesuń tekst w prawo o całkowity rozmiar kształtu i padding
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
