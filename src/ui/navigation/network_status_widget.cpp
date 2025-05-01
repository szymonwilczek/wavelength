#include "network_status_widget.h"

#include <QPainter>
#include <QNetworkReply>
#include <QHBoxLayout>
#include <QDateTime>
#include <qeventloop.h>
#include <QGraphicsDropShadowEffect>
#include <QPaintEvent>
#include <QTimer>

NetworkStatusWidget::NetworkStatusWidget(QWidget *parent)
    : QWidget(parent)
    , current_quality_(kNone)
    , ping_value_(0)
{
    // Zmniejszona wysokość widgetu
    setFixedHeight(30);  // Zmniejszone z 36 na 30
    setMinimumWidth(200);  // Nieznacznie zmniejszona szerokość

    // Układ poziomy dla ikon i tekstu z mniejszymi marginesami
    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 3, 10, 3);  // Zmniejszone marginesy
    layout->setSpacing(6);  // Zmniejszony odstęp między elementami

    // Ikona połączenia - zmniejszony rozmiar
    icon_label_ = new QLabel(this);
    icon_label_->setFixedSize(20, 18);  // Zmniejszone z 28x24 na 20x18

    // Etykieta statusu
    status_label_ = new QLabel("SYSTEM READY", this);

    // Ustaw czcionkę dla statusu
    status_label_->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';"
        "   font-size: 9px;"  // Mniejszy rozmiar czcionki
        "   letter-spacing: 0.5px;"
        "}"
    );

    // Nowa etykieta dla pingu - mniejsza czcionka
    ping_label_ = new QLabel("0ms", this);
    ping_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ping_label_->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Medium';"
        "   font-size: 9px;"  // Mniejszy rozmiar czcionki
        "}"
    );

    // Dodanie elementów do layoutu
    layout->addWidget(icon_label_);
    layout->addWidget(status_label_);
    layout->addStretch(1);
    layout->addWidget(ping_label_);

    setLayout(layout);

    // Ustawienie tła na przezroczyste
    setAttribute(Qt::WA_TranslucentBackground);

    // Efekt poświaty dla całego widgetu - zmniejszony blur
    const auto widget_glow = new QGraphicsDropShadowEffect(this);
    widget_glow->setBlurRadius(6);  // Zmniejszone z 8 na 6
    widget_glow->setOffset(0, 0);
    widget_glow->setColor(QColor(0, 195, 255, 100));
    setGraphicsEffect(widget_glow);

    // Utworzenie managera sieci
    network_manager_ = new QNetworkAccessManager(this);

    // Timer do regularnej aktualizacji statusu
    update_timer_ = new QTimer(this);
    connect(update_timer_, &QTimer::timeout, this, &NetworkStatusWidget::CheckNetworkStatus);
    update_timer_->start(5000); // Aktualizacja co 5 sekund

    // Początkowe sprawdzenie
    CheckNetworkStatus();
}

NetworkStatusWidget::~NetworkStatusWidget() {
    update_timer_->stop();
}

void NetworkStatusWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tworzymy zaokrągloną ramkę
    const QRect frame_rect = rect().adjusted(1, 1, -1, -1);
    constexpr int corner_radius = 6;

    // Rysowanie półprzezroczystego tła
    constexpr QColor background_color(20, 20, 30, 180);
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawRoundedRect(frame_rect, corner_radius, corner_radius);

    // Rysowanie ramki z kolorem zależnym od jakości połączenia
    QPen border_pen(border_color_);
    border_pen.setWidth(2);
    painter.setPen(border_pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(frame_rect, corner_radius, corner_radius);

    QWidget::paintEvent(event);
}

void NetworkStatusWidget::CheckNetworkStatus() {
    // Wykonanie testowego żądania do Google, aby sprawdzić łączność
    const QNetworkRequest request(QUrl("https://www.google.com")); //TODO: change to ping a wavelength socket server
    QNetworkReply *reply = network_manager_->get(request);

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
    const qint64 start_time = QDateTime::currentMSecsSinceEpoch();

    // Oczekiwanie na odpowiedź lub timeout
    loop.exec();

    // Obliczenie czasu odpowiedzi
    const qint64 response_time = QDateTime::currentMSecsSinceEpoch() - start_time;

    // Zachowujemy wartość pingu
    ping_value_ = response_time;

    // Określenie jakości połączenia na podstawie czasu odpowiedzi
    if (timer.isActive() && reply->error() == QNetworkReply::NoError) {
        timer.stop();

        if (response_time < 200) {
            current_quality_ = kExcellent;
        } else if (response_time < 500) {
            current_quality_ = kGood;
        } else if (response_time < 1000) {
            current_quality_ = kFair;
        } else {
            current_quality_ = kPoor;
        }
    } else {
        current_quality_ = kNone;
        ping_value_ = 0;  // Ping 0 przy braku połączenia
    }

    reply->deleteLater();

    // Aktualizacja wyświetlania
    UpdateStatusDisplay();
}

