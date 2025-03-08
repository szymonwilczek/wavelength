#ifndef WAVELENGTH_MANAGER_H
#define WAVELENGTH_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QDebug>
#include <QUuid>
#include <QTimer>

struct WavelengthInfo {
    int frequency;
    QString name;
    bool isPasswordProtected;
    QString password;
    QString hostId;
    bool isHost;
};

class WavelengthManager : public QObject {
    Q_OBJECT

public:
    static WavelengthManager* getInstance() {
        static WavelengthManager instance;
        return &instance;
    }

    bool isFrequencyAvailable(int frequency) {
        if (frequency < 30 || frequency > 300) {
            qDebug() << "Frequency out of valid range (30-300Hz)";
            return false;
        }

        bool available = !m_wavelengths.contains(frequency);
        qDebug() << "Checking if frequency" << frequency << "is available:" << available;
        return available;
    }

    bool createWavelength(int frequency, const QString& name, bool isPasswordProtected,
                          const QString& password) {
        if (!isFrequencyAvailable(frequency)) {
            qDebug() << "Cannot create wavelength - frequency" << frequency << "is already in use";
            return false;
        }

        WavelengthInfo info;
        info.frequency = frequency;
        info.name = name.isEmpty() ? QString("Wavelength-%1").arg(frequency) : name;
        info.isPasswordProtected = isPasswordProtected;
        info.password = password;
        info.hostId = QUuid::createUuid().toString();
        info.isHost = true;

        qDebug() << "Creating new wavelength server on frequency" << frequency
                 << (isPasswordProtected ? "(password protected)" : "");

        m_wavelengths[frequency] = info;
        m_activeWavelength = frequency;

        emit wavelengthCreated(frequency);
        return true;
    }

    bool joinWavelength(int frequency, const QString& password = QString()) {

        WavelengthInfo info;
        info.frequency = frequency;
        info.name = QString("Wavelength-%1").arg(frequency);
        info.isPasswordProtected = !password.isEmpty();
        info.password = password;
        info.isHost = false;

        qDebug() << "Joining wavelength" << frequency;

        m_wavelengths[frequency] = info;
        m_activeWavelength = frequency;

        emit wavelengthJoined(frequency);
        return true;
    }

    void leaveWavelength() {
        if (m_activeWavelength == -1) {
            qDebug() << "Not connected to any wavelength";
            return;
        }

        int freq = m_activeWavelength;

        if (m_wavelengths.contains(freq) && m_wavelengths[freq].isHost) {
            closeWavelength(freq);
        } else {
            m_wavelengths.remove(freq);
        }

        m_activeWavelength = -1;

        qDebug() << "Left wavelength" << freq;
        emit wavelengthLeft(freq);
    }

    void closeWavelength(int frequency) {
        if (!m_wavelengths.contains(frequency)) {
            qDebug() << "Wavelength" << frequency << "does not exist";
            return;
        }

        if (!m_wavelengths[frequency].isHost) {
            qDebug() << "Cannot close wavelength" << frequency << "- not the host";
            return;
        }

        qDebug() << "Closing wavelength" << frequency;

        m_wavelengths.remove(frequency);

        if (m_activeWavelength == frequency) {
            m_activeWavelength = -1;
        }

        emit wavelengthClosed(frequency);
    }

    void sendMessage(const QString& message) {
        if (m_activeWavelength == -1) {
            qDebug() << "Not connected to any wavelength";
            return;
        }

        qDebug() << "Sending message on wavelength" << m_activeWavelength << ":" << message;

        QTimer::singleShot(100, this, [this, message]() {
            emit messageReceived(m_activeWavelength, message);
        });

        emit messageSent(m_activeWavelength, message);
    }

    int getActiveWavelength() const {
        return m_activeWavelength;
    }

signals:
    void wavelengthCreated(int frequency);
    void wavelengthJoined(int frequency);
    void wavelengthLeft(int frequency);
    void wavelengthClosed(int frequency);
    void messageSent(int frequency, const QString& message);
    void messageReceived(int frequency, const QString& message);

private:
    WavelengthManager(QObject* parent = nullptr) : QObject(parent), m_activeWavelength(-1) {}
    ~WavelengthManager() {
        for (auto it = m_wavelengths.begin(); it != m_wavelengths.end(); ++it) {
            if (it.value().isHost) {
                closeWavelength(it.key());
            }
        }
    }

    WavelengthManager(const WavelengthManager&) = delete;
    WavelengthManager& operator=(const WavelengthManager&) = delete;

    QMap<int, WavelengthInfo> m_wavelengths;
    int m_activeWavelength;
};

#endif // WAVELENGTH_MANAGER_H