#include "overlay_widget.h"

#include "../navigation/navbar.h"

OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent), opacity_(0.0) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_StaticContents, false);

    if (parent) {
        parent->installEventFilter(this);
        QList<Navbar *> navbars = parent->findChildren<Navbar *>();
        if (!navbars.isEmpty()) {
            exclude_rect_ = navbars.first()->geometry();
        }
    }
}

void OverlayWidget::SetOpacity(const qreal opacity) {
    if (opacity_ != opacity) {
        opacity_ = opacity;
        update();
    }
}

bool OverlayWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parent()) {
        if (event->type() == QEvent::Resize) {
            const auto resizeEvent = dynamic_cast<QResizeEvent *>(event);
            UpdateGeometry(QRect(QPoint(0, 0), resizeEvent->size()));
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OverlayWidget::UpdateGeometry(const QRect &rect) {
    setGeometry(rect);
}

void OverlayWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QRegion region = rect();
    if (!exclude_rect_.isNull()) {
        region = region.subtracted(QRegion(exclude_rect_));
    }

    painter.setClipRegion(region);
    painter.fillRect(rect(), QColor(0, 0, 0, opacity_ * 120));
}
