#ifndef OVERLAY_WIDGET_H
#define OVERLAY_WIDGET_H
#include <QWidget>


class OverlayWidget final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit OverlayWidget(QWidget *parent = nullptr);
    void updateGeometry(const QRect& rect);
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

private:
    qreal m_opacity;
    QRect m_excludeRect;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};



#endif //OVERLAY_WIDGET_H
