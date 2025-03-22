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

        // 2. Złożona animacja powiększania
        QSequentialAnimationGroup* scaleGroup = new QSequentialAnimationGroup(this);

        // Najpierw powiększanie do 1.03 (lekki overshooting)
        QPropertyAnimation* scaleUp = new QPropertyAnimation(this, "scale");
        scaleUp->setDuration(260);
        scaleUp->setStartValue(0.93);
        scaleUp->setEndValue(1.03);
        scaleUp->setEasingCurve(QEasingCurve::OutQuad);

        // Potem zmniejszenie do docelowego rozmiaru
        QPropertyAnimation* scaleDown = new QPropertyAnimation(this, "scale");
        scaleDown->setDuration(180);
        scaleDown->setStartValue(1.03);
        scaleDown->setEndValue(1.0);
        scaleDown->setEasingCurve(QEasingCurve::OutBack);

        scaleGroup->addAnimation(scaleUp);
        scaleGroup->addAnimation(scaleDown);

        // 3. Animacja pojawienia się z kierunku zależnego od typu - POPRAWIONA
        QPropertyAnimation* posAnim = new QPropertyAnimation(this, "position");
        posAnim->setDuration(320);
        QPointF startPos = pos();
        QPointF endPos = startPos;

        switch (m_type) {
            case SentMessage: {
                // Wiadomości wysłane "wjeżdżają" z prawej strony
                posAnim->setStartValue(QPointF(parentWidget()->width() - 40, startPos.y()));
                posAnim->setKeyValueAt(0.7, QPointF(startPos.x() + 5, startPos.y()));
                break;
            }
            case ReceivedMessage: {
                // Wiadomości odebrane "wjeżdżają" z lewej strony
                posAnim->setStartValue(QPointF(-width() + 40, startPos.y()));
                posAnim->setKeyValueAt(0.7, QPointF(startPos.x() - 5, startPos.y()));
                break;
            }
            case SystemMessage: {
                // Wiadomości systemowe pojawiają się ze środka, z subtelnymi ruchami w górę i dół
                posAnim->setStartValue(QPointF(startPos.x(), startPos.y() - 20));
                posAnim->setKeyValueAt(0.5, QPointF(startPos.x(), startPos.y() + 5));
                break;
            }
        }

        posAnim->setEndValue(endPos);
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
    painter.setOpacity(m_opacityEffect->opacity());

    // Zastosowanie transformacji skali
    QTransform transform;
    transform.translate(width() / 2, height() / 2);
    transform.scale(m_scale, m_scale);
    transform.translate(-width() / 2, -height() / 2);
    painter.setTransform(transform);

    // Konfiguracja koloru tła
    QColor bgColor;

    switch(m_type) {
        case SentMessage:
            bgColor = QColor("#0b93f6");
            break;
        case ReceivedMessage:
            bgColor = QColor("#262626");
            break;
        case SystemMessage:
            // Bez tła dla wiadomości systemowych
            QFrame::paintEvent(event);
            return;
    }

    // Standardowy promień zaokrąglenia
    const int cornerRadius = 18;

    // Rysowanie dymku z rogiem - POPRAWIONE
    QPainterPath path;
    QRectF rect(0, 0, width(), height());

    if (m_type == SentMessage) {
        // Dymek wysłanej wiadomości (niebieski z rogiem po prawej) w stylu iOS
        // Tworzymy zaokrąglony prostokąt jako ścieżkę bazową
        path.addRoundedRect(rect.adjusted(0, 0, -6, 0), cornerRadius, cornerRadius);

        // Dodajemy "róg" jako część ścieżki - POPRAWIONE
        const int tipSize = 8; // Rozmiar "rogu"
        const int tipPosY = rect.bottom() - 15; // Pozycja Y rogu

        QPainterPath tipPath;
        tipPath.moveTo(rect.right() - 6, tipPosY);
        tipPath.lineTo(rect.right() + tipSize, rect.bottom() - 10);
        tipPath.lineTo(rect.right() - 6, tipPosY + 10);

        // Łączymy ścieżki używając operacji unii
        path = path.united(tipPath);

    } else if (m_type == ReceivedMessage) {
        // Dymek otrzymanej wiadomości (szary z rogiem po lewej) w stylu iOS
        path.addRoundedRect(rect.adjusted(6, 0, 0, 0), cornerRadius, cornerRadius);

        // Dodajemy "róg" jako część ścieżki - POPRAWIONE
        const int tipSize = 8; // Rozmiar "rogu"
        const int tipPosY = rect.bottom() - 15; // Pozycja Y rogu

        QPainterPath tipPath;
        tipPath.moveTo(rect.left() + 6, tipPosY);
        tipPath.lineTo(rect.left() - tipSize, rect.bottom() - 10);
        tipPath.lineTo(rect.left() + 6, tipPosY + 10);

        // Łączymy ścieżki używając operacji unii
        path = path.united(tipPath);
    }

    // Wypełniamy dymek kolorem
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    // Z-order w Qt działa automatycznie - child widgets są zawsze rysowane nad parent
    QFrame::paintEvent(event);
}

void MessageBubble::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    update(); // Wymuszamy przerysowanie przy zmianie rozmiaru
}