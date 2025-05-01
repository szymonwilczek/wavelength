#include "wavelength_stream_display.h"

#include <QRegularExpression>

WavelengthStreamDisplay::WavelengthStreamDisplay(QWidget *parent): QWidget(parent) {
    // Konfigurujemy główny układ
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);

    // Tworzymy strumień komunikacji (główny element)
    communication_stream_ = new CommunicationStream(this);
    communication_stream_->setMinimumHeight(300);
    main_layout->addWidget(communication_stream_, 1);

    // Kolejka wiadomości oczekujących na wyświetlenie
    message_queue_ = QQueue<MessageData>();

    // Mapa do śledzenia *wyświetlonych* wiadomości progresu po ich ID
    displayed_progress_messages_ = QMap<QString, StreamMessage*>();

    // Timer dla opóźnionego wyświetlania wiadomości
    message_timer_ = new QTimer(this);
    message_timer_->setSingleShot(true);
    connect(message_timer_, &QTimer::timeout, this, &WavelengthStreamDisplay::ProcessNextQueuedMessage);
}

void WavelengthStreamDisplay::SetFrequency(const QString &frequency, const QString &name) {
    // Ustawiamy nazwę strumienia
    if (name.isEmpty()) {
        communication_stream_->SetStreamName(QString("WAVELENGTH: %1 Hz").arg(frequency));
    } else {
        communication_stream_->SetStreamName(QString("%1 (%2 Hz)").arg(name).arg(frequency));
    }

    // Czyścimy wszystko
    Clear();
}

void WavelengthStreamDisplay::AddMessage(const QString &message, const QString &message_id,
    const StreamMessage::MessageType type) {
    // --- NOWA LOGIKA AKTUALIZACJI ---
    // Sprawdzamy, czy to wiadomość progresu (ma ID) i czy już jest wyświetlona
    if (!message_id.isEmpty() && displayed_progress_messages_.contains(message_id)) {
        if (StreamMessage* existing_message = displayed_progress_messages_.value(message_id)) {
            qDebug() << "Aktualizowanie istniejącej wiadomości progresu ID:" << message_id;
            existing_message->UpdateContent(message);
            return; // Zaktualizowano, nie dodajemy do kolejki
        }
        // Wskaźnik jest null lub nieprawidłowy, usuwamy z mapy
        qWarning() << "Usuwanie nieprawidłowego wskaźnika z mapy dla ID:" << message_id;
        displayed_progress_messages_.remove(message_id);
    }
    // --- KONIEC LOGIKI AKTUALIZACJI ---

    // Jeśli nie aktualizowaliśmy, dodajemy nową wiadomość do kolejki
    qDebug() << "Dodawanie nowej wiadomości do kolejki, ID:" << message_id;
    MessageData data;
    data.content = message; // Przekazujemy oryginalny HTML
    data.id = message_id;    // Zachowujemy ID
    data.type = type;

    // Sprawdzamy, czy wiadomość zawiera załączniki (na podstawie HTML)
    data.has_attachment = message.contains("image-placeholder") ||
                         message.contains("video-placeholder") ||
                         message.contains("audio-placeholder") ||
                         message.contains("gif-placeholder");

    // Wyodrębniamy dane nadawcy (prostsza logika dla wiadomości systemowych/progresu)
    if (type == StreamMessage::kTransmitted && message_id.isEmpty()) { // Zwykła wiadomość wysłana
        data.sender = "You";
    } else if (type == StreamMessage::kReceived) { // Zwykła wiadomość odebrana
        // Logika wyciągania nadawcy z HTML (jeśli potrzebna dla odebranych)
        const QRegularExpression regular_expression("<span[^>]*>([^<]+):</span>"); // Szukamy "Nadawca:"
        const QRegularExpressionMatch match = regular_expression.match(message);
        if (match.hasMatch()) {
            data.sender = match.captured(1);
        } else {
            data.sender = "Unknown";
        }
    } else { // Wiadomość systemowa lub progresu
        data.sender = "SYSTEM"; // Lub pusty, jeśli nie chcemy pokazywać nadawcy
    }


    // Dodajemy do kolejki
    message_queue_.enqueue(data);

    // Jeśli to pierwsza wiadomość w kolejce i timer nie działa, startujemy
    if (message_queue_.size() == 1 && !message_timer_->isActive()) {
        message_timer_->start(100); // Krótsze opóźnienie dla szybszej reakcji
    }
}

