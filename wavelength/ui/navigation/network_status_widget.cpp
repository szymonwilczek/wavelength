#include "network_status_widget.h"
#include "../../../font_manager.h"

#include <QPainter>
#include <QNetworkReply>
#include <QHBoxLayout>
#include <QDateTime>
#include <qeventloop.h>
#include <QPainterPath>
#include <QPaintEvent>

NetworkStatusWidget::NetworkStatusWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentQuality(NONE)
{
    // Układ poziomy dla ikon i tekstu
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(8);

    // Ikona połączenia
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(32, 24);
    
    // Etykieta statusu
    m_statusLabel = new QLabel("SYSTEM READY", this);
    
    // Ustawienie fontu BlenderPro dla napisu SYSTEM READY
    QFont statusFont = FontManager::instance().getFont(FontFamily::BlenderPro, FontStyle::Medium, 10);
    m_statusLabel->setFont(statusFont);
    
    // Stylizacja cyberpunkowa etykiety
    m_statusLabel->setStyleSheet("QLabel { color: #00C3FF; }");
    
    // Dodanie efektu poświaty dla napisu
    QGraphicsDropShadowEffect* textGlow = new QGraphicsDropShadowEffect(m_statusLabel);
    textGlow->setBlurRadius(5);
    textGlow->setOffset(0, 0);
    textGlow->setColor(QColor(0, 195, 255, 100));
    m_statusLabel->setGraphicsEffect(textGlow);

    // Dodanie elementów do layoutu
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_statusLabel);
    
    setLayout(layout);
    
    // Utworzenie managera sieci
    m_networkManager = new QNetworkAccessManager(this);
    
    // Timer do regularnej aktualizacji statusu
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &NetworkStatusWidget::checkNetworkStatus);
    m_updateTimer->start(5000); // Aktualizacja co 5 sekund
    
    // Początkowe sprawdzenie
    checkNetworkStatus();
}

NetworkStatusWidget::~NetworkStatusWidget() {
    m_updateTimer->stop();
}

void NetworkStatusWidget::checkNetworkStatus() {
    // Wykonanie testowego żądania do Google, aby sprawdzić łączność
    QNetworkRequest request(QUrl("https://www.google.com"));
    QNetworkReply *reply = m_networkManager->get(request);
    
    // Timeout dla żądania
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    
    // Połączenie sygnałów timera i odpowiedzi
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // Uruchomienie timera na 2 sekundy
    timer.start(2000);
    
    // Start pomiaru czasu
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    
    // Oczekiwanie na odpowiedź lub timeout
    loop.exec();
    
    // Obliczenie czasu odpowiedzi
    qint64 responseTime = QDateTime::currentMSecsSinceEpoch() - startTime;
    
    // Określenie jakości połączenia na podstawie czasu odpowiedzi
    if (timer.isActive() && reply->error() == QNetworkReply::NoError) {
        timer.stop();
        
        if (responseTime < 200) {
            m_currentQuality = EXCELLENT;
        } else if (responseTime < 500) {
            m_currentQuality = GOOD;
        } else if (responseTime < 1000) {
            m_currentQuality = FAIR;
        } else {
            m_currentQuality = POOR;
        }
    } else {
        m_currentQuality = NONE;
    }
    
    reply->deleteLater();
    
    // Aktualizacja wyświetlania
    updateStatusDisplay();
}

void NetworkStatusWidget::updateStatusDisplay() {
    // Aktualizacja etykiety statusu
    switch (m_currentQuality) {
        case EXCELLENT:
            m_statusLabel->setText("SYSTEM READY");
            break;
        case GOOD:
            m_statusLabel->setText("SYSTEM ONLINE");
            break;
        case FAIR:
            m_statusLabel->setText("CONNECTION FAIR");
            break;
        case POOR:
            m_statusLabel->setText("CONNECTION UNSTABLE");
            break;
        case NONE:
            m_statusLabel->setText("OFFLINE");
            break;
    }
    
    // Utworzenie ikony dla aktualnego statusu
    createNetworkIcon(m_currentQuality);
}

void NetworkStatusWidget::createNetworkIcon(NetworkQuality quality) {
    // Rozmiary ikony
    const int iconWidth = 32;
    const int iconHeight = 24;
    
    QPixmap pixmap(iconWidth, iconHeight);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Kolory dla różnych poziomów sygnału
    QColor activeColor;
    QColor inactiveColor(80, 80, 80, 100);
    
    switch (quality) {
        case EXCELLENT:
            activeColor = QColor(0, 255, 170); // Neonowy cyjan
            break;
        case GOOD: 
            activeColor = QColor(0, 195, 255); // Niebieski
            break;
        case FAIR:
            activeColor = QColor(255, 165, 0); // Pomarańczowy
            break;
        case POOR:
            activeColor = QColor(255, 50, 50); // Czerwony
            break;
        case NONE:
            activeColor = QColor(100, 100, 100); // Szary
            break;
    }
    
    // Szerokość i wysokość kresek
    int barWidth = 4;
    int spacing = 2;
    int startX = 2;
    int totalBars = 4;
    
    // Rysowanie kresek sygnału WiFi
    for (int i = 0; i < totalBars; i++) {
        int barHeight = 4 + (i * 4); // Rosnąca wysokość
        int y = iconHeight - barHeight;
        QColor color = (i < qualityToBarNumber(quality)) ? activeColor : inactiveColor;
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        
        // Rysowanie prostokąta
        QRect barRect(startX + i * (barWidth + spacing), y, barWidth, barHeight);
        painter.drawRoundedRect(barRect, 1, 1);
        
        // Dodanie delikatnej poświaty dla aktywnych kresek
        if (i < qualityToBarNumber(quality)) {
            QPainterPath path;
            path.addRoundedRect(barRect, 1, 1);
            
            QColor glowColor = activeColor;
            glowColor.setAlpha(80);
            
            for (int j = 3; j > 0; j--) {
                QPen glowPen(glowColor);
                glowPen.setWidth(j);
                painter.setPen(glowPen);
                painter.drawPath(path);
            }
        }
    }
    
    m_iconLabel->setPixmap(pixmap);
}

int NetworkStatusWidget::qualityToBarNumber(NetworkQuality quality) {
    switch (quality) {
        case EXCELLENT: return 4;
        case GOOD: return 3;
        case FAIR: return 2;
        case POOR: return 1;
        case NONE: return 0;
    }
    return 0;
}