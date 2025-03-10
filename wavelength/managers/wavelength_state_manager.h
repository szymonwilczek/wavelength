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

#include "../registry/wavelength_registry.h"

class WavelengthStateManager : public QObject {
    Q_OBJECT

public:
    static WavelengthStateManager* getInstance() {
        static WavelengthStateManager instance;
        return &instance;
    }

    WavelengthInfo getWavelengthInfo(int frequency, bool* isHost = nullptr) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        WavelengthInfo info = registry->getWavelengthInfo(frequency);

        if (isHost) {
            *isHost = info.isHost;
        }

        return info;
    }

    int getActiveWavelength() {
        return WavelengthRegistry::getInstance()->getActiveWavelength();
    }

    void setActiveWavelength(int frequency) {
        WavelengthRegistry::getInstance()->setActiveWavelength(frequency);
        emit activeWavelengthChanged(frequency);
    }

    bool isActiveWavelengthHost() {
        int frequency = getActiveWavelength();
        if (frequency == -1) return false;

        WavelengthInfo info = getWavelengthInfo(frequency);
        return info.isHost;
    }

    // Pobiera listę częstotliwości do których użytkownik jest dołączony
    QList<int> getJoinedWavelengths() {
        // Ponieważ nie mamy bezpośredniej metody, musimy śledzić dołączone wavelength sami
        QList<int> result;

        // Dodaj zarejestrowane wavelength z naszej własnej listy
        for (const auto& frequency : m_joinedWavelengths) {
            if (WavelengthRegistry::getInstance()->hasWavelength(frequency)) {
                result.append(frequency);
            }
        }

        return result;
    }

    // Rejestruje dołączenie do wavelength
    void registerJoinedWavelength(int frequency) {
        if (!m_joinedWavelengths.contains(frequency)) {
            m_joinedWavelengths.append(frequency);
        }
    }

    // Wyrejestrowuje opuszczenie wavelength
    void unregisterJoinedWavelength(int frequency) {
        m_joinedWavelengths.removeOne(frequency);
    }

    // Zwraca liczbę dołączonych wavelength
    int getJoinedWavelengthCount() {
        return getJoinedWavelengths().size();
    }

    bool isWavelengthPasswordProtected(int frequency) {
        WavelengthInfo info = getWavelengthInfo(frequency);
        return info.isPasswordProtected;
    }

    bool isWavelengthHost(int frequency) {
        WavelengthInfo info = getWavelengthInfo(frequency);
        return info.isHost;
    }

    QString getWavelengthName(int frequency) {
        WavelengthInfo info = getWavelengthInfo(frequency);
        return info.name;
    }

    QDateTime getWavelengthCreationTime(int frequency) {
        WavelengthInfo info = getWavelengthInfo(frequency);
        return info.createdAt;
    }

    bool isWavelengthJoined(int frequency) {
        return WavelengthRegistry::getInstance()->hasWavelength(frequency);
    }

    bool isWavelengthConnected(int frequency) {
        WavelengthInfo info = getWavelengthInfo(frequency);
        return info.socket && info.socket->isValid() &&
               info.socket->state() == QAbstractSocket::ConnectedState;
    }

    void addActiveSessionData(int frequency, const QString& key, const QVariant& value) {
        if (!m_sessionData.contains(frequency)) {
            m_sessionData[frequency] = QMap<QString, QVariant>();
        }

        m_sessionData[frequency][key] = value;
    }

    QVariant getActiveSessionData(int frequency, const QString& key, const QVariant& defaultValue = QVariant()) {
        if (!m_sessionData.contains(frequency) || !m_sessionData[frequency].contains(key)) {
            return defaultValue;
        }

        return m_sessionData[frequency][key];
    }

    void clearSessionData(int frequency) {
        if (m_sessionData.contains(frequency)) {
            m_sessionData.remove(frequency);
        }
    }

    void clearAllSessionData() {
        m_sessionData.clear();
    }

    // Obsługa stanu połączenia
    bool isConnecting(int frequency) {
        return m_connectingWavelengths.contains(frequency);
    }

    void setConnecting(int frequency, bool connecting) {
        if (connecting && !m_connectingWavelengths.contains(frequency)) {
            m_connectingWavelengths.append(frequency);
        } else if (!connecting) {
            m_connectingWavelengths.removeOne(frequency);
        }
    }

signals:
    void activeWavelengthChanged(int frequency);

private:
    WavelengthStateManager(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthStateManager() {}

    WavelengthStateManager(const WavelengthStateManager&) = delete;
    WavelengthStateManager& operator=(const WavelengthStateManager&) = delete;

    QMap<int, QMap<QString, QVariant>> m_sessionData;
    QList<int> m_connectingWavelengths;
    QList<int> m_joinedWavelengths; // Lista dołączonych wavelength
};

#endif // WAVELENGTH_STATE_MANAGER_H