void WavelengthStreamDisplay::Clear() {
    // CommunicationStream::clearMessages zajmie się usunięciem widgetów
    communication_stream_->ClearMessages();
    message_queue_.clear();
    displayed_progress_messages_.clear(); // Czyścimy mapę wskaźników
    message_timer_->stop();
}

void WavelengthStreamDisplay::ProcessNextQueuedMessage() {
    if (message_queue_.isEmpty()) {
        return;
    }
    const auto [content, sender, id, type, has_attachment] = message_queue_.dequeue();
    StreamMessage* displayed_message = nullptr;

    // Bezpiecznik - nie powinien być potrzebny, jeśli onStreamMessageDestroyed działa
    if (!id.isEmpty() && displayed_progress_messages_.contains(id)) {
        qWarning() << "Próba dodania wiadomości z ID, które już istnieje w mapie:" << id;
        displayed_message = displayed_progress_messages_.value(id);
        if(displayed_message) {
            displayed_message->UpdateContent(content); // Aktualizuj na wszelki wypadek
        } else {
            displayed_progress_messages_.remove(id); // Usuń zły wpis
            displayed_message = nullptr; // Wymuś utworzenie poniżej
        }
    }

    // Jeśli nie aktualizowaliśmy, dodajemy nową wiadomość do strumienia wizualnego
    if (!displayed_message) {
        qDebug() << "Przetwarzanie wiadomości z kolejki: " << content.left(30)
                << "... ID=" << id << " hasAttachment=" << has_attachment;

        // Wywołaj CommunicationStream, aby dodał wiadomość wizualnie, przekazując ID
        if (has_attachment) {
            displayed_message = communication_stream_->AddMessageWithAttachment(
                content, sender, type, id); // Przekaż ID
        } else {
            displayed_message = communication_stream_->AddMessage(
                content, sender, type, id); // Przekaż ID
        }
    }

    // Jeśli wiadomość została pomyślnie utworzona/wyświetlona przez CommunicationStream
    if (displayed_message) {
        // Jeśli to wiadomość progresu (ma ID), dodaj do mapy i podłącz sygnał destroyed
        if (!id.isEmpty()) {
            // Sprawdźmy jeszcze raz, czy ID nie zostało dodane w międzyczasie
            if (!displayed_progress_messages_.contains(id)) {
                qDebug() << "Dodano wiadomość progresu do mapy, ID:" << id;
                displayed_progress_messages_.insert(id, displayed_message);
                // Podłącz sygnał zniszczenia do slotu czyszczącego mapę
                connect(displayed_message, &QObject::destroyed, this, &WavelengthStreamDisplay::OnStreamMessageDestroyed);
            } else {
                qWarning() << "Wiadomość progresu z ID" << id << "już istnieje w mapie podczas dodawania!";
            }
        }
        // Dla zwykłych wiadomości (puste ID) nic nie robimy z mapą ani sygnałem
    } else {
        qWarning() << "CommunicationStream nie utworzył/zwrócił widgetu wiadomości dla ID:" << id;
    }

    // Uruchom timer dla następnej wiadomości w kolejce
    if (!message_queue_.isEmpty()) {
        const int delay = 300 + QRandomGenerator::global()->bounded(500);
        message_timer_->start(delay);
    }
}

void WavelengthStreamDisplay::OnStreamMessageDestroyed(const QObject *object) {
    // Iterujemy po mapie, aby znaleźć wpis pasujący do zniszczonego obiektu
    // To jest bezpieczniejsze niż poleganie na ID, jeśli jakimś cudem ID nie zostało ustawione
    auto it = displayed_progress_messages_.begin();
    while (it != displayed_progress_messages_.end()) {
        if (it.value() == object) {
            qDebug() << "Usuwanie wiadomości progresu z mapy po zniszczeniu, ID:" << it.key();
            it = displayed_progress_messages_.erase(it); // Usuń wpis i przejdź do następnego
            return; // Znaleziono i usunięto, kończymy
        }
        ++it;
    }
    qWarning() << "Otrzymano sygnał destroyed dla obiektu, którego nie ma w mapie wiadomości progresu.";
}
