#include "message_bubble.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QGraphicsBlurEffect>
#include <QTimer>

// Implementacja opcjonalnych dodatkowych metod dla MessageBubble

// Metoda do ustalenia optymalnego rozmiaru dymka
QSize MessageBubble::sizeHint() const {
    // Bazujemy na szerokości nadrzędnego widgetu, ale z ograniczeniem
    int parentWidth = parentWidget() ? parentWidget()->width() - 60 : 400;
    int maxWidth = std::min(parentWidth, 600); // Maksymalna szerokość dymka
    
    if (m_type == SentMessage) {
        // Wiadomości wysyłane są zazwyczaj węższe
        maxWidth = std::min(maxWidth, 400);
    }
    
    // Obliczamy potrzebną wysokość na podstawie tekstu
    QTextDocument doc;
    doc.setHtml(m_messageLabel->text());
    doc.setTextWidth(maxWidth - 26); // Uwzględniamy wewnętrzne marginesy
    
    int height = doc.size().height() + 20; // Dodajemy marginesy
    int width = std::min(maxWidth, (int)doc.idealWidth() + 26);
    
    // Minimalny rozmiar dymka
    width = std::max(width, 80);
    height = std::max(height, 40);
    
    return QSize(width, height);
}

// Zaawansowana metoda animacji wejścia z dostosowaniem do kierunku
void MessageBubble::startAdvancedEntryAnimation() {
    // Stopniujemy opóźnienie animacji w zależności od liczby widgetów w obszarze przewijania
    // Daje to efekt "kaskadowego" pojawiania się wiadomości
    int delay = 0;
    
    if (parentWidget() && parentWidget()->layout()) {
        int itemCount = parentWidget()->layout()->count();
        if (itemCount > 10) {
            // Ograniczamy opóźnienie dla zbyt wielu wiadomości
            delay = 15;
        } else {
            delay = itemCount * 10;
        }
    }
    
    QTimer::singleShot(delay, this, [this]() {
        // 1. Animacja opacity
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity");
        opacityAnim->setDuration(280);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
        
        // 2. Animacja powiększania z lekkim efektem odbicia
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
            // Wiadomości wysłane wjeżdżają z prawej i dolnej strony
            posAnim->setStartValue(QPointF(startPos.x() + 20, startPos.y() + 10));
        } else if (m_type == ReceivedMessage) {
            // Wiadomości odebrane wjeżdżają z lewej
            posAnim->setStartValue(QPointF(startPos.x() - 20, startPos.y()));
        } else {
            // Wiadomości systemowe pojawiają się z góry
            posAnim->setStartValue(QPointF(startPos.x(), startPos.y() - 15));
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
    });
}