#ifndef WAVELENGTH_EVENT_BROKER_H
#define WAVELENGTH_EVENT_BROKER_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <concepts>

class WavelengthEventBroker final : public QObject {
    Q_OBJECT

public:
    static WavelengthEventBroker* GetInstance() {
        static WavelengthEventBroker instance;
        return &instance;
    }
    
    // Publikuj zdarzenie
    void PublishEvent(const QString& event_type, const QVariantMap& data = QVariantMap()) {
        qDebug() << "Publishing event:" << event_type;
        emit eventPublished(event_type, data);
    }
    
    // Podłączanie odbiorników zdarzeń
    template<typename Receiver, typename Func>
    requires std::derived_from<Receiver, QObject> &&
             std::invocable<Func, const QVariantMap&>
    void SubscribeToEvent(const QString& event_type, Receiver* receiver, Func slot);

    void WavelengthCreated(const QString &frequency);

    void WavelengthJoined(const QString &frequency);

    void WavelengthLeft(const QString &frequency);

    void WavelengthClosed(const QString &frequency);

    void MessageReceived(const QString &frequency, const QString& message);

    void MessageSent(const QString &frequency, const QString& message);

    void ConnectionError(const QString& error_message);

    void AuthenticationFailed(const QString &frequency);

    void UserKicked(const QString &frequency, const QString& reason);

    // Zdarzenia związane ze stanem UI
    void UiStateChanged(const QString& component, const QVariant& state);

    // Generyczne zdarzenia systemowe
    void SystemMessage(const QString &frequency, const QString& message);

    void ActiveWavelengthChanged(const QString &frequency);

signals:
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    explicit WavelengthEventBroker(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthEventBroker() override {}

    WavelengthEventBroker(const WavelengthEventBroker&) = delete;
    WavelengthEventBroker& operator=(const WavelengthEventBroker&) = delete;
};

#endif // WAVELENGTH_EVENT_BROKER_H
