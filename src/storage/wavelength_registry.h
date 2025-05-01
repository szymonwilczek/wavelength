#ifndef WAVELENGTH_REGISTRY_H
#define WAVELENGTH_REGISTRY_H

#include <QObject>
#include <QString>
#include <QPointer>
#include <QWebSocket>
#include <QDebug>

struct WavelengthInfo {
    QString frequency;
    bool is_password_protected;
    QString password;
    QString host_id;
    bool is_host;
    int host_port;
    QPointer<QWebSocket> socket = nullptr;
    bool is_closing = false;
    QDateTime created_at = QDateTime::currentDateTime();
};

class WavelengthRegistry final : public QObject {
    Q_OBJECT

public:
    static WavelengthRegistry* GetInstance() {
        static WavelengthRegistry instance;
        return &instance;
    }

    bool AddWavelength(const QString &frequency, const WavelengthInfo& info);

    bool RemoveWavelength(const QString &frequency);

    bool HasWavelength(const QString &frequency) const {
        return wavelengths_.contains(frequency);
    }

    WavelengthInfo GetWavelengthInfo(const QString &frequency) const;

    QList<QString> GetAllWavelengths() const {
        return wavelengths_.keys();
    }

    bool UpdateWavelength(const QString &frequency, const WavelengthInfo& info);

    void SetActiveWavelength(const QString &frequency);

    QString GetActiveWavelength() const {
        return active_wavelength_;
    }

    double IsWavelengthActive(const QString &frequency) const {
        return active_wavelength_ == frequency;
    }

    bool AddPendingRegistration(const QString &frequency);

    bool RemovePendingRegistration(const QString &frequency);

    bool IsPendingRegistration(const QString &frequency) const {
        return pending_registrations_.contains(frequency);
    }

    QPointer<QWebSocket> GetWavelengthSocket(const QString &frequency) const;

    void SetWavelengthSocket(const QString &frequency, const QPointer<QWebSocket> &socket);

    bool MarkWavelengthClosing(const QString &frequency, bool closing = true);

    bool IsWavelengthClosing(const QString &frequency) const;

    void ClearAllWavelengths();

signals:
    void wavelengthAdded(QString frequency);
    void wavelengthRemoved(QString frequency);
    void wavelengthUpdated(QString frequency);
    void activeWavelengthChanged(QString previous_frequency, QString new_frequency);

private:
    explicit WavelengthRegistry(QObject* parent = nullptr) : QObject(parent), active_wavelength_(-1) {}
    ~WavelengthRegistry() override {}

    WavelengthRegistry(const WavelengthRegistry&) = delete;
    WavelengthRegistry& operator=(const WavelengthRegistry&) = delete;

    QMap<QString, WavelengthInfo> wavelengths_;
    QSet<QString> pending_registrations_;
    QString active_wavelength_;
};

#endif // WAVELENGTH_REGISTRY_H