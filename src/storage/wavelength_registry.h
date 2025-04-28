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

class WavelengthRegistry : public QObject {
    Q_OBJECT

public:
    static WavelengthRegistry* getInstance() {
        static WavelengthRegistry instance;
        return &instance;
    }

    bool addWavelength(QString frequency, const WavelengthInfo& info);

    bool removeWavelength(QString frequency);

    bool hasWavelength(QString frequency) const {
        return m_wavelengths.contains(frequency);
    }

    WavelengthInfo getWavelengthInfo(QString frequency) const;

    QList<QString> getAllWavelengths() const {
        return m_wavelengths.keys();
    }

    bool updateWavelength(QString frequency, const WavelengthInfo& info);

    void setActiveWavelength(QString frequency);

    QString getActiveWavelength() const {
        return m_activeWavelength;
    }

    double isWavelengthActive(QString frequency) const {
        return m_activeWavelength == frequency;
    }

    bool addPendingRegistration(QString frequency);

    bool removePendingRegistration(QString frequency);

    bool isPendingRegistration(QString frequency) const {
        return m_pendingRegistrations.contains(frequency);
    }

    QPointer<QWebSocket> getWavelengthSocket(QString frequency) const;

    void setWavelengthSocket(QString frequency, QPointer<QWebSocket> socket);

    bool markWavelengthClosing(QString frequency, bool closing = true);

    bool isWavelengthClosing(QString frequency) const;

    void clearAllWavelengths();

signals:
    void wavelengthAdded(QString frequency);
    void wavelengthRemoved(QString frequency);
    void wavelengthUpdated(QString frequency);
    void activeWavelengthChanged(QString previousFrequency, QString newFrequency);

private:
    WavelengthRegistry(QObject* parent = nullptr) : QObject(parent), m_activeWavelength(-1) {}
    ~WavelengthRegistry() {}

    WavelengthRegistry(const WavelengthRegistry&) = delete;
    WavelengthRegistry& operator=(const WavelengthRegistry&) = delete;

    QMap<QString, WavelengthInfo> m_wavelengths;
    QSet<QString> m_pendingRegistrations;
    QString m_activeWavelength;
};

#endif // WAVELENGTH_REGISTRY_H