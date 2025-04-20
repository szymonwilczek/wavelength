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
    void subscribeToEvent(const QString& eventType, Receiver* receiver, Func slot) {
        connect(this, &WavelengthEventBroker::eventPublished,
                receiver, [eventType, slot](const QString& type, const QVariantMap& data) {
            if (type == eventType) {
                (*slot)(data);
            }
        });
        
        qDebug() << "Subscribed to event:" << eventType;
    }
    
    // Zdarzenia związane z wavelength
    void wavelengthCreated(QString frequency) {
        QVariantMap data;
        data["frequency"] = frequency;
        publishEvent("wavelength_created", data);
    }
    
    void wavelengthJoined(QString frequency) {
        QVariantMap data;
        data["frequency"] = frequency;
        publishEvent("wavelength_joined", data);
    }
    
    void wavelengthLeft(QString frequency) {
        QVariantMap data;
        data["frequency"] = frequency;
        publishEvent("wavelength_left", data);
    }
    
    void wavelengthClosed(QString frequency) {
        QVariantMap data;
        data["frequency"] = frequency;
        publishEvent("wavelength_closed", data);
    }
    
    void messageReceived(QString frequency, const QString& message) {
        QVariantMap data;
        data["frequency"] = frequency;
        data["message"] = message;
        publishEvent("message_received", data);
    }
    
    void messageSent(QString frequency, const QString& message) {
        QVariantMap data;
        data["frequency"] = frequency;
        data["message"] = message;
        publishEvent("message_sent", data);
    }
    
    void connectionError(const QString& errorMessage) {
        QVariantMap data;
        data["error"] = errorMessage;
        publishEvent("connection_error", data);
    }
    
    void authenticationFailed(QString frequency) {
        QVariantMap data;
        data["frequency"] = frequency;
        publishEvent("authentication_failed", data);
    }
    
    void userKicked(QString frequency, const QString& reason) {
        QVariantMap data;
        data["frequency"] = frequency;
        data["reason"] = reason;
        publishEvent("user_kicked", data);
    }
    
    // Zdarzenia związane ze stanem UI
    void uiStateChanged(const QString& component, const QVariant& state) {
        QVariantMap data;
        data["component"] = component;
        data["state"] = state;
        publishEvent("ui_state_changed", data);
    }
    
    // Generyczne zdarzenia systemowe
    void systemMessage(QString frequency, const QString& message) {
        QVariantMap data;
        data["frequency"] = frequency;
        data["message"] = message;
        publishEvent("system_message", data);
    }
    
    void activeWavelengthChanged(QString frequency) {
        QVariantMap data;
        data["frequency"] = frequency;
        publishEvent("active_wavelength_changed", data);
    }

signals:
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    WavelengthEventBroker(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthEventBroker() {}

    WavelengthEventBroker(const WavelengthEventBroker&) = delete;
    WavelengthEventBroker& operator=(const WavelengthEventBroker&) = delete;
};

#endif // WAVELENGTH_EVENT_BROKER_H