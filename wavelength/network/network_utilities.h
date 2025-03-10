#ifndef NETWORK_UTILITIES_H
#define NETWORK_UTILITIES_H

#include <QObject>
#include <QString>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkAddressEntry>
#include <QNetworkConfigurationManager>
#include <QDebug>

class NetworkUtilities : public QObject {
    Q_OBJECT

public:
    static NetworkUtilities* getInstance() {
        static NetworkUtilities instance;
        return &instance;
    }

    QString getLocalIpAddress() {
        // Priorytet dla adresów z zakresu 192.168.*.*
        foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                !address.isLoopback() &&
                address.toString().startsWith("192.168.")) {
                return address.toString();
            }
        }

        // Drugi wybór - inne adresy niepętlowe
        foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                !address.isLoopback() &&
                !address.toString().startsWith("169.254.")) {
                return address.toString();
            }
        }

        // Jeśli nie znaleziono odpowiednich adresów, zwróć localhost
        return "127.0.0.1";
    }

    QList<QString> getAllLocalIpAddresses() {
        QList<QString> addresses;
        
        foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
            if (interface.flags().testFlag(QNetworkInterface::IsRunning) &&
                !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
                
                foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        addresses.append(entry.ip().toString());
                    }
                }
            }
        }

        return addresses;
    }

    bool isPortAvailable(int port) {
        QTcpSocket socket;
        socket.connectToHost("127.0.0.1", port);
        
        bool available = !socket.waitForConnected(500);
        
        if (socket.state() == QTcpSocket::ConnectedState) {
            socket.disconnectFromHost();
        }
        
        return available;
    }

    int findAvailablePort(int startPort = 10000, int maxTries = 100) {
        int port = startPort;
        
        for (int i = 0; i < maxTries; ++i) {
            if (isPortAvailable(port)) {
                return port;
            }
            
            port++;
        }
        
        return -1;  // Could not find available port
    }

    bool isNetworkAvailable() {
        QNetworkConfigurationManager manager;
        return manager.isOnline();
    }

    bool canConnectToHost(const QString& host, int port, int timeoutMs = 3000) {
        QTcpSocket socket;
        socket.connectToHost(host, port);
        
        bool success = socket.waitForConnected(timeoutMs);
        
        if (socket.state() == QTcpSocket::ConnectedState) {
            socket.disconnectFromHost();
        }
        
        return success;
    }

    bool testUdpBroadcast(int port) {
        QUdpSocket sendSocket;
        QByteArray datagram = "TEST_BROADCAST";
        
        bool sent = sendSocket.writeDatagram(
            datagram, 
            QHostAddress::Broadcast, 
            port
        ) == datagram.size();
        
        return sent;
    }

    QString getNetworkInterfaceName(const QString& ipAddress) {
        foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                if (entry.ip().toString() == ipAddress) {
                    return interface.humanReadableName();
                }
            }
        }
        
        return QString();
    }

    QString getSubnetMask(const QString& ipAddress) {
        foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
                if (entry.ip().toString() == ipAddress) {
                    return entry.netmask().toString();
                }
            }
        }
        
        return QString();
    }

    bool isValidIpAddress(const QString& address) {
        QHostAddress hostAddress(address);
        return !hostAddress.isNull() && 
               hostAddress.protocol() == QAbstractSocket::IPv4Protocol;
    }

private:
    NetworkUtilities(QObject* parent = nullptr) : QObject(parent) {}
    ~NetworkUtilities() {}

    NetworkUtilities(const NetworkUtilities&) = delete;
    NetworkUtilities& operator=(const NetworkUtilities&) = delete;
};

#endif // NETWORK_UTILITIES_H