#include "message_bubble.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QTimer>
#include <cmath>
#include <QScreen>
#include "../../ui/chat/chat_style.h"

MessageBubble::MessageBubble(const QString& message, BubbleType type, QWidget* parent)
    : QFrame(parent)
    , m_type(type)
    , m_scale(0.95)
    , m_position(0, 0)
{
    setObjectName("messageBubble");
    setAttribute(Qt::WA_TranslucentBackground);

    // Konfiguracja layoutu z większymi marginesami, aby uwzględnić "róg" dymka
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Marginesy dla różnych typów dymków
    if (m_type == SentMessage) {
        layout->setContentsMargins(13, 10, 18, 10); // Więcej miejsca po prawej dla "rogu"
    } else if (m_type == ReceivedMessage) {
        layout->setContentsMargins(18, 10, 13, 10); // Więcej miejsca po lewej dla "rogu"
    } else {
        layout->setContentsMargins(13, 10, 13, 10); // Systemowe bez rogu
    }

    layout->setSpacing(3);

    // Etykieta wiadomości
    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setTextFormat(Qt::RichText);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_messageLabel->setOpenExternalLinks(true);

    // Dostosowanie stylu w zależności od typu wiadomości
    switch(m_type) {
        case SentMessage:
            setStyleSheet("background-color: transparent;");
            m_messageLabel->setStyleSheet("color: white; background: transparent;");
            break;
        case ReceivedMessage:
            setStyleSheet("background-color: transparent;");
            m_messageLabel->setStyleSheet("color: white; background: transparent;");
            break;
        case SystemMessage:
            setStyleSheet("background-color: transparent;");
            m_messageLabel->setStyleSheet("color: #ffcc00; font-weight: bold; background: transparent;");
            break;
    }

    layout->addWidget(m_messageLabel);

    // Przygotowanie efektów dla animacji
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    // Optymalizacje renderowania
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

MessageBubble::~MessageBubble()
{
    // Efekt przezroczystości jest własnością tego obiektu, zostanie automatycznie usunięty
}

void MessageBubble::setScale(qreal scale)
{
    if (m_scale != scale) {
        m_scale = scale;
        updateGeometry();
        update();
    }
}

void MessageBubble::setPosition(const QPointF& pos)
{
    if (m_position != pos) {
        m_position = pos;
        move(pos.x(), pos.y());
    }
}

void MessageBubble::startEntryAnimation()
{
    // Animacja podstawowa - będziemy używać bardziej zaawansowanej wersji
    startAdvancedEntryAnimation(false);
}

void MessageBubble::startAdvancedEntryAnimation(bool delayed)
{
    int delay = delayed ? 25 + (rand() % 40) : 0;

    QTimer::singleShot(delay, this, [this]() {
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity");
        opacityAnim->setDuration(350);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        QPropertyAnimation* posAnim = new QPropertyAnimation(this, "pos");
        posAnim->setDuration(400);
        QPointF startPos = pos();
        QPointF endPos = startPos;

        if (m_type == SentMessage) {
            posAnim->setStartValue(QPointF(parentWidget()->width(), startPos.y()));
        } else if (m_type == ReceivedMessage) {
            posAnim->setStartValue(QPointF(-width(), startPos.y()));
        }

        posAnim->setEndValue(endPos);
        posAnim->setEasingCurve(QEasingCurve::OutBack);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(opacityAnim);
        group->addAnimation(posAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

QSize MessageBubble::sizeHint() const
{
    if (m_type == SystemMessage) {
        QFontMetrics metrics(QFont("Arial", 12, QFont::Bold));
        int width = metrics.horizontalAdvance(m_messageLabel->text()) + 20;
        int height = metrics.height() + 10;
        return QSize(width, height);
    }
    // Bazujemy na szerokości nadrzędnego widgetu, ale z ograniczeniem
    int parentWidth = parentWidget() ? parentWidget()->width() - 80 : 400;
    int maxWidth = std::min(parentWidth, 550); // Maksymalna szerokość dymka

    // Różna szerokość w zależności od typu wiadomości
    if (m_type == SentMessage) {
        // Własne wiadomości są węższe
        maxWidth = std::min(maxWidth, 350);
    } else if (m_type == SystemMessage) {
        // Wiadomości systemowe mają średnią szerokość
        maxWidth = std::min(maxWidth, 400);
    }

    // Obliczamy potrzebną wysokość na podstawie tekstu
    QTextDocument doc;
    doc.setHtml(m_messageLabel->text());
    doc.setTextWidth(maxWidth - 30); // Uwzględniamy wewnętrzne marginesy

    int height = doc.size().height() + 20; // Dodajemy marginesy
    int width = std::min(maxWidth, (int)doc.idealWidth() + 35);

    // Minimalny rozmiar dymka
    width = std::max(width, 80);
    height = std::max(height, 40);

    return QSize(width, height);
}

void MessageBubble::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Jeśli to wiadomość systemowa, renderujemy tylko tekst
    if (m_type == SystemMessage) {
        QStyleOption opt;
        opt.initFrom(this);
        painter.setPen(QColor("#ffcc00")); // Kolor tekstu systemowego
        painter.setFont(QFont("Arial", 12, QFont::Bold)); // Pogrubiona czcionka
        painter.drawText(rect(), Qt::AlignCenter, m_messageLabel->text());
        return;
    }

    // Dla innych typów wiadomości rysujemy dymek
    painter.setOpacity(m_opacityEffect->opacity());
    QColor bgColor = (m_type == SentMessage) ? QColor("#0b93f6") : QColor("#262626");
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    QRectF rect(0, 0, width(), height());
    const int cornerRadius = 18;

    if (m_type == SentMessage) {
        path.addRoundedRect(rect.adjusted(0, 0, -6, 0), cornerRadius, cornerRadius);
        QPainterPath tipPath;
        tipPath.moveTo(rect.right() - 6, rect.bottom() - 15);
        tipPath.lineTo(rect.right() + 8, rect.bottom() - 10);
        tipPath.lineTo(rect.right() - 6, rect.bottom() - 5);
        path = path.united(tipPath);
    } else if (m_type == ReceivedMessage) {
        path.addRoundedRect(rect.adjusted(6, 0, 0, 0), cornerRadius, cornerRadius);
        QPainterPath tipPath;
        tipPath.moveTo(rect.left() + 6, rect.bottom() - 15);
        tipPath.lineTo(rect.left() - 8, rect.bottom() - 10);
        tipPath.lineTo(rect.left() + 6, rect.bottom() - 5);
        path = path.united(tipPath);
    }

    painter.drawPath(path);
}

void MessageBubble::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    update(); // Wymuszamy przerysowanie przy zmianie rozmiaru
}