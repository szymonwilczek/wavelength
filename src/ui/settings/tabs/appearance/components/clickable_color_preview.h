#ifndef CLICKABLE_COLOR_PREVIEW_H
#define CLICKABLE_COLOR_PREVIEW_H

#include <QMouseEvent>
#include <QString>
#include <QDebug>
#include <QStyleOption>
#include <QPainter>

class ClickableColorPreview final : public QWidget {
    Q_OBJECT

public:
    explicit ClickableColorPreview(QWidget *parent = nullptr);

public slots:
    void SetColor(const QColor& color);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

};

#endif //CLICKABLE_COLOR_PREVIEW_H