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

    // Konfiguracja layoutu
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(13, 10, 13, 10);
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
            setStyleSheet(ChatStyle::getSentMessageStyle());
            m_messageLabel->setStyleSheet("color: white; background: transparent;");
            break;
        case ReceivedMessage:
            setStyleSheet(ChatStyle::getReceivedMessageStyle());
            m_messageLabel->setStyleSheet("color: white; background: transparent;");
            break;
        case SystemMessage:
            setStyleSheet(ChatStyle::getSystemMessageStyle());
            m_messageLabel->setStyleSheet("color: #ffcc00; background: transparent;");
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

    // Inicjalizacja ścieżki dymka
    updateBubblePath();
}

MessageBubble::~MessageBubble()
{
    // Opracityeffect jest własnością tego obiektu, zostanie automatycznie usunięty
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
    // 1. Animacja opacity
    QPropertyAnimation* opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity");
    opacityAnim->setDuration(280);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 2. Animacja powiększania
    QPropertyAnimation* scaleAnim = new QPropertyAnimation(this, "scale");
    scaleAnim->setDuration(320);
    scaleAnim->setStartValue(0.95);
    scaleAnim->setEndValue(1.0);
    scaleAnim->setEasingCurve(QEasingCurve::OutBack);

    // 3. Animacja pojawienia się z boku (zależnie od typu)
    QPropertyAnimation* posAnim = new QPropertyAnimation(this, "position");
    posAnim->setDuration(280);
    QPointF startPos = pos();

    if (m_type == SentMessage) {
        // Wiadomości wysłane wjeżdżają z prawej
        posAnim->setStartValue(QPointF(startPos.x() + 20, startPos.y()));
    } else {
        // Wiadomości odebrane wjeżdżają z lewej
        posAnim->setStartValue(QPointF(startPos.x() - 20, startPos.y()));
    }

    posAnim->setEndValue(startPos);
    posAnim->setEasingCurve(QEasingCurve::OutCubic);

    // Grupa animacji
    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(opacityAnim);
    group->addAnimation(scaleAnim);
    group->addAnimation(posAnim);

    // Rozpocznij animację
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MessageBubble::startAdvancedEntryAnimation(bool delayed)
{
    // Opóźnienie dla efektu kaskadowego (opcjonalne)
    int delay = delayed ? 25 + (rand() % 40) : 0;

    QTimer::singleShot(delay, this, [this]() {
        // 1. Animacja opacity z sekwencją
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity");
        opacityAnim->setDuration(350);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setKeyValueAt(0.3, 0.6);  // 30% animacji - 60% widoczności
        opacityAnim->setKeyValueAt(0.7, 0.9);  // 70% animacji - 90% widoczności
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        // 2. Złożona animacja powiększania z lekkim efektem "bujania"
        QSequentialAnimationGroup* scaleGroup = new QSequentialAnimationGroup(this);

        // Najpierw powiększanie do 1.03 (lekko ponad docelową wartość)
        QPropertyAnimation* scaleUp = new QPropertyAnimation(this, "scale");
        scaleUp->setDuration(260);
        scaleUp->setStartValue(0.93);
        scaleUp->setEndValue(1.03);  // Lekki overshooting
        scaleUp->setEasingCurve(QEasingCurve::OutQuad);

        // Potem zmniejszenie do docelowego rozmiaru
        QPropertyAnimation* scaleDown = new QPropertyAnimation(this, "scale");
        scaleDown->setDuration(180);
        scaleDown->setStartValue(1.03);
        scaleDown->setEndValue(1.0);
        scaleDown->setEasingCurve(QEasingCurve::OutBack);

        scaleGroup->addAnimation(scaleUp);
        scaleGroup->addAnimation(scaleDown);

        // 3. Animacja pojawienia się z kierunku zależnego od typu
        QPropertyAnimation* posAnim = new QPropertyAnimation(this, "position");
        posAnim->setDuration(320);
        QPointF startPos = pos();

        switch (m_type) {
            case SentMessage: {
                // Wiadomości wysłane wjeżdżają z prawej i lekko z dołu
                posAnim->setStartValue(QPointF(startPos.x() + 30, startPos.y() + 8));
                posAnim->setKeyValueAt(0.6, QPointF(startPos.x() + 3, startPos.y() + 1));
                break;
            }
            case ReceivedMessage: {
                // Wiadomości odebrane wjeżdżają z lewej
                posAnim->setStartValue(QPointF(startPos.x() - 25, startPos.y()));
                posAnim->setKeyValueAt(0.6, QPointF(startPos.x() - 3, startPos.y()));
                break;
            }
            case SystemMessage: {
                // Wiadomości systemowe pojawiają się z góry
                posAnim->setStartValue(QPointF(startPos.x(), startPos.y() - 15));
                posAnim->setKeyValueAt(0.7, QPointF(startPos.x(), startPos.y() - 2));
                break;
            }
        }

        posAnim->setEndValue(startPos);
        posAnim->setEasingCurve(QEasingCurve::OutQuint);

        // Grupa animacji
        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(opacityAnim);
        group->addAnimation(scaleGroup);
        group->addAnimation(posAnim);

        // Rozpocznij animację
        group->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

QSize MessageBubble::sizeHint() const
{
    // Bazujemy na szerokości nadrzędnego widgetu, ale z ograniczeniem
    int parentWidth = parentWidget() ? parentWidget()->width() - 60 : 400;
    int maxWidth = std::min(parentWidth, 600); // Maksymalna szerokość dymka

    // Różna szerokość w zależności od typu wiadomości
    if (m_type == SentMessage) {
        // Własne wiadomości są węższe
        maxWidth = std::min(maxWidth, 380);
    } else if (m_type == SystemMessage) {
        // Wiadomości systemowe mają średnią szerokość
        maxWidth = std::min(maxWidth, 450);
    }

    // Obliczamy potrzebną wysokość na podstawie tekstu
    QTextDocument doc;
    doc.setHtml(m_messageLabel->text());
    doc.setTextWidth(maxWidth - 26); // Uwzględniamy wewnętrzne marginesy

    int height = doc.size().height() + 20; // Dodajemy marginesy
    int width = std::min(maxWidth, (int)doc.idealWidth() + 30);

    // Minimalny rozmiar dymka
    width = std::max(width, 80);
    height = std::max(height, 40);

    return QSize(width, height);
}

void MessageBubble::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(m_opacityEffect->opacity());

    // Zastosowanie transformacji skali
    QTransform transform;
    transform.translate(width() / 2, height() / 2);
    transform.scale(m_scale, m_scale);
    transform.translate(-width() / 2, -height() / 2);
    painter.setTransform(transform);

    // Rysowanie tła dymka
    QBrush brush;
    QColor bgColor;

    switch(m_type) {
        case SentMessage:
            bgColor = QColor("#0b93f6");
            break;
        case ReceivedMessage:
            bgColor = QColor("#262626");
            break;
        case SystemMessage:
            bgColor = QColor("#3a3a3a");
            break;
    }

    brush = QBrush(bgColor);
    painter.setBrush(brush);
    painter.setPen(Qt::NoPen);

    // Rysowanie dymka
    painter.drawPath(m_bubblePath);

    // Wywołanie rysowania standardowych elementów
    style()->drawPrimitive(QStyle::PE_Frame, &opt, &painter, this);
}

void MessageBubble::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    updateBubblePath();
}

void MessageBubble::updateBubblePath()
{
    // Aktualizujemy ścieżkę dymka po zmianie rozmiaru
    m_bubblePath = QPainterPath();

    QRect rect = this->rect();
    int cornerRadius = 18;

    if (m_type == SystemMessage) {
        cornerRadius = 14; // Mniejsze zaokrąglenie dla systemowych
    }

    m_bubblePath.addRoundedRect(rect, cornerRadius, cornerRadius);
}