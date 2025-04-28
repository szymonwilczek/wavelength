#ifndef WAVELENGTH_STATE_MANAGER_H
#define WAVELENGTH_STATE_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>

#include "../storage/wavelength_registry.h"

class WavelengthStateManager final : public QObject {
    Q_OBJECT

public:
    static WavelengthStateManager* getInstance() {
        static WavelengthStateManager instance;
        return &instance;
    }

    static WavelengthInfo getWavelengthInfo(const QString &frequency, bool* isHost = nullptr);

    static QString getActiveWavelength();

    void setActiveWavelength(const QString &frequency);

    static bool isActiveWavelengthHost();

    // Pobiera listę częstotliwości do których użytkownik jest dołączony
    QList<QString> getJoinedWavelengths();

    // Rejestruje dołączenie do wavelength
    void registerJoinedWavelength(const QString &frequency);

    // Wyrejestrowuje opuszczenie wavelength
    void unregisterJoinedWavelength(const QString &frequency);

    // Zwraca liczbę dołączonych wavelength
    int getJoinedWavelengthCount();

    static bool isWavelengthPasswordProtected(const QString &frequency);

    static bool isWavelengthHost(const QString &frequency);


    static QDateTime getWavelengthCreationTime(const QString &frequency);

    static bool isWavelengthJoined(const QString &frequency);

    static bool isWavelengthConnected(const QString &frequency);

    void addActiveSessionData(const QString &frequency, const QString& key, const QVariant& value);

    QVariant getActiveSessionData(const QString &frequency, const QString& key, const QVariant& defaultValue = QVariant());

    void clearSessionData(const QString &frequency);

    void clearAllSessionData();

    // Obsługa stanu połączenia
    bool isConnecting(const QString &frequency) const;

    void setConnecting(const QString &frequency, bool connecting);

signals:
    void activeWavelengthChanged(QString frequency);

private:
    explicit WavelengthStateManager(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthStateManager() override {}

    WavelengthStateManager(const WavelengthStateManager&) = delete;
    WavelengthStateManager& operator=(const WavelengthStateManager&) = delete;

    QMap<QString, QMap<QString, QVariant>> m_sessionData;
    QList<QString> m_connectingWavelengths;
    QList<QString> m_joinedWavelengths; // Lista dołączonych wavelength
};

#endif // WAVELENGTH_STATE_MANAGER_H