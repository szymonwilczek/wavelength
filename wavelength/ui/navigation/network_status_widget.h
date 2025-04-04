#ifndef NETWORK_STATUS_WIDGET_H
#define NETWORK_STATUS_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QGraphicsDropShadowEffect>

class NetworkStatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit NetworkStatusWidget(QWidget *parent = nullptr);
    ~NetworkStatusWidget();

    public slots:
        void checkNetworkStatus();

private:
    enum NetworkQuality {
        NONE,
        POOR,
        FAIR,
        GOOD,
        EXCELLENT
    };

    QLabel *m_statusLabel;
    QLabel *m_iconLabel;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_updateTimer;
    NetworkQuality m_currentQuality;

    void updateStatusDisplay();
    void createNetworkIcon(NetworkQuality quality);
    int qualityToBarNumber(NetworkQuality quality);
};

#endif // NETWORK_STATUS_WIDGET_H