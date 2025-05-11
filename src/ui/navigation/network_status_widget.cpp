#include "network_status_widget.h"

#include <QEventLoop>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkReply>
#include <QPainter>
#include <QTimer>

#include "../../app/managers/translation_manager.h"

NetworkStatusWidget::NetworkStatusWidget(QWidget *parent)
    : QWidget(parent)
      , current_quality_(kNone)
      , ping_value_(0) {
    setFixedHeight(30);
    setMinimumWidth(150);
    setAttribute(Qt::WA_TranslucentBackground);

    translator_ = TranslationManager::GetInstance();

    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 3, 10, 3);
    layout->setSpacing(6);

    icon_label_ = new QLabel(this);
    icon_label_->setFixedSize(20, 18);

    status_label_ = new QLabel(
        translator_->Translate("NetworkStatusWidget.SystemReady", "SYSTEM READY"), this);

    status_label_->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';"
        "   font-size: 9px;"
        "   letter-spacing: 0.5px;"
        "}"
    );

    ping_label_ = new QLabel("0ms", this);
    ping_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ping_label_->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Medium';"
        "   font-size: 9px;"
        "}"
    );

    layout->addWidget(icon_label_);
    layout->addWidget(status_label_);
    layout->addStretch(1);
    layout->addWidget(ping_label_);
    setLayout(layout);

    const auto widget_glow = new QGraphicsDropShadowEffect(this);
    widget_glow->setBlurRadius(6);
    widget_glow->setOffset(0, 0);
    widget_glow->setColor(QColor(0, 195, 255, 100));
    setGraphicsEffect(widget_glow);

    network_manager_ = new QNetworkAccessManager(this);

    update_timer_ = new QTimer(this);
    connect(update_timer_, &QTimer::timeout, this, &NetworkStatusWidget::CheckNetworkStatus);
    update_timer_->start(5000);

    CheckNetworkStatus();
}

NetworkStatusWidget::~NetworkStatusWidget() {
    update_timer_->stop();
}

void NetworkStatusWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect frame_rect = rect().adjusted(1, 1, -1, -1);
    constexpr int corner_radius = 6;

    constexpr QColor background_color(20, 20, 30, 180);
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawRoundedRect(frame_rect, corner_radius, corner_radius);

    QPen border_pen(border_color_);
    border_pen.setWidth(2);
    painter.setPen(border_pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(frame_rect, corner_radius, corner_radius);

    QWidget::paintEvent(event);
}

void NetworkStatusWidget::CheckNetworkStatus() {
    const QNetworkRequest request(QUrl("https://www.google.com")); //TODO: change to ping a wavelength socket server
    QNetworkReply *reply = network_manager_->get(request);

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start(2000);

    const qint64 start_time = QDateTime::currentMSecsSinceEpoch();
    loop.exec();
    const qint64 response_time = QDateTime::currentMSecsSinceEpoch() - start_time;
    ping_value_ = response_time;

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
        ping_value_ = 0;
    }

    reply->deleteLater();
    UpdateStatusDisplay();
}

void NetworkStatusWidget::UpdateStatusDisplay() {
    switch (current_quality_) {
        case kExcellent:
            status_label_->setText(translator_->Translate("NetworkStatusWidget.SystemReady", "SYSTEM READY"));
            break;
        case kGood:
            status_label_->setText(translator_->Translate("NetworkStatusWidget.SystemOnline", "SYSTEM ONLINE"));
            break;
        case kFair:
            status_label_->setText(translator_->Translate("NetworkStatusWidget.ConnectionFair", "CONNECTION FAIR"));
            break;
        case kPoor:
            status_label_->setText(
                translator_->Translate("NetworkStatusWidget.ConnectionUnstable", "CONNECTION UNSTABLE"));
            break;
        case kNone:
            status_label_->setText(translator_->Translate("NetworkStatusWidget.Offline", "OFFLINE"));
            break;
    }

    if (ping_value_ > 0) {
        ping_label_->setText(QString("%1ms").arg(ping_value_));
    } else {
        ping_label_->setText("---"); // no connection
    }

    border_color_ = GetQualityColor(current_quality_);
    const QString color_style = QString("color: %1;").arg(border_color_.name());
    status_label_->setStyleSheet(color_style);
    ping_label_->setStyleSheet(color_style);

    CreateNetworkIcon(current_quality_);
    update();
}

QColor NetworkStatusWidget::GetQualityColor(const NetworkQuality quality) {
    switch (quality) {
        case kExcellent:
            return QColor(0, 255, 170);
        case kGood:
            return QColor(0, 195, 255);
        case kFair:
            return QColor(255, 165, 0);
        case kPoor:
            return QColor(255, 50, 50);
        case kNone:
            return QColor(100, 100, 100);
    }
    return QColor(100, 100, 100);
}

void NetworkStatusWidget::CreateNetworkIcon(const NetworkQuality quality) const {
    constexpr int icon_width = 20;
    constexpr int icon_height = 18;

    QPixmap pixmap(icon_width, icon_height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor signal_color = GetQualityColor(quality);
    constexpr QColor inactive_color(80, 80, 80, 100);

    constexpr QPointF center(icon_width / 2, icon_height * 0.8);
    constexpr int max_arcs = 4;
    int active_arcs = 0;

    switch (quality) {
        case kExcellent: active_arcs = 4;
            break;
        case kGood: active_arcs = 3;
            break;
        case kFair: active_arcs = 2;
            break;
        case kPoor: active_arcs = 1;
            break;
        case kNone: active_arcs = 0;
            break;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(active_arcs > 0 ? signal_color : inactive_color);
    painter.drawEllipse(center, 1.5, 1.5);

    for (int i = 0; i < max_arcs; i++) {
        const int arc_radius = 4 + i * 3;

        QPen arc_pen;
        if (i < active_arcs) {
            arc_pen = QPen(signal_color, 1.5);
            if (quality != kNone) {
                painter.setPen(QPen(
                    QColor(signal_color.red(), signal_color.green(), signal_color.blue(), 80),
                    2));
                painter.drawArc(QRectF(center.x() - arc_radius, center.y() - arc_radius,
                                       arc_radius * 2, arc_radius * 2),
                                45 * 16, 90 * 16);
            }
        } else {
            arc_pen = QPen(inactive_color, 1.5);
        }

        painter.setPen(arc_pen);
        painter.drawArc(QRectF(center.x() - arc_radius, center.y() - arc_radius,
                               arc_radius * 2, arc_radius * 2),
                        45 * 16, 90 * 16);
    }

    icon_label_->setPixmap(pixmap);
}
