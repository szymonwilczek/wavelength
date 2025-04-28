#include "wavelength_event_broker.h"

template<typename Receiver, typename Func>
void WavelengthEventBroker::subscribeToEvent(const QString &eventType, Receiver *receiver, Func slot) {
    connect(this, &WavelengthEventBroker::eventPublished,
            receiver, [eventType, slot](const QString& type, const QVariantMap& data) {
                if (type == eventType) {
                    (*slot)(data);
                }
            });

    qDebug() << "Subscribed to event:" << eventType;
}

void WavelengthEventBroker::wavelengthCreated(QString frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    publishEvent("wavelength_created", data);
}

void WavelengthEventBroker::wavelengthJoined(QString frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    publishEvent("wavelength_joined", data);
}

void WavelengthEventBroker::wavelengthLeft(QString frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    publishEvent("wavelength_left", data);
}

void WavelengthEventBroker::wavelengthClosed(QString frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    publishEvent("wavelength_closed", data);
}

void WavelengthEventBroker::messageReceived(QString frequency, const QString &message) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["message"] = message;
    publishEvent("message_received", data);
}

void WavelengthEventBroker::messageSent(QString frequency, const QString &message) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["message"] = message;
    publishEvent("message_sent", data);
}

void WavelengthEventBroker::connectionError(const QString &errorMessage) {
    QVariantMap data;
    data["error"] = errorMessage;
    publishEvent("connection_error", data);
}

void WavelengthEventBroker::authenticationFailed(QString frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    publishEvent("authentication_failed", data);
}

void WavelengthEventBroker::userKicked(QString frequency, const QString &reason) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["reason"] = reason;
    publishEvent("user_kicked", data);
}

void WavelengthEventBroker::uiStateChanged(const QString &component, const QVariant &state) {
    QVariantMap data;
    data["component"] = component;
    data["state"] = state;
    publishEvent("ui_state_changed", data);
}

void WavelengthEventBroker::systemMessage(QString frequency, const QString &message) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["message"] = message;
    publishEvent("system_message", data);
}

void WavelengthEventBroker::activeWavelengthChanged(QString frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    publishEvent("active_wavelength_changed", data);
}
