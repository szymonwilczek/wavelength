#ifndef WAVELENGTH_EVENT_BROKER_H
#define WAVELENGTH_EVENT_BROKER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <functional>

class WavelengthEventBroker : public QObject {
    Q_OBJECT

public:
    static WavelengthEventBroker* getInstance() {
        static WavelengthEventBroker instance;
        return &instance;
    }
    
    // Publikuj zdarzenie
    void publishEvent(const QString& eventType, const QVariantMap& data = QVariantMap()) {
        qDebug() << "Publishing event:" << eventType;
        emit eventPublished(eventType, data);
    }
    
    // Podłączanie odbiorników zdarzeń
    template<typename Receiver, typename Func>
    void subscribeToEvent(const QString& eventType, Receiver* receiver, Func slot);

    // Zdarzenia związane z wavelength
    void wavelengthCreated(QString frequency);

    void wavelengthJoined(QString frequency);

    void wavelengthLeft(QString frequency);

    void wavelengthClosed(QString frequency);

    void messageReceived(QString frequency, const QString& message);

    void messageSent(QString frequency, const QString& message);

    void connectionError(const QString& errorMessage);

    void authenticationFailed(QString frequency);

    void userKicked(QString frequency, const QString& reason);

    // Zdarzenia związane ze stanem UI
    void uiStateChanged(const QString& component, const QVariant& state);

    // Generyczne zdarzenia systemowe
    void systemMessage(QString frequency, const QString& message);

    void activeWavelengthChanged(QString frequency);

signals:
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    WavelengthEventBroker(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthEventBroker() {}

    WavelengthEventBroker(const WavelengthEventBroker&) = delete;
    WavelengthEventBroker& operator=(const WavelengthEventBroker&) = delete;
};

#endif // WAVELENGTH_EVENT_BROKER_H
