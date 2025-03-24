#ifndef WAVELENGTH_STREAM_DISPLAY_H
#define WAVELENGTH_STREAM_DISPLAY_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QQueue>
#include <QRandomGenerator>

#include "../ui/chat/communication_stream.h"

class CommunicationStream;

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
        
        // Wyświetlamy wiadomość powitalną
        QString welcomeMsg = QString("Connected to wavelength %1 Hz at %2")
            .arg(frequency)
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
        
        addMessage(welcomeMsg, "SYSTEM", StreamMessage::System);
    }
    
    void addMessage(const QString& message, const QString& sender, StreamMessage::MessageType type) {
        // Dodajemy wiadomość do kolejki
        MessageData data;
        data.content = message;
        data.sender = sender;
        data.type = type;
        
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
        // Wyciągamy informacje o nadawcy z formatowanej wiadomości HTML
        QString sender = "Unknown";
        QString content = message;
        
        int senderStart = message.indexOf("<span style=\"color:");
        int senderEnd = message.indexOf("</span>");
        
        if (senderStart >= 0 && senderEnd > senderStart) {
            // Wyciągamy dane nadawcy - usuwamy znaczniki HTML
            QString senderHtml = message.mid(senderStart, senderEnd - senderStart + 7);
            sender = senderHtml;
            sender.remove(QRegExp("<[^>]*>"));
            
            // Usuwamy końcowy dwukropek i dodatkowe spacje
            if (sender.endsWith(":")) {
                sender = sender.left(sender.length() - 1).trimmed();
            }
            
            // Wyciągamy treść wiadomości
            content = message.mid(senderEnd + 7).trimmed();
            // Usuwamy pozostałe znaczniki HTML dla lepszej czytelności
            content.remove(QRegExp("<[^>]*>"));
        }
        
        // Określamy typ wiadomości
        StreamMessage::MessageType type = StreamMessage::Received;
        if (message.contains("[You]:")) {
            type = StreamMessage::Transmitted;
        } else if (message.contains("color:#ffcc00;")) {
            type = StreamMessage::System;
        }
        
        // Dodajemy wiadomość do wyświetlenia
        addMessage(content, sender, type);
    }
    
private slots:
    void processNextQueuedMessage() {
        if (m_messageQueue.isEmpty()) {
            return;
        }
        
        // Pobieramy pierwszą wiadomość z kolejki
        MessageData data = m_messageQueue.dequeue();
        
        // Wyświetlamy wiadomość w strumieniu
        m_communicationStream->addMessage(data.content, data.sender, data.type);
        
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
        StreamMessage::MessageType type;
    };
    
    CommunicationStream* m_communicationStream;
    QQueue<MessageData> m_messageQueue;
    QTimer* m_messageTimer;
};

#endif // WAVELENGTH_STREAM_DISPLAY_H