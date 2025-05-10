#include "wavelength_event_broker.h"
#include <concepts>

template<typename Receiver, typename Func>
    requires std::derived_from<Receiver, QObject> &&
             std::invocable<Func, const QVariantMap &>
void WavelengthEventBroker::SubscribeToEvent(const QString &event_type, Receiver *receiver, Func slot) {
    connect(this, &WavelengthEventBroker::eventPublished,
            receiver, [event_type, slot](const QString &type, const QVariantMap &data) {
                if (type == event_type) {
                    (*slot)(data);
                }
            });
}

void WavelengthEventBroker::WavelengthCreated(const QString &frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    PublishEvent("wavelength_created", data);
}

void WavelengthEventBroker::WavelengthJoined(const QString &frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    PublishEvent("wavelength_joined", data);
}

void WavelengthEventBroker::WavelengthLeft(const QString &frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    PublishEvent("wavelength_left", data);
}

void WavelengthEventBroker::WavelengthClosed(const QString &frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    PublishEvent("wavelength_closed", data);
}

void WavelengthEventBroker::MessageReceived(const QString &frequency, const QString &message) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["message"] = message;
    PublishEvent("message_received", data);
}

void WavelengthEventBroker::MessageSent(const QString &frequency, const QString &message) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["message"] = message;
    PublishEvent("message_sent", data);
}

void WavelengthEventBroker::ConnectionError(const QString &error_message) {
    QVariantMap data;
    data["error"] = error_message;
    PublishEvent("connection_error", data);
}

void WavelengthEventBroker::AuthenticationFailed(const QString &frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    PublishEvent("authentication_failed", data);
}

void WavelengthEventBroker::UserKicked(const QString &frequency, const QString &reason) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["reason"] = reason;
    PublishEvent("user_kicked", data);
}

void WavelengthEventBroker::UiStateChanged(const QString &component, const QVariant &state) {
    QVariantMap data;
    data["component"] = component;
    data["state"] = state;
    PublishEvent("ui_state_changed", data);
}

void WavelengthEventBroker::SystemMessage(const QString &frequency, const QString &message) {
    QVariantMap data;
    data["frequency"] = frequency;
    data["message"] = message;
    PublishEvent("system_message", data);
}

void WavelengthEventBroker::ActiveWavelengthChanged(const QString &frequency) {
    QVariantMap data;
    data["frequency"] = frequency;
    PublishEvent("active_wavelength_changed", data);
}
