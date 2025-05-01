#ifndef WAVELENGTH_STATE_MANAGER_H
#define WAVELENGTH_STATE_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>

#include "../storage/wavelength_registry.h"

class WavelengthStateManager final : public QObject {
    Q_OBJECT

public:
    static WavelengthStateManager* GetInstance() {
        static WavelengthStateManager instance;
        return &instance;
    }

    static WavelengthInfo GetWavelengthInfo(const QString &frequency, bool* is_host = nullptr);

    static QString GetActiveWavelength();

    void SetActiveWavelength(const QString &frequency);

    static bool IsActiveWavelengthHost();

    // Pobiera listę częstotliwości do których użytkownik jest dołączony
    QList<QString> GetJoinedWavelengths();

    // Rejestruje dołączenie do wavelength
    void RegisterJoinedWavelength(const QString &frequency);

    // Wyrejestrowuje opuszczenie wavelength
    void UnregisterJoinedWavelength(const QString &frequency);

    // Zwraca liczbę dołączonych wavelength
    int GetJoinedWavelengthCount();

    static bool IsWavelengthPasswordProtected(const QString &frequency);

    static bool IsWavelengthHost(const QString &frequency);

    static QDateTime GetWavelengthCreationTime(const QString &frequency);

    static bool IsWavelengthJoined(const QString &frequency);

    static bool IsWavelengthConnected(const QString &frequency);

    void AddActiveSessionData(const QString &frequency, const QString& key, const QVariant& value);

    QVariant GetActiveSessionData(const QString &frequency, const QString& key, const QVariant& default_value = QVariant());

    void ClearSessionData(const QString &frequency);

    void ClearAllSessionData();

    bool IsConnecting(const QString &frequency) const;

    void SetConnecting(const QString &frequency, bool connecting);

signals:
    void activeWavelengthChanged(QString frequency);

private:
    explicit WavelengthStateManager(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthStateManager() override {}

    WavelengthStateManager(const WavelengthStateManager&) = delete;
    WavelengthStateManager& operator=(const WavelengthStateManager&) = delete;

    QMap<QString, QMap<QString, QVariant>> session_data_;
    QList<QString> connecting_wavelengths_;
    QList<QString> joined_wavelengths_;
};

#endif // WAVELENGTH_STATE_MANAGER_H