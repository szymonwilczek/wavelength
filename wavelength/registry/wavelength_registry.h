#ifndef WAVELENGTH_REGISTRY_H
#define WAVELENGTH_REGISTRY_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QPointer>
#include <QWebSocket>
#include <QDebug>

struct WavelengthInfo {
    double frequency;
    QString name;
    bool isPasswordProtected;
    QString password;
    QString hostId;
    bool isHost;
    QString hostAddress;
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

    bool addWavelength(double frequency, const WavelengthInfo& info) {
        if (hasWavelength(frequency)) {
            qDebug() << "Wavelength" << frequency << "already exists";
            return false;
        }
        
        m_wavelengths[frequency] = info;
        m_wavelengths[frequency].frequency = frequency;  // Ensure frequency is correct
        
        qDebug() << "Added wavelength:" << frequency << info.name 
                 << "protected:" << info.isPasswordProtected;
                 
        if (m_pendingRegistrations.contains(frequency)) {
            m_pendingRegistrations.remove(frequency);
        }
        
        emit wavelengthAdded(frequency);
        return true;
    }

    bool removeWavelength(int frequency) {
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

    bool hasWavelength(int frequency) const {
        return m_wavelengths.contains(frequency);
    }

    WavelengthInfo getWavelengthInfo(int frequency) const {
        if (hasWavelength(frequency)) {
            return m_wavelengths[frequency];
        }
        return WavelengthInfo(); // Return empty info
    }

    QList<int> getAllWavelengths() const {
        return m_wavelengths.keys();
    }

    bool updateWavelength(int frequency, const WavelengthInfo& info) {
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

    void setActiveWavelength(int frequency) {
        if (frequency != -1 && !hasWavelength(frequency)) {
            qDebug() << "Cannot set active - wavelength" << frequency << "does not exist";
            return;
        }
        
        int previousActive = m_activeWavelength;
        m_activeWavelength = frequency;
        
        qDebug() << "Active wavelength changed from" << previousActive << "to" << frequency;
        emit activeWavelengthChanged(previousActive, frequency);
    }

    int getActiveWavelength() const {
        return m_activeWavelength;
    }

    bool isWavelengthActive(int frequency) const {
        return m_activeWavelength == frequency;
    }

    bool addPendingRegistration(int frequency) {
        if (hasWavelength(frequency) || m_pendingRegistrations.contains(frequency)) {
            qDebug() << "Cannot add pending registration - wavelength" 
                     << frequency << "already exists or is pending";
            return false;
        }
        
        m_pendingRegistrations.insert(frequency);
        qDebug() << "Added pending registration for wavelength:" << frequency;
        return true;
    }

    bool removePendingRegistration(int frequency) {
        if (m_pendingRegistrations.contains(frequency)) {
            m_pendingRegistrations.remove(frequency);
            qDebug() << "Removed pending registration for wavelength:" << frequency;
            return true;
        }
        return false;
    }

    bool isPendingRegistration(int frequency) const {
        return m_pendingRegistrations.contains(frequency);
    }

    QPointer<QWebSocket> getWavelengthSocket(int frequency) const {
        if (hasWavelength(frequency)) {
            return m_wavelengths[frequency].socket;
        }
        return nullptr;
    }

    void setWavelengthSocket(int frequency, QPointer<QWebSocket> socket) {
        if (hasWavelength(frequency)) {
            m_wavelengths[frequency].socket = socket;
            qDebug() << "Set socket for wavelength:" << frequency;
        }
    }

    bool markWavelengthClosing(int frequency, bool closing = true) {
        if (!hasWavelength(frequency)) {
            return false;
        }
        
        m_wavelengths[frequency].isClosing = closing;
        qDebug() << "Marked wavelength" << frequency << "as" 
                 << (closing ? "closing" : "not closing");
        return true;
    }

    bool isWavelengthClosing(int frequency) const {
        if (hasWavelength(frequency)) {
            return m_wavelengths[frequency].isClosing;
        }
        return false;
    }

    void clearAllWavelengths() {
        QList<int> frequencies = m_wavelengths.keys();
        
        for (int frequency : frequencies) {
            removeWavelength(frequency);
        }
        
        m_pendingRegistrations.clear();
        m_activeWavelength = -1;
        
        qDebug() << "Cleared all wavelengths";
    }

signals:
    void wavelengthAdded(int frequency);
    void wavelengthRemoved(int frequency);
    void wavelengthUpdated(int frequency);
    void activeWavelengthChanged(int previousFrequency, int newFrequency);

private:
    WavelengthRegistry(QObject* parent = nullptr) : QObject(parent), m_activeWavelength(-1) {}
    ~WavelengthRegistry() {}

    WavelengthRegistry(const WavelengthRegistry&) = delete;
    WavelengthRegistry& operator=(const WavelengthRegistry&) = delete;

    QMap<int, WavelengthInfo> m_wavelengths;
    QSet<int> m_pendingRegistrations;
    int m_activeWavelength;
};

#endif // WAVELENGTH_REGISTRY_H