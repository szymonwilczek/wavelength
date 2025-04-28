#ifndef USER_INFO_LABEL_H
#define USER_INFO_LABEL_H

#include <QLabel>
#include <QPainter>
// #include <QPainterPath> // Już niepotrzebne
#include <QColor>
#include <QStyleOption>
#include <QtMath> // Dla qMax

class UserInfoLabel : public QLabel {
    Q_OBJECT

public:
    explicit UserInfoLabel(QWidget* parent = nullptr)
        : QLabel(parent),
          m_circleDiameter(10), // Średnica głównego okręgu
          m_penWidth(2.0),      // Grubość linii okręgu
          m_glowLayers(4),      // Liczba warstw poświaty
          m_glowSpread(2.0),    // Jak bardzo rozszerza się poświata na warstwę
          m_shapePadding(5)     // Odstęp między okręgiem (z poświatą) a tekstem
    {
        // Oblicz całkowity rozmiar potrzebny na kształt z poświatą
        m_totalShapeSize = m_circleDiameter + m_glowLayers * m_glowSpread * 2;
    }

    // void setShape(const QPainterPath& path); // USUNIĘTO

    void setShapeColor(const QColor& color) {
        m_shapeColor = color;
        update();
    }

    QSize sizeHint() const override {
        QSize textSize = QLabel::sizeHint();
        textSize.setWidth(textSize.width() + m_totalShapeSize + m_shapePadding);
        textSize.setHeight(qMax(textSize.height(), m_totalShapeSize + 2));
        return textSize;
    }

    QSize minimumSizeHint() const override {
        QSize textSize = QLabel::minimumSizeHint();
        textSize.setWidth(textSize.width() + m_totalShapeSize + m_shapePadding);
        textSize.setHeight(qMax(textSize.height(), m_totalShapeSize + 2));
        return textSize;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 1. Narysuj tło (bez zmian)
        QStyleOption opt;
        opt.initFrom(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

        // 2. Narysuj okrąg z poświatą
        if (m_shapeColor.isValid()) {
            painter.save();

            // Wyśrodkuj obszar rysowania kształtu pionowo i umieść po lewej
            qreal totalPadding = m_glowLayers * m_glowSpread; // Dodatkowy padding na poświatę
            qreal yPos = (height() - m_totalShapeSize) / 2.0;
            QRectF baseRect(contentsMargins().left() + totalPadding, // Zostaw miejsce na poświatę po lewej
                            yPos + totalPadding,                   // Zostaw miejsce na poświatę na górze
                            m_circleDiameter, m_circleDiameter);

            // Rysowanie poświaty (od zewnątrz do wewnątrz)
            for (int i = m_glowLayers; i >= 1; --i) {
                qreal currentSpread = i * m_glowSpread;
                qreal currentDiameter = m_circleDiameter + currentSpread * 2;
                QRectF glowRect(baseRect.center().x() - currentDiameter / 2.0,
                                baseRect.center().y() - currentDiameter / 2.0,
                                currentDiameter, currentDiameter);

                // Kolor poświaty - ten sam co główny, ale z niską alfą
                QColor glowColor = m_shapeColor;
                // Alfa maleje im dalej od środka (np. liniowo lub potęgowo)
                int alpha = qMax(0, 50 - i * (50 / m_glowLayers)); // Prosty spadek liniowy
                glowColor.setAlpha(alpha);

                // Grubość linii poświaty może być większa
                qreal glowPenWidth = m_penWidth + currentSpread * 0.5; // Grubsza dla większego rozmycia

                QPen glowPen(glowColor, glowPenWidth);
                glowPen.setCapStyle(Qt::RoundCap); // Zaokrąglone końce dla gładszej poświaty
                painter.setPen(glowPen);
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(glowRect);
            }

            // Rysowanie głównego okręgu
            QPen mainPen(m_shapeColor, m_penWidth);
            painter.setPen(mainPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(baseRect);

            painter.restore();
        }

        // 3. Narysuj tekst obok kształtu (bez zmian w logice rysowania tekstu)
        QRect textRect = contentsRect();
        // Przesuń tekst w prawo o całkowity rozmiar kształtu i padding
        textRect.setLeft(textRect.left() + m_totalShapeSize + m_shapePadding);

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

private:
    // QPainterPath m_shape; // USUNIĘTO
    QColor m_shapeColor;
    int m_circleDiameter;   // Średnica głównego okręgu
    qreal m_penWidth;       // Grubość linii okręgu
    int m_glowLayers;       // Liczba warstw poświaty
    qreal m_glowSpread;     // Rozszerzenie poświaty na warstwę
    int m_shapePadding;     // Odstęp między kształtem a tekstem
    int m_totalShapeSize;   // Całkowity rozmiar (średnica + poświata)
};

#endif // USER_INFO_LABEL_H