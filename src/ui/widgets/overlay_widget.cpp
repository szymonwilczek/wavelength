#include "overlay_widget.h"

#include "../navigation/navbar.h"

OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent)
    , m_opacity(0.0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    // Wyłączamy niektóre flagi, które powodowały konflikt
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_StaticContents, false);

    // Instalujemy filtr zdarzeń na rodzicu
    if (parent) {
        parent->installEventFilter(this);
    }

    // Znajdź Navbar i zapisz jego geometrię
    QList<Navbar*> navbars = parent->findChildren<Navbar*>();
    if (!navbars.isEmpty()) {
        m_excludeRect = navbars.first()->geometry();
    }
}

void OverlayWidget::setOpacity(qreal opacity)
{
    if (m_opacity != opacity) {
        m_opacity = opacity;
        update();
    }
}

bool OverlayWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parent()) {
        if (event->type() == QEvent::Resize) {
            QResizeEvent *resizeEvent = dynamic_cast<QResizeEvent*>(event);
            updateGeometry(QRect(QPoint(0,0), resizeEvent->size()));
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OverlayWidget::updateGeometry(const QRect& rect)
{
    setGeometry(rect);
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Używamy kompozycji dla płynnego przejścia
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Rysujemy tło z wyłączeniem obszaru nawigacji
    QRegion region = rect();
    if (!m_excludeRect.isNull()) {
        region = region.subtracted(QRegion(m_excludeRect));
    }

    painter.setClipRegion(region);
    painter.fillRect(rect(), QColor(0, 0, 0, m_opacity * 120));
}
