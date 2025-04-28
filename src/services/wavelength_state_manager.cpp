//
// Created by szymo on 10.03.2025.
//

#include "wavelength_state_manager.h"

WavelengthInfo WavelengthStateManager::getWavelengthInfo(QString frequency, bool *isHost) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    WavelengthInfo info = registry->getWavelengthInfo(frequency);

    if (isHost) {
        *isHost = info.isHost;
    }

    return info;
}

QString WavelengthStateManager::getActiveWavelength() {
    return WavelengthRegistry::getInstance()->getActiveWavelength();
}

void WavelengthStateManager::setActiveWavelength(QString frequency) {
    WavelengthRegistry::getInstance()->setActiveWavelength(frequency);
    emit activeWavelengthChanged(frequency);
}

bool WavelengthStateManager::isActiveWavelengthHost() {
    QString frequency = getActiveWavelength();
    if (frequency == "-1") return false;

    WavelengthInfo info = getWavelengthInfo(frequency);
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

void WavelengthStateManager::registerJoinedWavelength(QString frequency) {
    if (!m_joinedWavelengths.contains(frequency)) {
        m_joinedWavelengths.append(frequency);
    }
}

void WavelengthStateManager::unregisterJoinedWavelength(QString frequency) {
    m_joinedWavelengths.removeOne(frequency);
}

int WavelengthStateManager::getJoinedWavelengthCount() {
    return getJoinedWavelengths().size();
}

bool WavelengthStateManager::isWavelengthPasswordProtected(QString frequency) {
    WavelengthInfo info = getWavelengthInfo(frequency);
    return info.isPasswordProtected;
}

bool WavelengthStateManager::isWavelengthHost(QString frequency) {
    WavelengthInfo info = getWavelengthInfo(frequency);
    return info.isHost;
}

QDateTime WavelengthStateManager::getWavelengthCreationTime(QString frequency) {
    WavelengthInfo info = getWavelengthInfo(frequency);
    return info.createdAt;
}

bool WavelengthStateManager::isWavelengthJoined(QString frequency) {
    return WavelengthRegistry::getInstance()->hasWavelength(frequency);
}

bool WavelengthStateManager::isWavelengthConnected(QString frequency) {
    WavelengthInfo info = getWavelengthInfo(frequency);
    return info.socket && info.socket->isValid() &&
           info.socket->state() == QAbstractSocket::ConnectedState;
}

void WavelengthStateManager::addActiveSessionData(QString frequency, const QString &key, const QVariant &value) {
    if (!m_sessionData.contains(frequency)) {
        m_sessionData[frequency] = QMap<QString, QVariant>();
    }

    m_sessionData[frequency][key] = value;
}

QVariant WavelengthStateManager::getActiveSessionData(QString frequency, const QString &key,
    const QVariant &defaultValue) {
    if (!m_sessionData.contains(frequency) || !m_sessionData[frequency].contains(key)) {
        return defaultValue;
    }

    return m_sessionData[frequency][key];
}

void WavelengthStateManager::clearSessionData(QString frequency) {
    if (m_sessionData.contains(frequency)) {
        m_sessionData.remove(frequency);
    }
}

void WavelengthStateManager::clearAllSessionData() {
    m_sessionData.clear();
}

bool WavelengthStateManager::isConnecting(QString frequency) {
    return m_connectingWavelengths.contains(frequency);
}

void WavelengthStateManager::setConnecting(QString frequency, bool connecting) {
    if (connecting && !m_connectingWavelengths.contains(frequency)) {
        m_connectingWavelengths.append(frequency);
    } else if (!connecting) {
        m_connectingWavelengths.removeOne(frequency);
    }
}
