#include "wavelength_state_manager.h"

WavelengthInfo WavelengthStateManager::GetWavelengthInfo(const QString &frequency, bool *is_host) {
    const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    WavelengthInfo info = registry->getWavelengthInfo(frequency);

    if (is_host) {
        *is_host = info.isHost;
    }

    return info;
}

QString WavelengthStateManager::GetActiveWavelength() {
    return WavelengthRegistry::getInstance()->getActiveWavelength();
}

void WavelengthStateManager::SetActiveWavelength(const QString &frequency) {
    WavelengthRegistry::getInstance()->setActiveWavelength(frequency);
    emit activeWavelengthChanged(frequency);
}

bool WavelengthStateManager::IsActiveWavelengthHost() {
    const QString frequency = GetActiveWavelength();
    if (frequency == "-1") return false;

    const WavelengthInfo info = GetWavelengthInfo(frequency);
    return info.isHost;
}

QList<QString> WavelengthStateManager::GetJoinedWavelengths() {
    // Ponieważ nie mamy bezpośredniej metody, musimy śledzić dołączone wavelength sami
    QList<QString> result;

    // Dodaj zarejestrowane wavelength z naszej własnej listy
    for (const auto& frequency : joined_wavelengths_) {
        if (WavelengthRegistry::getInstance()->hasWavelength(frequency)) {
            result.append(frequency);
        }
    }

    return result;
}

void WavelengthStateManager::RegisterJoinedWavelength(const QString &frequency) {
    if (!joined_wavelengths_.contains(frequency)) {
        joined_wavelengths_.append(frequency);
    }
}

void WavelengthStateManager::UnregisterJoinedWavelength(const QString &frequency) {
    joined_wavelengths_.removeOne(frequency);
}

int WavelengthStateManager::GetJoinedWavelengthCount() {
    return GetJoinedWavelengths().size();
}

bool WavelengthStateManager::IsWavelengthPasswordProtected(const QString &frequency) {
    const WavelengthInfo info = GetWavelengthInfo(frequency);
    return info.isPasswordProtected;
}

bool WavelengthStateManager::IsWavelengthHost(const QString &frequency) {
    const WavelengthInfo info = GetWavelengthInfo(frequency);
    return info.isHost;
}

QDateTime WavelengthStateManager::GetWavelengthCreationTime(const QString &frequency) {
    WavelengthInfo info = GetWavelengthInfo(frequency);
    return info.createdAt;
}

bool WavelengthStateManager::IsWavelengthJoined(const QString &frequency) {
    return WavelengthRegistry::getInstance()->hasWavelength(frequency);
}

bool WavelengthStateManager::IsWavelengthConnected(const QString &frequency) {
    const WavelengthInfo info = GetWavelengthInfo(frequency);
    return info.socket && info.socket->isValid() &&
           info.socket->state() == QAbstractSocket::ConnectedState;
}

void WavelengthStateManager::AddActiveSessionData(const QString &frequency, const QString &key, const QVariant &value) {
    if (!session_data_.contains(frequency)) {
        session_data_[frequency] = QMap<QString, QVariant>();
    }

    session_data_[frequency][key] = value;
}

QVariant WavelengthStateManager::GetActiveSessionData(const QString &frequency, const QString &key,
    const QVariant &default_value) {
    if (!session_data_.contains(frequency) || !session_data_[frequency].contains(key)) {
        return default_value;
    }

    return session_data_[frequency][key];
}

void WavelengthStateManager::ClearSessionData(const QString &frequency) {
    if (session_data_.contains(frequency)) {
        session_data_.remove(frequency);
    }
}

void WavelengthStateManager::ClearAllSessionData() {
    session_data_.clear();
}

bool WavelengthStateManager::IsConnecting(const QString &frequency) const {
    return connecting_wavelengths_.contains(frequency);
}

void WavelengthStateManager::SetConnecting(const QString &frequency, const bool connecting) {
    if (connecting && !connecting_wavelengths_.contains(frequency)) {
        connecting_wavelengths_.append(frequency);
    } else if (!connecting) {
        connecting_wavelengths_.removeOne(frequency);
    }
}
