//
// Created by szymo on 10.03.2025.
//

#include "wavelength_registry.h"

bool WavelengthRegistry::addWavelength(QString frequency, const WavelengthInfo &info) {
    if (hasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "already exists";
        return false;
    }

    m_wavelengths[frequency] = info;
    m_wavelengths[frequency].frequency = frequency;  // Ensure frequency is correct

    qDebug() << "Added wavelength:" << frequency
            << "protected:" << info.isPasswordProtected;

    if (m_pendingRegistrations.contains(frequency)) {
        m_pendingRegistrations.remove(frequency);
    }

    emit wavelengthAdded(frequency);
    return true;
}

bool WavelengthRegistry::removeWavelength(QString frequency) {
    if (!hasWavelength(frequency)) {
        qDebug() << "Cannot remove - wavelength" << frequency << "does not exist";
        return false;
    }

    // If this is the active wavelength, clear it
    if (m_activeWavelength == frequency) {
        m_activeWavelength = -1;
    }

    m_wavelengths.remove(frequency);
    qDebug() << "Removed wavelength:" << frequency;

    emit wavelengthRemoved(frequency);
    return true;
}

WavelengthInfo WavelengthRegistry::getWavelengthInfo(QString frequency) const {
    if (hasWavelength(frequency)) {
        return m_wavelengths[frequency];
    }
    return WavelengthInfo(); // Return empty info
}

bool WavelengthRegistry::updateWavelength(QString frequency, const WavelengthInfo &info) {
    if (!hasWavelength(frequency)) {
        qDebug() << "Cannot update - wavelength" << frequency << "does not exist";
        return false;
    }

    m_wavelengths[frequency] = info;
    m_wavelengths[frequency].frequency = frequency;  // Ensure frequency is correct

    qDebug() << "Updated wavelength:" << frequency;
    emit wavelengthUpdated(frequency);
    return true;
}

void WavelengthRegistry::setActiveWavelength(QString frequency) {
    if (frequency != -1 && !hasWavelength(frequency)) {
        qDebug() << "Cannot set active - wavelength" << frequency << "does not exist";
        return;
    }

    QString previousActive = m_activeWavelength;
    m_activeWavelength = frequency;

    qDebug() << "Active wavelength changed from" << previousActive << "to" << frequency;
    emit activeWavelengthChanged(previousActive, frequency);
}

bool WavelengthRegistry::addPendingRegistration(QString frequency) {
    if (hasWavelength(frequency) || m_pendingRegistrations.contains(frequency)) {
        qDebug() << "Cannot add pending registration - wavelength"
                << frequency << "already exists or is pending";
        return false;
    }

    m_pendingRegistrations.insert(frequency);
    qDebug() << "Added pending registration for wavelength:" << frequency;
    return true;
}

bool WavelengthRegistry::removePendingRegistration(QString frequency) {
    if (m_pendingRegistrations.contains(frequency)) {
        m_pendingRegistrations.remove(frequency);
        qDebug() << "Removed pending registration for wavelength:" << frequency;
        return true;
    }
    return false;
}

QPointer<QWebSocket> WavelengthRegistry::getWavelengthSocket(QString frequency) const {
    if (hasWavelength(frequency)) {
        return m_wavelengths[frequency].socket;
    }
    return nullptr;
}

void WavelengthRegistry::setWavelengthSocket(QString frequency, QPointer<QWebSocket> socket) {
    if (hasWavelength(frequency)) {
        m_wavelengths[frequency].socket = socket;
        qDebug() << "Set socket for wavelength:" << frequency;
    }
}

bool WavelengthRegistry::markWavelengthClosing(QString frequency, bool closing) {
    if (!hasWavelength(frequency)) {
        return false;
    }

    m_wavelengths[frequency].isClosing = closing;
    qDebug() << "Marked wavelength" << frequency << "as"
            << (closing ? "closing" : "not closing");
    return true;
}

bool WavelengthRegistry::isWavelengthClosing(QString frequency) const {
    if (hasWavelength(frequency)) {
        return m_wavelengths[frequency].isClosing;
    }
    return false;
}

void WavelengthRegistry::clearAllWavelengths() {
    QList<QString> frequencies = m_wavelengths.keys();

    for (QString frequency : frequencies) {
        removeWavelength(frequency);
    }

    m_pendingRegistrations.clear();
    m_activeWavelength = -1;

    qDebug() << "Cleared all wavelengths";
}
