#ifndef NETWORK_STATUS_WIDGET_H
#define NETWORK_STATUS_WIDGET_H

#include <QLabel>
#include <QNetworkAccessManager>
#include <QWidget>

class NetworkStatusWidget final : public QWidget {
    Q_OBJECT

public:
    explicit NetworkStatusWidget(QWidget *parent = nullptr);
    ~NetworkStatusWidget() override;

    public slots:
        void CheckNetworkStatus();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    enum NetworkQuality {
        None,
        Poor,
        Fair,
        Good,
        Excellent
    };

    QLabel *status_label_;
    QLabel *ping_label_;  // Nowa etykieta dla wartości pingu
    QLabel *icon_label_;
    QNetworkAccessManager *network_manager_;
    QTimer *update_timer_;
    NetworkQuality current_quality_;
    QColor border_color_;
    qint64 ping_value_;   // Nowe pole do przechowywania pingu w ms

    void UpdateStatusDisplay();
    void CreateNetworkIcon(NetworkQuality quality) const;

    static QColor GetQualityColor(NetworkQuality quality);
};

#endif // NETWORK_STATUS_WIDGET_H