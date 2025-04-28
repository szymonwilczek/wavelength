#ifndef WAVELENGTH_STATE_MANAGER_H
#define WAVELENGTH_STATE_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QList>
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>
#include <QVariant>

#include "../storage/wavelength_registry.h"

class WavelengthStateManager : public QObject {
    Q_OBJECT

public:
    static WavelengthStateManager* getInstance() {
        static WavelengthStateManager instance;
        return &instance;
    }

    WavelengthInfo getWavelengthInfo(QString frequency, bool* isHost = nullptr);

    static QString getActiveWavelength();

    void setActiveWavelength(QString frequency);

    bool isActiveWavelengthHost();

    // Pobiera listę częstotliwości do których użytkownik jest dołączony
    QList<QString> getJoinedWavelengths();

    // Rejestruje dołączenie do wavelength
    void registerJoinedWavelength(QString frequency);

    // Wyrejestrowuje opuszczenie wavelength
    void unregisterJoinedWavelength(QString frequency);

    // Zwraca liczbę dołączonych wavelength
    int getJoinedWavelengthCount();

    bool isWavelengthPasswordProtected(QString frequency);

    bool isWavelengthHost(QString frequency);


    QDateTime getWavelengthCreationTime(QString frequency);

    bool isWavelengthJoined(QString frequency);

    bool isWavelengthConnected(QString frequency);

    void addActiveSessionData(QString frequency, const QString& key, const QVariant& value);

    QVariant getActiveSessionData(QString frequency, const QString& key, const QVariant& defaultValue = QVariant());

    void clearSessionData(QString frequency);

    void clearAllSessionData();

    // Obsługa stanu połączenia
    bool isConnecting(QString frequency);

    void setConnecting(QString frequency, bool connecting);

signals:
    void activeWavelengthChanged(QString frequency);

private:
    WavelengthStateManager(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthStateManager() {}

    WavelengthStateManager(const WavelengthStateManager&) = delete;
    WavelengthStateManager& operator=(const WavelengthStateManager&) = delete;

    QMap<QString, QMap<QString, QVariant>> m_sessionData;
    QList<QString> m_connectingWavelengths;
    QList<QString> m_joinedWavelengths; // Lista dołączonych wavelength
};

#endif // WAVELENGTH_STATE_MANAGER_H