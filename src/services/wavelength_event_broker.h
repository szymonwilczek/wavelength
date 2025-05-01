#ifndef WAVELENGTH_EVENT_BROKER_H
#define WAVELENGTH_EVENT_BROKER_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <concepts>

/**
 * @brief A singleton event broker for decoupling components within the Wavelength application.
 *
 * This class implements a publish-subscribe pattern using Qt's signals and slots.
 * Various parts of the application can publish events (e.g., "wavelength_created", "message_received")
 * with associated data (QVariantMap). Other components can subscribe to specific event types
 * and react accordingly, without needing direct references to the publisher.
 * This promotes loose coupling and modularity.
 */
class WavelengthEventBroker final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthEventBroker.
     * @return Pointer to the singleton WavelengthEventBroker instance.
     */
    static WavelengthEventBroker* GetInstance() {
        static WavelengthEventBroker instance;
        return &instance;
    }

    /**
     * @brief Publishes an event of a specific type with associated data.
     * Emits the eventPublished signal, which subscribed components can connect to.
     * @param event_type A string identifying the type of event (e.g., "message_received").
     * @param data Optional QVariantMap containing data associated with the event.
     */
    void PublishEvent(const QString& event_type, const QVariantMap& data = QVariantMap()) {
        qDebug() << "Publishing event:" << event_type << data; // Added data logging
        emit eventPublished(event_type, data);
    }

    /**
     * @brief Subscribes a receiver object's slot to a specific event type.
     * Uses a template to ensure the receiver is a QObject derivative and the slot
     * is invocable with a const QVariantMap&. Connects the internal eventPublished
     * signal to the receiver's slot, filtering by event_type.
     * @tparam Receiver The type of the receiving object (must derive from QObject).
     * @tparam Func The type of the slot/lambda function (must accept const QVariantMap&).
     * @param event_type The string identifier of the event to subscribe to.
     * @param receiver Pointer to the QObject instance that will receive the event.
     * @param slot A pointer to a member function or a lambda function to be called when the event occurs.
     */
    template<typename Receiver, typename Func>
    requires std::derived_from<Receiver, QObject> &&
             std::invocable<Func, const QVariantMap&>
    void SubscribeToEvent(const QString& event_type, Receiver* receiver, Func slot);

    // --- Specific Event Publishing Methods ---

    /** @brief Publishes a "wavelength_created" event. Data: {"frequency": QString}. */
    void WavelengthCreated(const QString &frequency);
    /** @brief Publishes a "wavelength_joined" event. Data: {"frequency": QString}. */
    void WavelengthJoined(const QString &frequency);
    /** @brief Publishes a "wavelength_left" event. Data: {"frequency": QString}. */
    void WavelengthLeft(const QString &frequency);
    /** @brief Publishes a "wavelength_closed" event. Data: {"frequency": QString}. */
    void WavelengthClosed(const QString &frequency);
    /** @brief Publishes a "message_received" event. Data: {"frequency": QString, "message": QString}. */
    void MessageReceived(const QString &frequency, const QString& message);
    /** @brief Publishes a "message_sent" event. Data: {"frequency": QString, "message": QString}. */
    void MessageSent(const QString &frequency, const QString& message);
    /** @brief Publishes a "connection_error" event. Data: {"error": QString}. */
    void ConnectionError(const QString& error_message);
    /** @brief Publishes an "authentication_failed" event. Data: {"frequency": QString}. */
    void AuthenticationFailed(const QString &frequency);
    /** @brief Publishes a "user_kicked" event. Data: {"frequency": QString, "reason": QString}. */
    void UserKicked(const QString &frequency, const QString& reason);
    /** @brief Publishes a "ui_state_changed" event. Data: {"component": QString, "state": QVariant}. */
    void UiStateChanged(const QString& component, const QVariant& state);
    /** @brief Publishes a "system_message" event. Data: {"frequency": QString, "message": QString}. */
    void SystemMessage(const QString &frequency, const QString& message);
    /** @brief Publishes an "active_wavelength_changed" event. Data: {"frequency": QString}. */
    void ActiveWavelengthChanged(const QString &frequency);

signals:
    /**
     * @brief Signal emitted whenever any event is published via PublishEvent().
     * Subscribed components connect to this signal and filter based on eventType.
     * @param eventType The string identifier of the published event.
     * @param data The QVariantMap containing data associated with the event.
     */
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit WavelengthEventBroker(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Private destructor.
     */
    ~WavelengthEventBroker() override = default; // Use default destructor

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    WavelengthEventBroker(const WavelengthEventBroker&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    WavelengthEventBroker& operator=(const WavelengthEventBroker&) = delete;
};
#endif // WAVELENGTH_EVENT_BROKER_H