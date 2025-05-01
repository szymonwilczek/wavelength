#ifndef USER_INFO_LABEL_H
#define USER_INFO_LABEL_H

#include <QLabel>
#include <QColor>
#include <QtMath>

class UserInfoLabel final : public QLabel {
    Q_OBJECT

public:
    explicit UserInfoLabel(QWidget* parent = nullptr);

    void SetShapeColor(const QColor& color);

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor shape_color_;
    int circle_diameter_;   // Średnica głównego okręgu
    qreal pen_width_;       // Grubość linii okręgu
    int glow_layers_;       // Liczba warstw poświaty
    qreal glow_spread_;     // Rozszerzenie poświaty na warstwę
    int shape_padding_;     // Odstęp między kształtem a tekstem
    int total_shape_size_;   // Całkowity rozmiar (średnica + poświata)
};

#endif // USER_INFO_LABEL_H