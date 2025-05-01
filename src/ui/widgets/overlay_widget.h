#ifndef OVERLAY_WIDGET_H
#define OVERLAY_WIDGET_H
#include <QWidget>


class OverlayWidget final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit OverlayWidget(QWidget *parent = nullptr);
    void UpdateGeometry(const QRect& rect);
    qreal GetOpacity() const { return opacity_; }
    void SetOpacity(qreal opacity);

private:
    qreal opacity_;
    QRect exclude_rect_;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};



#endif //OVERLAY_WIDGET_H
