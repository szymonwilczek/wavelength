#ifndef WAVELENGTH_EVENT_BROKER_H
#define WAVELENGTH_EVENT_BROKER_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <concepts>

class WavelengthEventBroker final : public QObject {
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
    requires std::derived_from<Receiver, QObject> &&
             std::invocable<Func, const QVariantMap&>
    void subscribeToEvent(const QString& eventType, Receiver* receiver, Func slot);

    // Zdarzenia związane z wavelength
    void wavelengthCreated(const QString &frequency);

    void wavelengthJoined(const QString &frequency);

    void wavelengthLeft(const QString &frequency);

    void wavelengthClosed(const QString &frequency);

    void messageReceived(const QString &frequency, const QString& message);

    void messageSent(const QString &frequency, const QString& message);

    void connectionError(const QString& errorMessage);

    void authenticationFailed(const QString &frequency);

    void userKicked(const QString &frequency, const QString& reason);

    // Zdarzenia związane ze stanem UI
    void uiStateChanged(const QString& component, const QVariant& state);

    // Generyczne zdarzenia systemowe
    void systemMessage(const QString &frequency, const QString& message);

    void activeWavelengthChanged(const QString &frequency);

signals:
    void eventPublished(const QString& eventType, const QVariantMap& data);

private:
    explicit WavelengthEventBroker(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthEventBroker() override {}

    WavelengthEventBroker(const WavelengthEventBroker&) = delete;
    WavelengthEventBroker& operator=(const WavelengthEventBroker&) = delete;
};

#endif // WAVELENGTH_EVENT_BROKER_H
