#ifndef WAVELENGTH_STREAM_DISPLAY_H
#define WAVELENGTH_STREAM_DISPLAY_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QMap> // Dodajemy QMap do śledzenia wiadomości progresu

#include "../ui/chat/communication_stream.h" // Zawiera StreamMessage

class WavelengthStreamDisplay : public QWidget {
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
    WavelengthStreamDisplay(QWidget* parent = nullptr) : QWidget(parent) {
        // Konfigurujemy główny układ
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Tworzymy strumień komunikacji (główny element)
        m_communicationStream = new CommunicationStream(this);
        m_communicationStream->setMinimumHeight(300);
        mainLayout->addWidget(m_communicationStream, 1);

        // Kolejka wiadomości oczekujących na wyświetlenie
        m_messageQueue = QQueue<MessageData>();

        // Mapa do śledzenia *wyświetlonych* wiadomości progresu po ich ID
        m_displayedProgressMessages = QMap<QString, StreamMessage*>();

        // Timer dla opóźnionego wyświetlania wiadomości
        m_messageTimer = new QTimer(this);
        m_messageTimer->setSingleShot(true);
        connect(m_messageTimer, &QTimer::timeout, this, &WavelengthStreamDisplay::processNextQueuedMessage);
    }

    void setFrequency(double frequency, const QString& name = QString()) {
        // Ustawiamy nazwę strumienia
        if (name.isEmpty()) {
            m_communicationStream->setStreamName(QString("WAVELENGTH: %1 Hz").arg(frequency));
        } else {
            m_communicationStream->setStreamName(QString("%1 (%2 Hz)").arg(name).arg(frequency));
        }

        // Czyścimy wszystko
        clear();
    }

    // Główna metoda publiczna do dodawania/aktualizowania wiadomości
    void addMessage(const QString& message, const QString& messageId, StreamMessage::MessageType type) {
        // --- NOWA LOGIKA AKTUALIZACJI ---
        // Sprawdzamy, czy to wiadomość progresu (ma ID) i czy już jest wyświetlona
        if (!messageId.isEmpty() && m_displayedProgressMessages.contains(messageId)) {
            StreamMessage* existingMessage = m_displayedProgressMessages.value(messageId);
            if (existingMessage) {
                qDebug() << "Aktualizowanie istniejącej wiadomości progresu ID:" << messageId;
                existingMessage->updateContent(message);
                return; // Zaktualizowano, nie dodajemy do kolejki
            } else {
                // Wskaźnik jest null lub nieprawidłowy, usuwamy z mapy
                qWarning() << "Usuwanie nieprawidłowego wskaźnika z mapy dla ID:" << messageId;
                m_displayedProgressMessages.remove(messageId);
            }
        }
        // --- KONIEC LOGIKI AKTUALIZACJI ---

        // Jeśli nie aktualizowaliśmy, dodajemy nową wiadomość do kolejki
        qDebug() << "Dodawanie nowej wiadomości do kolejki, ID:" << messageId;
        MessageData data;
        data.content = message; // Przekazujemy oryginalny HTML
        data.id = messageId;    // Zachowujemy ID
        data.type = type;

        // Sprawdzamy, czy wiadomość zawiera załączniki (na podstawie HTML)
        data.hasAttachment = message.contains("image-placeholder") ||
                              message.contains("video-placeholder") ||
                              message.contains("audio-placeholder") ||
                              message.contains("gif-placeholder");

        // Wyodrębniamy dane nadawcy (prostsza logika dla wiadomości systemowych/progresu)
        if (type == StreamMessage::Transmitted && messageId.isEmpty()) { // Zwykła wiadomość wysłana
             data.sender = "You";
        } else if (type == StreamMessage::Received) { // Zwykła wiadomość odebrana
             // Logika wyciągania nadawcy z HTML (jeśli potrzebna dla odebranych)
             QRegularExpression re("<span[^>]*>([^<]+):</span>"); // Szukamy "Nadawca:"
             QRegularExpressionMatch match = re.match(message);
             if (match.hasMatch()) {
                 data.sender = match.captured(1);
             } else {
                 data.sender = "Unknown";
             }
        } else { // Wiadomość systemowa lub progresu
             data.sender = "SYSTEM"; // Lub pusty, jeśli nie chcemy pokazywać nadawcy
        }


        // Dodajemy do kolejki
        m_messageQueue.enqueue(data);

        // Jeśli to pierwsza wiadomość w kolejce i timer nie działa, startujemy
        if (m_messageQueue.size() == 1 && !m_messageTimer->isActive()) {
            m_messageTimer->start(100); // Krótsze opóźnienie dla szybszej reakcji
        }
    }


