#include "wavelength_state_manager.h"

WavelengthInfo WavelengthStateManager::getWavelengthInfo(const QString &frequency, bool *isHost) {
    const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    WavelengthInfo info = registry->getWavelengthInfo(frequency);

    if (isHost) {
        *isHost = info.isHost;
    }

    return info;
}

QString WavelengthStateManager::getActiveWavelength() {
    return WavelengthRegistry::getInstance()->getActiveWavelength();
}

void WavelengthStateManager::setActiveWavelength(const QString &frequency) {
    WavelengthRegistry::getInstance()->setActiveWavelength(frequency);
    emit activeWavelengthChanged(frequency);
}

bool WavelengthStateManager::isActiveWavelengthHost() {
    const QString frequency = getActiveWavelength();
    if (frequency == "-1") return false;

    const WavelengthInfo info = getWavelengthInfo(frequency);
    return info.isHost;
}

QList<QString> WavelengthStateManager::getJoinedWavelengths() {
    // Ponieważ nie mamy bezpośredniej metody, musimy śledzić dołączone wavelength sami
    QList<QString> result;

    // Dodaj zarejestrowane wavelength z naszej własnej listy
    for (const auto& frequency : m_joinedWavelengths) {
        if (WavelengthRegistry::getInstance()->hasWavelength(frequency)) {
            result.append(frequency);
        }
    }

    return result;
}

void WavelengthStateManager::registerJoinedWavelength(const QString &frequency) {
    if (!m_joinedWavelengths.contains(frequency)) {
        m_joinedWavelengths.append(frequency);
    }
}

void WavelengthStateManager::unregisterJoinedWavelength(const QString &frequency) {
    m_joinedWavelengths.removeOne(frequency);
}

int WavelengthStateManager::getJoinedWavelengthCount() {
    return getJoinedWavelengths().size();
}

bool WavelengthStateManager::isWavelengthPasswordProtected(const QString &frequency) {
    const WavelengthInfo info = getWavelengthInfo(frequency);
    return info.isPasswordProtected;
}

bool WavelengthStateManager::isWavelengthHost(const QString &frequency) {
    const WavelengthInfo info = getWavelengthInfo(frequency);
    return info.isHost;
}

QDateTime WavelengthStateManager::getWavelengthCreationTime(const QString &frequency) {
    WavelengthInfo info = getWavelengthInfo(frequency);
    return info.createdAt;
}

bool WavelengthStateManager::isWavelengthJoined(const QString &frequency) {
    return WavelengthRegistry::getInstance()->hasWavelength(frequency);
}

bool WavelengthStateManager::isWavelengthConnected(const QString &frequency) {
    const WavelengthInfo info = getWavelengthInfo(frequency);
    return info.socket && info.socket->isValid() &&
           info.socket->state() == QAbstractSocket::ConnectedState;
}

void WavelengthStateManager::addActiveSessionData(const QString &frequency, const QString &key, const QVariant &value) {
    if (!m_sessionData.contains(frequency)) {
        m_sessionData[frequency] = QMap<QString, QVariant>();
    }

    m_sessionData[frequency][key] = value;
}

QVariant WavelengthStateManager::getActiveSessionData(const QString &frequency, const QString &key,
    const QVariant &defaultValue) {
    if (!m_sessionData.contains(frequency) || !m_sessionData[frequency].contains(key)) {
        return defaultValue;
    }

    return m_sessionData[frequency][key];
}

void WavelengthStateManager::clearSessionData(const QString &frequency) {
    if (m_sessionData.contains(frequency)) {
        m_sessionData.remove(frequency);
    }
}

void WavelengthStateManager::clearAllSessionData() {
    m_sessionData.clear();
}

bool WavelengthStateManager::isConnecting(const QString &frequency) const {
    return m_connectingWavelengths.contains(frequency);
}

void WavelengthStateManager::setConnecting(const QString &frequency, const bool connecting) {
    if (connecting && !m_connectingWavelengths.contains(frequency)) {
        m_connectingWavelengths.append(frequency);
    } else if (!connecting) {
        m_connectingWavelengths.removeOne(frequency);
    }
}