void NetworkStatusWidget::UpdateStatusDisplay() {
    // Aktualizacja etykiety statusu
    switch (current_quality_) {
    case kExcellent:
        status_label_->setText("SYSTEM READY");
        break;
    case kGood:
        status_label_->setText("SYSTEM ONLINE");
        break;
    case kFair:
        status_label_->setText("CONNECTION FAIR");
        break;
    case kPoor:
        status_label_->setText("CONNECTION UNSTABLE");
        break;
    case kNone:
        status_label_->setText("OFFLINE");
        break;
    }

    // Aktualizacja wartości pingu
    if (ping_value_ > 0) {
        ping_label_->setText(QString("%1ms").arg(ping_value_));
    } else {
        ping_label_->setText("---");  // Brak połączenia
    }

    // Aktualizacja koloru wszystkich elementów
    border_color_ = GetQualityColor(current_quality_);
    const QString color_style = QString("color: %1;").arg(border_color_.name());
    status_label_->setStyleSheet(color_style);
    ping_label_->setStyleSheet(color_style);

    // Utworzenie ikony dla aktualnego statusu
    CreateNetworkIcon(current_quality_);

    // Odświeżenie widgetu
    update();
}

QColor NetworkStatusWidget::GetQualityColor(const NetworkQuality quality) {
    switch (quality) {
    case kExcellent:
        return QColor(0, 255, 170); // Neonowy cyjan
    case kGood:
        return QColor(0, 195, 255); // Niebieski
    case kFair:
        return QColor(255, 165, 0); // Pomarańczowy
    case kPoor:
        return QColor(255, 50, 50); // Czerwony
    case kNone:
        return QColor(100, 100, 100); // Szary
    }
    return QColor(100, 100, 100);
}

void NetworkStatusWidget::CreateNetworkIcon(const NetworkQuality quality) const {
    // Zmniejszone rozmiary ikony
    constexpr int icon_width = 20;   // Zmniejszone z 28
    constexpr int icon_height = 18;  // Zmniejszone z 24

    QPixmap pixmap(icon_width, icon_height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor signal_color = GetQualityColor(quality);
    constexpr QColor inactive_color(80, 80, 80, 100);

    constexpr QPointF center(icon_width/2, icon_height*0.8);
    constexpr int max_arcs = 4;
    int active_arcs = 0;

    switch(quality) {
        case kExcellent: active_arcs = 4; break;
        case kGood:      active_arcs = 3; break;
        case kFair:      active_arcs = 2; break;
        case kPoor:      active_arcs = 1; break;
        case kNone:      active_arcs = 0; break;
    }

    // Rysuj punkt źródłowy - mniejszy punkt
    painter.setPen(Qt::NoPen);
    painter.setBrush(active_arcs > 0 ? signal_color : inactive_color);
    painter.drawEllipse(center, 1.5, 1.5);  // Zmniejszone z 2,2

    // Rysuj łuki WiFi - mniejsze i bliżej siebie
    for (int i = 0; i < max_arcs; i++) {
        // Wielkość łuku zależy od numeru (większe na zewnątrz) - zmniejszone wartości
        const int arc_radius = 4 + (i * 3);  // Zmniejszone z 5+(i*4)

        QPen arc_pen;
        if (i < active_arcs) {
            arc_pen = QPen(signal_color, 1.5);  // Cieńsza linia
            // Dodanie subtelnej poświaty dla aktywnych łuków
            if (quality != kNone) {
                painter.setPen(QPen(QColor(signal_color.red(), signal_color.green(), signal_color.blue(), 80), 2));
                painter.drawArc(QRectF(center.x() - arc_radius, center.y() - arc_radius,
                                      arc_radius * 2, arc_radius * 2),
                               45 * 16, 90 * 16);
            }
        } else {
            arc_pen = QPen(inactive_color, 1.5);  // Cieńsza linia
        }

        painter.setPen(arc_pen);
        painter.drawArc(QRectF(center.x() - arc_radius, center.y() - arc_radius,
                              arc_radius * 2, arc_radius * 2),
                       45 * 16, 90 * 16);
    }

    icon_label_->setPixmap(pixmap);
}