    void clear() {
        // CommunicationStream::clearMessages zajmie się usunięciem widgetów
        m_communicationStream->clearMessages();
        m_messageQueue.clear();
        m_displayedProgressMessages.clear(); // Czyścimy mapę wskaźników
        m_messageTimer->stop();
    }

public slots:
    void setGlitchIntensity(qreal intensity) {
        m_communicationStream->setGlitchIntensity(intensity);
    }

    // --- NOWE SLOTY PRZEKAZUJĄCE ---
    void setTransmittingUser(const QString& userId) {
        m_communicationStream->setTransmittingUser(userId);
    }

    void clearTransmittingUser() {
        m_communicationStream->clearTransmittingUser();
    }

    void setAudioAmplitude(qreal amplitude) {
        m_communicationStream->setAudioAmplitude(amplitude);
    }

private slots:
    void processNextQueuedMessage() {
        if (m_messageQueue.isEmpty()) {
            return;
        }
        MessageData data = m_messageQueue.dequeue();
        StreamMessage* displayedMessage = nullptr;

        // Bezpiecznik - nie powinien być potrzebny, jeśli onStreamMessageDestroyed działa
        if (!data.id.isEmpty() && m_displayedProgressMessages.contains(data.id)) {
             qWarning() << "Próba dodania wiadomości z ID, które już istnieje w mapie:" << data.id;
             displayedMessage = m_displayedProgressMessages.value(data.id);
             if(displayedMessage) {
                 displayedMessage->updateContent(data.content); // Aktualizuj na wszelki wypadek
             } else {
                 m_displayedProgressMessages.remove(data.id); // Usuń zły wpis
                 displayedMessage = nullptr; // Wymuś utworzenie poniżej
             }
        }

        // Jeśli nie aktualizowaliśmy, dodajemy nową wiadomość do strumienia wizualnego
        if (!displayedMessage) {
            qDebug() << "Przetwarzanie wiadomości z kolejki: " << data.content.left(30)
                     << "... ID=" << data.id << " hasAttachment=" << data.hasAttachment;

            // Wywołaj CommunicationStream, aby dodał wiadomość wizualnie, przekazując ID
            if (data.hasAttachment) {
                displayedMessage = m_communicationStream->addMessageWithAttachment(
                    data.content, data.sender, data.type, data.id); // Przekaż ID
            } else {
                displayedMessage = m_communicationStream->addMessage(
                    data.content, data.sender, data.type, data.id); // Przekaż ID
            }
        }

        // Jeśli wiadomość została pomyślnie utworzona/wyświetlona przez CommunicationStream
        if (displayedMessage) {
            // Jeśli to wiadomość progresu (ma ID), dodaj do mapy i podłącz sygnał destroyed
            if (!data.id.isEmpty()) {
                // Sprawdźmy jeszcze raz, czy ID nie zostało dodane w międzyczasie
                if (!m_displayedProgressMessages.contains(data.id)) {
                    qDebug() << "Dodano wiadomość progresu do mapy, ID:" << data.id;
                    m_displayedProgressMessages.insert(data.id, displayedMessage);
                    // Podłącz sygnał zniszczenia do slotu czyszczącego mapę
                    connect(displayedMessage, &QObject::destroyed, this, &WavelengthStreamDisplay::onStreamMessageDestroyed);
                } else {
                     qWarning() << "Wiadomość progresu z ID" << data.id << "już istnieje w mapie podczas dodawania!";
                }
            }
            // Dla zwykłych wiadomości (puste ID) nic nie robimy z mapą ani sygnałem
        } else {
             qWarning() << "CommunicationStream nie utworzył/zwrócił widgetu wiadomości dla ID:" << data.id;
        }

        // Uruchom timer dla następnej wiadomości w kolejce
        if (!m_messageQueue.isEmpty()) {
            int delay = 300 + QRandomGenerator::global()->bounded(500);
            m_messageTimer->start(delay);
        }
    }

    void onStreamMessageDestroyed(QObject* obj) {
        // Iterujemy po mapie, aby znaleźć wpis pasujący do zniszczonego obiektu
        // To jest bezpieczniejsze niż poleganie na ID, jeśli jakimś cudem ID nie zostało ustawione
        auto it = m_displayedProgressMessages.begin();
        while (it != m_displayedProgressMessages.end()) {
            if (it.value() == obj) {
                qDebug() << "Usuwanie wiadomości progresu z mapy po zniszczeniu, ID:" << it.key();
                it = m_displayedProgressMessages.erase(it); // Usuń wpis i przejdź do następnego
                return; // Znaleziono i usunięto, kończymy
            } else {
                ++it;
            }
        }
        qWarning() << "Otrzymano sygnał destroyed dla obiektu, którego nie ma w mapie wiadomości progresu.";
    }

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