#ifndef USER_INFO_LABEL_H
#define USER_INFO_LABEL_H

#include <QLabel>
#include <QPainter>
#include <QColor>
#include <QStyleOption>
#include <QtMath>

class UserInfoLabel final : public QLabel {
    Q_OBJECT

public:
    explicit UserInfoLabel(QWidget* parent = nullptr);

    // void setShape(const QPainterPath& path); // USUNIĘTO

    void setShapeColor(const QColor& color);

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

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