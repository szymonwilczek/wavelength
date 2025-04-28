#ifndef NETWORK_STATUS_WIDGET_H
#define NETWORK_STATUS_WIDGET_H

#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QGraphicsDropShadowEffect>

class NetworkStatusWidget final : public QWidget {
    Q_OBJECT

public:
    explicit NetworkStatusWidget(QWidget *parent = nullptr);
    ~NetworkStatusWidget() override;

    public slots:
        void checkNetworkStatus();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    enum NetworkQuality {
        NONE,
        POOR,
        FAIR,
        GOOD,
        EXCELLENT
    };

    QLabel *m_statusLabel;
    QLabel *m_pingLabel;  // Nowa etykieta dla wartości pingu
    QLabel *m_iconLabel;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_updateTimer;
    NetworkQuality m_currentQuality;
    QColor m_borderColor;
    qint64 m_pingValue;   // Nowe pole do przechowywania pingu w ms

    void updateStatusDisplay();
    void createNetworkIcon(NetworkQuality quality) const;

    static QColor getQualityColor(NetworkQuality quality);
};

#endif // NETWORK_STATUS_WIDGET_H