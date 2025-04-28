#include "network_status_widget.h"

#include <QPainter>
#include <QNetworkReply>
#include <QHBoxLayout>
#include <QDateTime>
#include <qeventloop.h>
#include <QPaintEvent>

NetworkStatusWidget::NetworkStatusWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentQuality(NONE)
    , m_pingValue(0)
{
    // Zmniejszona wysokość widgetu
    setFixedHeight(30);  // Zmniejszone z 36 na 30
    setMinimumWidth(200);  // Nieznacznie zmniejszona szerokość

    // Układ poziomy dla ikon i tekstu z mniejszymi marginesami
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 3, 10, 3);  // Zmniejszone marginesy
    layout->setSpacing(6);  // Zmniejszony odstęp między elementami

    // Ikona połączenia - zmniejszony rozmiar
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(20, 18);  // Zmniejszone z 28x24 na 20x18

    // Etykieta statusu
    m_statusLabel = new QLabel("SYSTEM READY", this);

    // Ustaw czcionkę dla statusu
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';"
        "   font-size: 9px;"  // Mniejszy rozmiar czcionki
        "   letter-spacing: 0.5px;"
        "}"
    );

    // Nowa etykieta dla pingu - mniejsza czcionka
    m_pingLabel = new QLabel("0ms", this);
    m_pingLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_pingLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Medium';"
        "   font-size: 9px;"  // Mniejszy rozmiar czcionki
        "}"
    );

    // Dodanie elementów do layoutu
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_statusLabel);
    layout->addStretch(1);
    layout->addWidget(m_pingLabel);

    setLayout(layout);

    // Ustawienie tła na przezroczyste
    setAttribute(Qt::WA_TranslucentBackground);

    // Efekt poświaty dla całego widgetu - zmniejszony blur
    QGraphicsDropShadowEffect* widgetGlow = new QGraphicsDropShadowEffect(this);
    widgetGlow->setBlurRadius(6);  // Zmniejszone z 8 na 6
    widgetGlow->setOffset(0, 0);
    widgetGlow->setColor(QColor(0, 195, 255, 100));
    setGraphicsEffect(widgetGlow);

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

void NetworkStatusWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tworzymy zaokrągloną ramkę
    QRect frameRect = rect().adjusted(1, 1, -1, -1);
    int cornerRadius = 6;

    // Rysowanie półprzezroczystego tła
    QColor bgColor(20, 20, 30, 180);
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(frameRect, cornerRadius, cornerRadius);

    // Rysowanie ramki z kolorem zależnym od jakości połączenia
    QPen borderPen(m_borderColor);
    borderPen.setWidth(2);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(frameRect, cornerRadius, cornerRadius);

    QWidget::paintEvent(event);
}

void NetworkStatusWidget::checkNetworkStatus() {
    // Wykonanie testowego żądania do Google, aby sprawdzić łączność
    QNetworkRequest request(QUrl("https://www.google.com")); //TODO: change to ping a wavelength socket server
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

    // Zachowujemy wartość pingu
    m_pingValue = responseTime;

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
        m_pingValue = 0;  // Ping 0 przy braku połączenia
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

    // Aktualizacja wartości pingu
    if (m_pingValue > 0) {
        m_pingLabel->setText(QString("%1ms").arg(m_pingValue));
    } else {
        m_pingLabel->setText("---");  // Brak połączenia
    }

    // Aktualizacja koloru wszystkich elementów
    m_borderColor = getQualityColor(m_currentQuality);
    QString colorStyle = QString("color: %1;").arg(m_borderColor.name());
    m_statusLabel->setStyleSheet(colorStyle);
    m_pingLabel->setStyleSheet(colorStyle);

    // Utworzenie ikony dla aktualnego statusu
    createNetworkIcon(m_currentQuality);

    // Odświeżenie widgetu
    update();
}

QColor NetworkStatusWidget::getQualityColor(NetworkQuality quality) {
    switch (quality) {
    case EXCELLENT:
        return QColor(0, 255, 170); // Neonowy cyjan
    case GOOD:
        return QColor(0, 195, 255); // Niebieski
    case FAIR:
        return QColor(255, 165, 0); // Pomarańczowy
    case POOR:
        return QColor(255, 50, 50); // Czerwony
    case NONE:
        return QColor(100, 100, 100); // Szary
    }
    return QColor(100, 100, 100);
}

void NetworkStatusWidget::createNetworkIcon(NetworkQuality quality) {
    // Zmniejszone rozmiary ikony
    const int iconWidth = 20;   // Zmniejszone z 28
    const int iconHeight = 18;  // Zmniejszone z 24

    QPixmap pixmap(iconWidth, iconHeight);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor signalColor = getQualityColor(quality);
    QColor inactiveColor(80, 80, 80, 100);

    // Rysuj symbol WiFi - łuki o rosnącym promieniu
    QPointF center(iconWidth/2, iconHeight*0.8);
    int maxArcs = 4;
    int activeArcs = 0;

    switch(quality) {
        case EXCELLENT: activeArcs = 4; break;
        case GOOD:      activeArcs = 3; break;
        case FAIR:      activeArcs = 2; break;
        case POOR:      activeArcs = 1; break;
        case NONE:      activeArcs = 0; break;
    }

    // Rysuj punkt źródłowy - mniejszy punkt
    painter.setPen(Qt::NoPen);
    painter.setBrush(activeArcs > 0 ? signalColor : inactiveColor);
    painter.drawEllipse(center, 1.5, 1.5);  // Zmniejszone z 2,2

    // Rysuj łuki WiFi - mniejsze i bliżej siebie
    for (int i = 0; i < maxArcs; i++) {
        // Wielkość łuku zależy od numeru (większe na zewnątrz) - zmniejszone wartości
        int arcRadius = 4 + (i * 3);  // Zmniejszone z 5+(i*4)

        QPen arcPen;
        if (i < activeArcs) {
            arcPen = QPen(signalColor, 1.5);  // Cieńsza linia
            // Dodanie subtelnej poświaty dla aktywnych łuków
            if (quality != NONE) {
                painter.setPen(QPen(QColor(signalColor.red(), signalColor.green(), signalColor.blue(), 80), 2));
                painter.drawArc(QRectF(center.x() - arcRadius, center.y() - arcRadius,
                                      arcRadius * 2, arcRadius * 2),
                               45 * 16, 90 * 16);
            }
        } else {
            arcPen = QPen(inactiveColor, 1.5);  // Cieńsza linia
        }

        painter.setPen(arcPen);
        painter.drawArc(QRectF(center.x() - arcRadius, center.y() - arcRadius,
                              arcRadius * 2, arcRadius * 2),
                       45 * 16, 90 * 16);
    }

    m_iconLabel->setPixmap(pixmap);
}