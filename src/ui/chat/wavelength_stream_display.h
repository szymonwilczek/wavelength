#ifndef WAVELENGTH_STREAM_DISPLAY_H
#define WAVELENGTH_STREAM_DISPLAY_H


#include "../../ui/chat/communication_stream.h"

class WavelengthStreamDisplay final : public QWidget {
    Q_OBJECT

    // Struktura przechowująca dane wiadomości w kolejce
    struct MessageData {
        QString content;
        QString sender;
        QString id; // ID jest kluczowe dla aktualizacji
        StreamMessage::MessageType type;
        bool hasAttachment = false;
    };

public:
    explicit WavelengthStreamDisplay(QWidget* parent = nullptr);

    void setFrequency(const QString &frequency, const QString& name = QString());

    // Główna metoda publiczna do dodawania/aktualizowania wiadomości
    void addMessage(const QString& message, const QString& messageId, StreamMessage::MessageType type);


    void clear();

public slots:
    void setGlitchIntensity(const qreal intensity) const {
        m_communicationStream->setGlitchIntensity(intensity);
    }

    // --- NOWE SLOTY PRZEKAZUJĄCE ---
    void setTransmittingUser(const QString& userId) const {
        m_communicationStream->setTransmittingUser(userId);
    }

    void clearTransmittingUser() const {
        m_communicationStream->clearTransmittingUser();
    }

    void setAudioAmplitude(const qreal amplitude) const {
        m_communicationStream->setAudioAmplitude(amplitude);
    }

private slots:
    void processNextQueuedMessage();

    void onStreamMessageDestroyed(const QObject* obj);

private:
    CommunicationStream* m_communicationStream;
    QQueue<MessageData> m_messageQueue;
    QTimer* m_messageTimer;
    // Mapa przechowująca wskaźniki do *aktualnie wyświetlonych* wiadomości progresu
    QMap<QString, StreamMessage*> m_displayedProgressMessages;

    // Usunięta prywatna metoda addMessage, logika przeniesiona do publicznej
    // void addMessage(const QString& rawHtml, const QString& sender, StreamMessage::MessageType type, const QString& messageId) { ... }
};

#endif // WAVELENGTH_STREAM_DISPLAY_H