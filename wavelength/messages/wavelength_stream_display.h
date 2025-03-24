#ifndef WAVELENGTH_STREAM_DISPLAY_H
#define WAVELENGTH_STREAM_DISPLAY_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>

#include "../ui/chat/communication_stream.h"

class WavelengthStreamDisplay : public QWidget {
    Q_OBJECT

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

        // Czyścimy wszystkie wiadomości
        m_communicationStream->clearMessages();
        m_messageQueue.clear();
    }

    void addMessage(const QString& message, const QString& messageId, StreamMessage::MessageType type) {
        // Dodajemy wiadomość do kolejki
        MessageData data;
        data.content = message;
        data.id = messageId;
        data.type = type;

        // Wyodrębniamy dane nadawcy
        if (message.contains("<span style=")) {
            QRegularExpression re("<span[^>]*>([^<]+)</span>");
            QRegularExpressionMatch match = re.match(message);
            if (match.hasMatch()) {
                data.sender = match.captured(1);
                // Usuwamy dwukropek jeśli istnieje
                if (data.sender.endsWith(":"))
                    data.sender = data.sender.left(data.sender.length() - 1);
            } else {
                data.sender = (type == StreamMessage::Transmitted) ? "You" : "Unknown";
            }
        } else {
            data.sender = (type == StreamMessage::Transmitted) ? "You" :
                         (type == StreamMessage::System) ? "SYSTEM" : "Unknown";
        }

        // Czyścimy treść wiadomości z tagów HTML
        QString cleanContent = message;
        cleanContent.remove(QRegularExpression("<[^>]*>"));
        data.content = cleanContent;

        m_messageQueue.enqueue(data);

        // Jeśli to pierwsza wiadomość w kolejce, rozpoczynamy przetwarzanie
        if (m_messageQueue.size() == 1 && !m_messageTimer->isActive()) {
            // Krótkie opóźnienie przed wyświetleniem pierwszej wiadomości
            m_messageTimer->start(300);
        }
    }

    void clear() {
        // Czyścimy wszystkie wiadomości
        m_communicationStream->clearMessages();
        m_messageQueue.clear();
        m_messageTimer->stop();
    }

public slots:
    void onMessageReceived(double frequency, const QString& message) {
        // Określamy typ wiadomości
        StreamMessage::MessageType type;
        if (message.contains("[You]:") || message.contains("color:#60ff8a;")) {
            type = StreamMessage::Transmitted;
        } else if (message.contains("color:#ffcc00;")) {
            type = StreamMessage::System;
        } else {
            type = StreamMessage::Received;
        }

        // Wyciągamy treść i nadawcę z HTML
        QString sender = "Unknown";
        QString content = message;

        // Typowe dane nadawcy znajdują się w spanie na początku wiadomości
        int senderStart = message.indexOf("<span style=\"color:");
        int senderEnd = message.indexOf("</span>");

        if (senderStart >= 0 && senderEnd > senderStart) {
            // Wyciągamy dane nadawcy
            QString senderHtml = message.mid(senderStart, senderEnd - senderStart + 7);
            sender = senderHtml;
            sender.remove(QRegExp("<[^>]*>"));

            // Usuwamy końcowy dwukropek i dodatkowe spacje
            if (sender.endsWith(":")) {
                sender = sender.left(sender.length() - 1).trimmed();
            }

            // Wyciągamy treść wiadomości
            content = message.mid(senderEnd + 7).trimmed();
        }

        // Generujemy identyfikator wiadomości
        QString id = QString::number(QRandomGenerator::global()->generate64());

        // Dodajemy wiadomość do wyświetlenia
        addMessage(message, sender, type, id);
    }

private slots:
    void processNextQueuedMessage() {
        if (m_messageQueue.isEmpty()) {
            return;
        }

        // Pobieramy pierwszą wiadomość z kolejki
        MessageData data = m_messageQueue.dequeue();

        // Wyświetlamy wiadomość w strumieniu
        StreamMessage* message = nullptr;

        // Tworzymy wiadomość i sprawdzamy, czy zawiera załącznik
        if (data.hasAttachment) {
            // Tworzymy wiadomość z załącznikiem
            message = m_communicationStream->addMessageWithAttachment(
                data.content, data.sender, data.type, data.id);
        } else {
            // Zwykła wiadomość
            message = m_communicationStream->addMessage(
                data.content, data.sender, data.type);
        }

        // Jeśli mamy więcej wiadomości w kolejce, ustawiamy timer na następną
        if (!m_messageQueue.isEmpty()) {
            // Losowe opóźnienie między wiadomościami dla bardziej naturalnego efektu
            int delay = 800 + QRandomGenerator::global()->bounded(1200);
            m_messageTimer->start(delay);
        }
    }

private:
    // Struktura przechowująca dane wiadomości w kolejce
    struct MessageData {
        QString content;
        QString sender;
        QString id;
        StreamMessage::MessageType type;
        bool hasAttachment = false;
    };
    
    CommunicationStream* m_communicationStream;
    QQueue<MessageData> m_messageQueue;
    QTimer* m_messageTimer;

    void addMessage(const QString& rawHtml, const QString& sender, StreamMessage::MessageType type, const QString& messageId) {
        // Sprawdzamy, czy wiadomość zawiera załączniki
        bool hasAttachment = rawHtml.contains("placeholder");

        // Dodaj wiadomość do kolejki
        MessageData data;
        data.content = rawHtml;
        data.sender = sender;
        data.type = type;
        data.id = messageId;
        data.hasAttachment = hasAttachment;

        m_messageQueue.enqueue(data);

        // Jeśli to pierwsza wiadomość w kolejce, rozpoczynamy przetwarzanie
        if (m_messageQueue.size() == 1 && !m_messageTimer->isActive()) {
            // Krótkie opóźnienie przed wyświetleniem pierwszej wiadomości
            m_messageTimer->start(300);
        }
    }

};

#endif // WAVELENGTH_STREAM_DISPLAY_H