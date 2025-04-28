#ifndef WAVELENGTH_REGISTRY_H
#define WAVELENGTH_REGISTRY_H

#include <QObject>
#include <QString>
#include <QPointer>
#include <QWebSocket>
#include <QDebug>

struct WavelengthInfo {
    QString frequency;
    bool isPasswordProtected;
    QString password;
    QString hostId;
    bool isHost;
    int hostPort;
    QPointer<QWebSocket> socket = nullptr;
    bool isClosing = false;
    QDateTime createdAt = QDateTime::currentDateTime();
};

class WavelengthRegistry final : public QObject {
    Q_OBJECT

public:
    static WavelengthRegistry* getInstance() {
        static WavelengthRegistry instance;
        return &instance;
    }

    bool addWavelength(const QString &frequency, const WavelengthInfo& info);

    bool removeWavelength(const QString &frequency);

    bool hasWavelength(const QString &frequency) const {
        return m_wavelengths.contains(frequency);
    }

    WavelengthInfo getWavelengthInfo(const QString &frequency) const;

    QList<QString> getAllWavelengths() const {
        return m_wavelengths.keys();
    }

    bool updateWavelength(const QString &frequency, const WavelengthInfo& info);

    void setActiveWavelength(const QString &frequency);

    QString getActiveWavelength() const {
        return m_activeWavelength;
    }

    double isWavelengthActive(const QString &frequency) const {
        return m_activeWavelength == frequency;
    }

    bool addPendingRegistration(const QString &frequency);

    bool removePendingRegistration(const QString &frequency);

    bool isPendingRegistration(const QString &frequency) const {
        return m_pendingRegistrations.contains(frequency);
    }

    QPointer<QWebSocket> getWavelengthSocket(const QString &frequency) const;

    void setWavelengthSocket(const QString &frequency, const QPointer<QWebSocket> &socket);

    bool markWavelengthClosing(const QString &frequency, bool closing = true);

    bool isWavelengthClosing(const QString &frequency) const;

    void clearAllWavelengths();

signals:
    void wavelengthAdded(QString frequency);
    void wavelengthRemoved(QString frequency);
    void wavelengthUpdated(QString frequency);
    void activeWavelengthChanged(QString previousFrequency, QString newFrequency);

private:
    explicit WavelengthRegistry(QObject* parent = nullptr) : QObject(parent), m_activeWavelength(-1) {}
    ~WavelengthRegistry() override {}

    WavelengthRegistry(const WavelengthRegistry&) = delete;
    WavelengthRegistry& operator=(const WavelengthRegistry&) = delete;

    QMap<QString, WavelengthInfo> m_wavelengths;
    QSet<QString> m_pendingRegistrations;
    QString m_activeWavelength;
};

#endif // WAVELENGTH_REGISTRY_H