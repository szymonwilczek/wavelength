#include "communication_stream.h"

#include <QParallelAnimationGroup>

CommunicationStream::CommunicationStream(QWidget *parent): QOpenGLWidget(parent),
                                                           base_wave_amplitude_(0.05), // Bazowa amplituda w stanie Idle
                                                           amplitude_scale_(0.25),    // Mnożnik dla amplitudy audio (dostosuj wg potrzeb)
                                                           wave_amplitude_(base_wave_amplitude_),
                                                           target_wave_amplitude_(base_wave_amplitude_), // Docelowa amplituda
                                                           wave_frequency_(2.0), wave_speed_(1.0),
                                                           glitch_intensity_(0.0), wave_thickness_(0.008),
                                                           state_(kIdle), current_message_index_(-1),
                                                           initialized_(false), time_offset_(0.0), shader_program_(nullptr),
                                                           vertex_buffer_(QOpenGLBuffer::VertexBuffer) {
    setMinimumSize(600, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Timer animacji
    animation_timer_ = new QTimer(this);
    connect(animation_timer_, &QTimer::timeout, this, &CommunicationStream::UpdateAnimation);
    animation_timer_->setTimerType(Qt::PreciseTimer);
    animation_timer_->start(16); // ~60fps

    // Timer losowych zakłóceń w trybie Idle
    glitch_timer_ = new QTimer(this);
    connect(glitch_timer_, &QTimer::timeout, this, &CommunicationStream::TriggerRandomGlitch);
    glitch_timer_->start(3000 + QRandomGenerator::global()->bounded(2000)); // Częstsze zakłócenia

    // Etykieta nazwy strumienia
    stream_name_label_ = new QLabel("COMMUNICATION STREAM", this);
    stream_name_label_->setStyleSheet(
        "QLabel {"
        "  color: #00ccff;"
        "  background-color: rgba(0, 15, 30, 180);"
        "  border: 1px solid #00aaff;"
        "  border-radius: 0px;"
        "  padding: 3px 10px;"
        "  font-family: 'Consolas', sans-serif;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
    );
    stream_name_label_->setAlignment(Qt::AlignCenter);
    stream_name_label_->adjustSize();
    stream_name_label_->move((width() - stream_name_label_->width()) / 2, 10);

    // --- NOWY WSKAŹNIK NADAJĄCEGO ---
    transmitting_user_label_ = new UserInfoLabel(this);
    transmitting_user_label_->setStyleSheet(
        "UserInfoLabel {"
        "  color: #dddddd;"
        "  background-color: rgba(0, 0, 0, 180);"
        "  border: 1px solid #555555;"
        "  border-radius: 0px;"
        "  padding: 2px 4px;"
        "  font-family: 'Consolas', sans-serif;"
        "  font-weight: bold;"
        "  font-size: 10px;"
        "}"
    );
    transmitting_user_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    transmitting_user_label_->hide();
    // Pozycja zostanie ustawiona w resizeGL
    // --- KONIEC NOWEGO WSKAŹNIKA --

    // Format dla OpenGL
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // MSAA
    format.setSwapInterval(1); // Włącza V-Sync
    setFormat(format);

    // Dodanie opcji dla lepszego buforowania
    setAttribute(Qt::WA_OpaquePaintEvent, true);  // Optymalizacja dla nieprzezroczystych widżetów
    setAttribute(Qt::WA_NoSystemBackground, true);
}

CommunicationStream::~CommunicationStream() {
    makeCurrent();
    vao_.destroy();
    vertex_buffer_.destroy();
    delete shader_program_;
    doneCurrent();
}

StreamMessage * CommunicationStream::AddMessageWithAttachment(const QString &content, const QString &sender,
    const StreamMessage::MessageType type, const QString &message_id) {
    StartReceivingAnimation();

    // Tworzymy nową wiadomość, przekazując messageId
    const auto message = new StreamMessage(content, sender, type, message_id, this); // Przekazujemy ID!
    message->hide();

    qDebug() << "Dodawanie załącznika dla wiadomości ID:" << message_id << " Content:" << content.left(30) << "...";
    message->AddAttachment(content);

    messages_.append(message);

    // Podłączamy sygnały
    ConnectSignalsForMessage(message); // Używamy metody pomocniczej

    // Jeśli nie ma wyświetlanej wiadomości, pokaż tę po zakończeniu animacji odbioru
    if (current_message_index_ == -1) {
        QTimer::singleShot(1200, this, [this]() {
            // Sprawdź, czy lista nie jest pusta przed pokazaniem
            if (!messages_.isEmpty()) {
                ShowMessageAtIndex(messages_.size() - 1);
            }
        });
    } else {
        // Aktualizuj przyciski nawigacyjne istniejącej wiadomości
        UpdateNavigationButtonsForCurrentMessage();
    }

    return message;
}

void CommunicationStream::SetWaveAmplitude(const qreal amplitude) {
    wave_amplitude_ = amplitude;
    update();
}

void CommunicationStream::SetWaveFrequency(const qreal frequency) {
    wave_frequency_ = frequency;
    update();
}

void CommunicationStream::SetWaveSpeed(const qreal speed) {
    wave_speed_ = speed;
    update();
}

void CommunicationStream::SetGlitchIntensity(const qreal intensity) {
    glitch_intensity_ = intensity;
    update();
}

void CommunicationStream::SetWaveThickness(const qreal thickness) {
    wave_thickness_ = thickness;
    update();
}

void CommunicationStream::SetStreamName(const QString &name) const {
    stream_name_label_->setText(name);
    stream_name_label_->adjustSize();
    stream_name_label_->move((width() - stream_name_label_->width()) / 2, 10);
}

StreamMessage * CommunicationStream::AddMessage(const QString &content, const QString &sender,
    const StreamMessage::MessageType type, const QString &message_id) {
    StartReceivingAnimation();

    // Tworzymy nową wiadomość, przekazując messageId
    const auto message = new StreamMessage(content, sender, type, message_id, this); // Przekazujemy ID!
    message->hide();

    messages_.append(message);

    // Podłączamy sygnały
    ConnectSignalsForMessage(message); // Używamy metody pomocniczej

    // Jeśli nie ma wyświetlanej wiadomości, pokaż tę po zakończeniu animacji odbioru
    if (current_message_index_ == -1) {
        QTimer::singleShot(1200, this, [this]() {
            if (!messages_.isEmpty()) {
                ShowMessageAtIndex(messages_.size() - 1);
            }
        });
    } else {
        // Aktualizuj przyciski nawigacyjne istniejącej wiadomości
        UpdateNavigationButtonsForCurrentMessage();
    }
    return message;
}

void CommunicationStream::ClearMessages() {
    qDebug() << "CommunicationStream::clearMessages - Czyszczenie wszystkich wiadomości.";
    // Ukrywamy i rozłączamy sygnały dla aktualnie wyświetlanej wiadomości, jeśli istnieje
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        StreamMessage* current_message = messages_[current_message_index_];
        DisconnectSignalsForMessage(current_message); // Rozłącz sygnały przed usunięciem
        current_message->hide();
    }

    // Rozłącz sygnały i usuń wszystkie widgety wiadomości
    for (StreamMessage* message : qAsConst(messages_)) {
        DisconnectSignalsForMessage(message); // Upewnij się, że sygnały są rozłączone
        message->deleteLater(); // Bezpieczne usunięcie widgetu
    }

    messages_.clear(); // Wyczyść listę wskaźników
    current_message_index_ = -1; // Zresetuj indeks
    ReturnToIdleAnimation(); // Przywróć animację tła do stanu bezczynności
    update(); // Odśwież widok
}

void CommunicationStream::SetTransmittingUser(const QString &user_id) const {
    QString display_text = user_id;
    // Skracanie ID (bez zmian)
    if (user_id.length() > 15 && user_id.startsWith("client_")) {
        display_text = "CLIENT " + user_id.split('_').last();
    } else if (user_id.length() > 15 && user_id.startsWith("ws_")) {
        display_text = "HOST " + user_id.split('_').last();
    } else if (user_id == "You") {
        display_text = "YOU";
    }

    // --- GENERUJ KOLOR I USTAW LABEL ---
    const auto [color] = GenerateUserVisuals(user_id);
    // m_transmittingUserLabel->setShape(visuals.shape); // USUNIĘTO
    transmitting_user_label_->SetShapeColor(color); // Ustaw tylko kolor
    transmitting_user_label_->setText(display_text);
    // --- KONIEC USTAWIANIA ---

    transmitting_user_label_->adjustSize();
    transmitting_user_label_->move(width() - transmitting_user_label_->width() - 10, 10);
    transmitting_user_label_->show();
}

void CommunicationStream::ClearTransmittingUser() const {
    transmitting_user_label_->hide();
    transmitting_user_label_->setText(""); // Wyczyść tekst na wszelki wypadek
}

void CommunicationStream::SetAudioAmplitude(const qreal amplitude) {
    target_wave_amplitude_ = base_wave_amplitude_ + qBound(0.0, amplitude, 1.0) * amplitude_scale_;
}

void CommunicationStream::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Inicjalizujemy shader program
    shader_program_ = new QOpenGLShaderProgram();

    // Vertex shader
    const auto vertex_shader_source = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;

            uniform vec2 resolution;

            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

    // Fragment shader - poprawiony dla lepszego wyglądu gridu i grubszej fali
    // W metodzie initializeGL() zmodyfikuj fragment shader:

    // Fragment shader - naprawiony, aby zachować podstawową linię
    const auto fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec2 resolution;
    uniform float time;
    uniform float amplitude;
    uniform float frequency;
    uniform float speed;
    uniform float glitchIntensity;
    uniform float waveThickness;

    // Funkcja pseudolosowa
    float random(vec2 st) {
        return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
    }

    void main() {
        // Znormalizowane koordynaty ekranu
        vec2 uv = gl_FragCoord.xy / resolution.xy;
        uv = uv * 2.0 - 1.0;  // Przeskalowanie do -1..1

        // Parametry linii
        float lineThickness = waveThickness;
        float glow = 0.03;
        vec3 lineColor = vec3(0.0, 0.7, 1.0);  // Neonowy niebieski
        vec3 glowColor = vec3(0.0, 0.4, 0.8);  // Ciemniejszy niebieski dla poświaty

        // Podstawowa fala
        float xFreq = frequency * 3.14159;
        float tOffset = time * speed;
        float wave = sin(uv.x * xFreq + tOffset) * amplitude;

        // Przesuwające się zakłócenie
        if (glitchIntensity > 0.01) {
            // Kierunek (zmienia się co kilka sekund)
            float direction = sin(floor(time * 0.2) * 0.5) > 0.0 ? 1.0 : -1.0;

            // Pozycja zakłócenia (-1..1)
            float glitchPos = fract(time * 0.5) * 2.0 - 1.0;
            glitchPos = direction > 0.0 ? glitchPos : -glitchPos;

            // Szerokość i siła zakłócenia
            float glitchWidth = 0.1 + glitchIntensity * 0.2;
            float distFromGlitch = abs(uv.x - glitchPos);
            float glitchFactor = smoothstep(glitchWidth, 0.0, distFromGlitch);

            // Dodajemy zakłócenie do fali
            wave += glitchFactor * glitchIntensity * 0.3 * sin(uv.x * 30.0 + time * 10.0);
        }

        // Odległość od punktu do linii fali
        float dist = abs(uv.y - wave);

        // Rysujemy linię z poświatą
        float line = smoothstep(lineThickness, 0.0, dist);
        float glowFactor = smoothstep(glow, lineThickness, dist);

        // Końcowy kolor
        vec3 color = lineColor * line + glowColor * glowFactor * (0.3 + glitchIntensity);

        // Siatka w tle - jaśniejsza
        float gridIntensity = 0.1;
        vec2 gridCoord = uv * 20.0;
        if (mod(gridCoord.x, 1.0) < 0.05 || mod(gridCoord.y, 1.0) < 0.05) {
            color += vec3(0.0, 0.3, 0.4) * gridIntensity;
        }

        // Subtelne linie drugorzędne
        gridCoord = uv * 40.0;
        if (mod(gridCoord.x, 1.0) < 0.02 || mod(gridCoord.y, 1.0) < 0.02) {
            color += vec3(0.0, 0.1, 0.2) * gridIntensity * 0.5;
        }

        // Końcowa przezroczystość
        float alpha = line + glowFactor * 0.6 + 0.1;

        FragColor = vec4(color, alpha);
    }
)";

    shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source);
    shader_program_->link();

    vao_.create();
    vao_.bind();

    // Wierzchołki prostokąta dla fullscreen quad
    static constexpr GLfloat vertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f
    };

    vertex_buffer_.create();
    vertex_buffer_.bind();
    vertex_buffer_.allocate(vertices, sizeof(vertices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    vertex_buffer_.release();
    vao_.release();

    initialized_ = true;
}

void CommunicationStream::resizeGL(const int w, const int h) {
    glViewport(0, 0, w, h);
    stream_name_label_->move((width() - stream_name_label_->width()) / 2, 10);

    // --- AKTUALIZACJA POZYCJI UserInfoLabel ---
    // adjustSize() powinien być wywołany w setTransmittingUser
    transmitting_user_label_->move(width() - transmitting_user_label_->width() - 10, 10);
    // --- KONIEC AKTUALIZACJI ---

    UpdateMessagePosition();
}

void CommunicationStream::paintGL() {
    if (!initialized_) return;

    glClear(GL_COLOR_BUFFER_BIT);

    // Używamy shadera
    shader_program_->bind();

    // Ustawiamy uniformy
    shader_program_->setUniformValue("resolution", QVector2D(width(), height()));
    shader_program_->setUniformValue("time", static_cast<float>(time_offset_));
    shader_program_->setUniformValue("amplitude", static_cast<float>(wave_amplitude_));
    shader_program_->setUniformValue("frequency", static_cast<float>(wave_frequency_));
    shader_program_->setUniformValue("speed", static_cast<float>(wave_speed_));
    shader_program_->setUniformValue("glitchIntensity", static_cast<float>(glitch_intensity_));
    shader_program_->setUniformValue("waveThickness", static_cast<float>(wave_thickness_));

    // Rysujemy quad
    vao_.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    vao_.release();

    shader_program_->release();
}

void CommunicationStream::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Tab) {
        event->accept(); // Zachowaj obsługę Tab
        return; // Zakończ przetwarzanie, aby uniknąć dalszych działań
    }

    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        const StreamMessage* current_message = messages_[current_message_index_];

        switch (event->key()) {
            case Qt::Key_Right:
                if (current_message->GetNextButton()->isVisible()) {
                    ShowNextMessage();
                    event->accept(); // Akceptuj zdarzenie tylko jeśli akcja została wykonana
                }
                return; // Zakończ przetwarzanie dla tego klawisza

            case Qt::Key_Left:
                if (current_message->GetPrevButton()->isVisible()) {
                    ShowPreviousMessage();
                    event->accept(); // Akceptuj zdarzenie tylko jeśli akcja została wykonana
                }
                return; // Zakończ przetwarzanie dla tego klawisza

            case Qt::Key_Return: // Używaj tylko Enter (Return)
                // case Qt::Key_Enter: // Qt::Key_Return zazwyczaj obejmuje Enter na numpadzie
                OnMessageRead(); // Wywołaj zamknięcie wiadomości
                event->accept(); // Akceptuj zdarzenie
                return; // Zakończ przetwarzanie dla tego klawisza

            // Usuwamy obsługę Spacji do zamykania wiadomości
            // case Qt::Key_Space:
            //     onMessageRead();
            //     event->accept();
            //     return;

            default:
                // Jeśli żaden z naszych klawiszy nie został naciśnięty,
                // przekaż zdarzenie do bazowej implementacji lub do aktywnej wiadomości
                // (jeśli wiadomość ma własną obsługę klawiszy, np. do przewijania)
                if (current_message->hasFocus()) {
                    // Pozwól wiadomości obsłużyć inne klawisze, jeśli ma fokus
                    // currentMsg->keyPressEvent(event); // Można odkomentować, jeśli wiadomość ma obsługiwać np. PageUp/Down
                    // Jeśli nie chcemy, aby wiadomość przechwytywała inne klawisze, zostawiamy przekazanie poniżej
                }
                break; // Przejdź do domyślnej obsługi poniżej
        }
    }
    // Jeśli żadna z powyższych obsług nie przechwyciła zdarzenia,
    // przekaż je do domyślnej implementacji QOpenGLWidget
    QOpenGLWidget::keyPressEvent(event);
}

void CommunicationStream::UpdateAnimation() {
    // Aktualizujemy czas animacji
    time_offset_ += 0.02 * wave_speed_;

    // --- PŁYNNA ZMIANA AMPLITUDY ---
    // Interpolujemy aktualną amplitudę w kierunku docelowej
    // Współczynnik 0.1 oznacza, że w każdej klatce zbliżamy się o 10% do celu
    // Można dostosować (np. 0.2 dla szybszej reakcji, 0.05 dla wolniejszej)
    wave_amplitude_ = wave_amplitude_ * 0.9 + target_wave_amplitude_ * 0.1;
    // --- KONIEC PŁYNNEJ ZMIANY ---

    // Stopniowo zmniejszamy intensywność zakłóceń (jeśli są używane niezależnie)
    if (glitch_intensity_ > 0.0) {
        glitch_intensity_ = qMax(0.0, glitch_intensity_ - 0.005);
    }

    // Wymuszamy aktualizację warstwy OpenGL
    const QRegion update_region(0, 0, width(), height());
    update(update_region);
}

void CommunicationStream::TriggerRandomGlitch() {
    // Wywołujemy losowe zakłócenia tylko w trybie bezczynności
    if (state_ == kIdle) {
        // Silniejsza intensywność dla lepszej widoczności
        const float intensity = 0.3 + QRandomGenerator::global()->bounded(40) / 100.0;

        // Wywołujemy animację zakłócenia
        StartGlitchAnimation(intensity);
    }

    // Ustawiamy kolejny losowy interwał - częstsze zakłócenia
    glitch_timer_->setInterval(1000 + QRandomGenerator::global()->bounded(2000));
}

void CommunicationStream::StartReceivingAnimation() {
    // Zmieniamy stan
    state_ = kReceiving;

    // Animujemy parametry fali
    const auto amplitude_animation = new QPropertyAnimation(this, "waveAmplitude");
    amplitude_animation->setDuration(1000);
    amplitude_animation->setStartValue(wave_amplitude_);
    // Zamiast dużej amplitudy, ustawiamy tylko nieznacznie podniesioną bazę
    // Główna animacja amplitudy będzie pochodzić z setAudioAmplitude
    amplitude_animation->setEndValue(0.15); // Np. podwójna bazowa
    amplitude_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto frequency_animation = new QPropertyAnimation(this, "waveFrequency");
    frequency_animation->setDuration(1000);
    frequency_animation->setStartValue(wave_frequency_);
    frequency_animation->setEndValue(6.0);  // Wyższa częstotliwość
    frequency_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto speed_animation = new QPropertyAnimation(this, "waveSpeed");
    speed_animation->setDuration(1000);
    speed_animation->setStartValue(wave_speed_);
    speed_animation->setEndValue(2.5);  // Szybsza prędkość
    speed_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto thickness_animation = new QPropertyAnimation(this, "waveThickness");
    thickness_animation->setDuration(1000);
    thickness_animation->setStartValue(wave_thickness_);
    thickness_animation->setEndValue(0.012);  // Grubsza linia podczas aktywności
    thickness_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    glitch_animation->setDuration(1000);
    glitch_animation->setStartValue(0.0);
    glitch_animation->setKeyValueAt(0.3, 0.6);  // Większe zakłócenia
    glitch_animation->setKeyValueAt(0.6, 0.3);
    glitch_animation->setEndValue(0.2);
    glitch_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(amplitude_animation);
    group->addAnimation(frequency_animation);
    group->addAnimation(speed_animation);
    group->addAnimation(thickness_animation);
    group->addAnimation(glitch_animation);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    // Zmieniamy stan na wyświetlanie po zakończeniu animacji
    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        target_wave_amplitude_ = 0.15;
        state_ = kDisplaying;
    });
}

void CommunicationStream::ReturnToIdleAnimation() {
    // Zmieniamy stan
    state_ = kIdle;

    target_wave_amplitude_ = base_wave_amplitude_;
    // --- KONIEC RESETU ---

    // Animujemy powrót do stanu bezczynności
    const auto amplitude_animation = new QPropertyAnimation(this, "waveAmplitude");
    amplitude_animation->setDuration(1500);
    amplitude_animation->setStartValue(wave_amplitude_);
    amplitude_animation->setEndValue(0.01); // Powrót do bazowej
    amplitude_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto frequency_animation = new QPropertyAnimation(this, "waveFrequency");
    frequency_animation->setDuration(1500);
    frequency_animation->setStartValue(wave_frequency_);
    frequency_animation->setEndValue(2.0);
    frequency_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto speed_animation = new QPropertyAnimation(this, "waveSpeed");
    speed_animation->setDuration(1500);
    speed_animation->setStartValue(wave_speed_);
    speed_animation->setEndValue(1.0);
    speed_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto thickness_animation = new QPropertyAnimation(this, "waveThickness");
    thickness_animation->setDuration(1500);
    thickness_animation->setStartValue(wave_thickness_);
    thickness_animation->setEndValue(0.008);  // Powrót do normalnej grubości
    thickness_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(amplitude_animation);
    group->addAnimation(frequency_animation);
    group->addAnimation(speed_animation);
    group->addAnimation(thickness_animation);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void CommunicationStream::StartGlitchAnimation(const qreal intensity) {
    // Krótszy czas dla szybszego efektu
    const int duration = 600 + QRandomGenerator::global()->bounded(400);

    const auto glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    glitch_animation->setDuration(duration);
    glitch_animation->setStartValue(glitch_intensity_);
    glitch_animation->setKeyValueAt(0.1, intensity); // Szybki wzrost
    glitch_animation->setKeyValueAt(0.5, intensity * 0.7);
    glitch_animation->setEndValue(0.0);
    glitch_animation->setEasingCurve(QEasingCurve::OutQuad);
    glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void CommunicationStream::ShowMessageAtIndex(int index) {
    OptimizeForMessageTransition(); // Optymalizacja (jeśli potrzebna)

    // Sprawdzenie poprawności indeksu i obsługa pustej listy
    if (index < 0 || index >= messages_.size()) {
        qWarning() << "CommunicationStream::showMessageAtIndex - Nieprawidłowy indeks:" << index << ", rozmiar listy:" << messages_.size();
        if (!messages_.isEmpty()) {
            index = 0; // Pokaż pierwszą jako fallback, jeśli lista nie jest pusta
            qWarning() << "CommunicationStream::showMessageAtIndex - Ustawiono indeks na 0.";
        } else {
            current_message_index_ = -1;
            ReturnToIdleAnimation();
            return; // Zakończ, jeśli lista jest pusta
        }
    }

    // Sprawdź, czy indeks się faktycznie zmienia
    if (index == current_message_index_ && messages_[index]->isVisible()) {
        qDebug() << "CommunicationStream::showMessageAtIndex - Indeks (" << index << ") jest taki sam jak bieżący i wiadomość jest widoczna. Nic nie robię.";
        // Upewnij się, że ma fokus
        messages_[index]->setFocus();
        return;
    }


    // Ukrywamy poprzednią wiadomość (jeśli była inna niż nowa)
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        if (current_message_index_ != index) {
            StreamMessage* previous_message_ptr = nullptr;
            previous_message_ptr = messages_[current_message_index_];
            qDebug() << "CommunicationStream::showMessageAtIndex - Ukrywanie poprzedniej wiadomości (indeks:" << current_message_index_ << ")";
            DisconnectSignalsForMessage(previous_message_ptr);
            previous_message_ptr->hide();
        }
    }

    // Ustaw nowy bieżący indeks
    current_message_index_ = index;
    StreamMessage* message = messages_[current_message_index_];
    qDebug() << "CommunicationStream::showMessageAtIndex - Pokazywanie wiadomości (indeks:" << current_message_index_ << ")";

    // Ponownie podłączamy sygnały dla nowej, bieżącej wiadomości
    ConnectSignalsForMessage(message);

    // Ustawiamy stan
    state_ = kDisplaying;
    // m_targetWaveAmplitude = m_baseWaveAmplitude * 6;

    // --- ZMIANA KOLEJNOŚCI I METOD ---
    // 1. Upewnij się, że widget ma szansę obliczyć swój rozmiar
    message->adjustSize(); // Spróbuj wymusić obliczenie rozmiaru na podstawie zawartości
    // 2. Ustaw pozycję i rozmiar za pomocą setGeometry
    UpdateMessagePosition(); // Ta funkcja teraz użyje setGeometry
    // 3. Podnieś na wierzch
    message->raise();
    // 4. Pokaż widget
    message->show();
    // 5. Uruchom animację pojawiania się
    message->FadeIn();
    // 6. Zaktualizuj przyciski nawigacyjne
    UpdateNavigationButtonsForCurrentMessage();
    // 7. Ustaw fokus
    message->setFocus();
    // 8. Zaktualizuj tło OpenGL
    update();
    // --- KONIEC ZMIANY ---
}

void CommunicationStream::ShowNextMessage() {
    if (current_message_index_ < messages_.size() - 1) {
        OptimizeForMessageTransition();
        ShowMessageAtIndex(current_message_index_ + 1);
    }
}

void CommunicationStream::ShowPreviousMessage() {
    if (current_message_index_ > 0) {
        OptimizeForMessageTransition();
        ShowMessageAtIndex(current_message_index_ - 1);
    }
}

void CommunicationStream::OnMessageRead() {
    if (current_message_index_ < 0 || current_message_index_ >= messages_.size()) {
        qWarning() << "CommunicationStream::onMessageRead - Brak bieżącej wiadomości do rozpoczęcia zamykania.";
        return;
    }

    // --- NOWA LOGIKA DLA ENTER ---
    // Ustaw flagę, że chcemy wyczyścić WSZYSTKIE wiadomości.
    // Proces rozpocznie się od zamknięcia bieżącej wiadomości.
    is_clearing_all_messages_ = true;
    qDebug() << "CommunicationStream::onMessageRead - Inicjowanie czyszczenia wszystkich wiadomości. Zamykanie bieżącej (indeks:" << current_message_index_ << ")";
    // --- KONIEC NOWEJ LOGIKI ---

    StreamMessage* message = messages_[current_message_index_];
    message->MarkAsRead(); // Rozpocznij animację zamykania bieżącej wiadomości
    // Dalsza logika czyszczenia zostanie obsłużona w handleMessageHidden po zakończeniu animacji bieżącej wiadomości.
}

void CommunicationStream::HandleMessageHidden() {
    const auto hidden_message = qobject_cast<StreamMessage*>(sender());
    if (!hidden_message) {
        qWarning() << "CommunicationStream::handleMessageHidden - Otrzymano sygnał hidden() od nieznanego obiektu.";
        return;
    }

    qDebug() << "CommunicationStream::handleMessageHidden - Wiadomość została ukryta. ID:" << hidden_message->GetMessageId() << "Typ:" << hidden_message->GetType() << "m_isClearingAllMessages:" << is_clearing_all_messages_;

    // Znajdź indeks ukrytej wiadomości PRZED potencjalnym usunięciem
    const int hidden_message_index = messages_.indexOf(hidden_message);

    // Zawsze rozłączamy sygnały od ukrytej wiadomości
    DisconnectSignalsForMessage(hidden_message);

    // --- NOWA LOGIKA DLA CZYSZCZENIA WSZYSTKICH WIADOMOŚCI ---
    if (is_clearing_all_messages_) {
        qDebug() << "CommunicationStream::handleMessageHidden - Tryb czyszczenia wszystkich wiadomości aktywny.";

        // Usuń ukrytą wiadomość z listy (jeśli tam jest) i zaplanuj jej usunięcie
        if (hidden_message_index != -1) {
            messages_.removeAt(hidden_message_index);
        } else {
            qWarning() << "CommunicationStream::handleMessageHidden [ClearAll] - Nie znaleziono ukrytej wiadomości w liście!";
        }
        hidden_message->deleteLater();

        // Ukryj i usuń wszystkie POZOSTAŁE wiadomości
        // Tworzymy kopię listy wskaźników, aby bezpiecznie iterować i modyfikować oryginał
        QList<StreamMessage*> messages_to_remove = messages_;
        messages_.clear(); // Czyścimy oryginalną listę od razu

        qDebug() << "CommunicationStream::handleMessageHidden [ClearAll] - Usuwanie pozostałych" << messages_to_remove.size() << "wiadomości.";
        for (StreamMessage* message : messages_to_remove) {
            if (message) {
                DisconnectSignalsForMessage(message); // Rozłącz sygnały na wszelki wypadek
                message->hide(); // Ukryj natychmiast
                message->deleteLater(); // Zaplanuj usunięcie
            }
        }

        // Zresetuj stan strumienia
        current_message_index_ = -1;
        is_clearing_all_messages_ = false; // Zresetuj flagę
        ReturnToIdleAnimation();
        update();
        qDebug() << "CommunicationStream::handleMessageHidden [ClearAll] - Zakończono czyszczenie.";
        return; // Zakończ obsługę w tym trybie
    }
    // --- KONIEC NOWEJ LOGIKI ---


    // Poniżej znajduje się istniejąca logika dla normalnego ukrywania (progres, zamknięcie pojedynczej, nawigacja)

    // Sprawdzamy, czy to wiadomość progresu (ma niepuste ID)
    if (!hidden_message->GetMessageId().isEmpty()) {
        // ... (istniejąca logika usuwania wiadomości progresu - bez zmian) ...
        qDebug() << "CommunicationStream::handleMessageHidden - Usuwanie wiadomości progresu ID:" << hidden_message->GetMessageId();
        if (hidden_message_index != -1) {
            messages_.removeAt(hidden_message_index);
            // Dostosuj indeks bieżącej wiadomości, jeśli usunięta była przed nią
            if (hidden_message_index < current_message_index_) {
                current_message_index_--;
            } else if (hidden_message_index == current_message_index_) {
                // Jeśli wiadomość progresu była jakimś cudem aktualna, zresetuj indeks
                current_message_index_ = -1;
            }
        }
        hidden_message->deleteLater(); // Bezpieczne usunięcie widgetu

        // Sprawdź, czy po usunięciu są jeszcze jakieś wiadomości
        if (messages_.isEmpty()) {
            current_message_index_ = -1;
            ReturnToIdleAnimation();
        } else if (current_message_index_ == -1) {
            ReturnToIdleAnimation();
        }
    } else {
        // To jest zwykła wiadomość, która została ukryta (i NIE jesteśmy w trybie czyszczenia).
        qDebug() << "CommunicationStream::handleMessageHidden - Zwykła wiadomość ukryta (indeks: " << hidden_message_index << ", aktualny: " << current_message_index_ << ")";

        if (hidden_message_index == current_message_index_) {
            // Wiadomość była aktualnie wyświetlana i została zamknięta przez użytkownika (nie przez Enter w trybie ClearAll).
            qDebug() << "CommunicationStream::handleMessageHidden - Ukryta wiadomość była aktualnie wyświetlana (zamknięcie pojedyncze). Usuwanie i pokazywanie następnej.";

            // Usuwamy wiadomość z listy
            if (hidden_message_index != -1) {
                messages_.removeAt(hidden_message_index);
            } else {
                qWarning() << "CommunicationStream::handleMessageHidden [SingleClose] - Nie znaleziono zamkniętej wiadomości w liście!";
            }
            hidden_message->deleteLater(); // Usuń widget

            // Pokaż następną lub wróć do Idle
            if (messages_.isEmpty()) {
                qDebug() << "CommunicationStream::handleMessageHidden [SingleClose] - Brak więcej wiadomości, powrót do Idle.";
                current_message_index_ = -1;
                ReturnToIdleAnimation();
            } else {
                int next_index_to_show = qMin(hidden_message_index, messages_.size() - 1);
                qDebug() << "CommunicationStream::handleMessageHidden [SingleClose] - Są inne wiadomości. Próba pokazania indeksu:" << next_index_to_show;
                QTimer::singleShot(0, this, [this, next_index_to_show]() {
                    if (next_index_to_show >= 0 && next_index_to_show < messages_.size()) {
                        ShowMessageAtIndex(next_index_to_show);
                    } else if (!messages_.isEmpty()){
                        ShowMessageAtIndex(0);
                    } else {
                        current_message_index_ = -1;
                        ReturnToIdleAnimation();
                    }
                });
                current_message_index_ = -1; // Tymczasowo resetuj indeks
            }
        } else {
            // Wiadomość ukryta podczas nawigacji. Nic nie robimy.
            qDebug() << "CommunicationStream::handleMessageHidden - Ukryta wiadomość nie była aktualnie wyświetlana (nawigacja). Nic nie robię.";
        }
    }
    update(); // Ogólna aktualizacja na koniec
}

UserVisuals CommunicationStream::GenerateUserVisuals(const QString &user_id) {
    UserVisuals visuals;
    const quint32 hash = qHash(user_id);

    // Wygeneruj kolor na podstawie hasha (w przestrzeni HSL dla lepszych kolorów)
    const int hue = hash % 360; // Odcień 0-359
    // Utrzymuj wysokie nasycenie i jasność dla neonowego efektu
    const int saturation = 230 + ((hash >> 8) % 26); // Nasycenie 230-255 (bardzo nasycone)
    const int lightness = 140 + ((hash >> 16) % 31); // Jasność 140-170 (jasne, ale nie białe)

    visuals.color = QColor::fromHsl(hue, saturation, lightness);
    return visuals;
}

void CommunicationStream::ConnectSignalsForMessage(StreamMessage *message) {
    if (!message) return;
    // Używamy UniqueConnection, aby uniknąć duplikatów połączeń
    connect(message->GetNextButton(), &QPushButton::clicked, this, &CommunicationStream::ShowNextMessage, Qt::UniqueConnection);
    connect(message->GetPrevButton(), &QPushButton::clicked, this, &CommunicationStream::ShowPreviousMessage, Qt::UniqueConnection);
    connect(message->mark_read_button, &QPushButton::clicked, this, &CommunicationStream::OnMessageRead, Qt::UniqueConnection);
    // Podłączamy sygnał ukrycia wiadomości
    connect(message, &StreamMessage::hidden, this, &CommunicationStream::HandleMessageHidden, Qt::UniqueConnection);
    qDebug() << "CommunicationStream::connectSignalsForMessage - Podłączono sygnały dla wiadomości (indeks: " << messages_.indexOf(message) << ")";
}

void CommunicationStream::DisconnectSignalsForMessage(StreamMessage *message) const {
    if (!message) return;
    // Rozłącz wszystkie sygnały od tej wiadomości do tego obiektu (CommunicationStream)
    // To powinno obejmować sygnał hidden() podłączony w connectSignalsForMessage
    disconnect(message, nullptr, this, nullptr);
    qDebug() << "CommunicationStream::disconnectSignalsForMessage - Rozłączono sygnały dla wiadomości (indeks: " << messages_.indexOf(message) << ")";
}

void CommunicationStream::UpdateNavigationButtonsForCurrentMessage() {
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        const StreamMessage* current_message = messages_[current_message_index_];
        const bool has_previous = current_message_index_ > 0;
        const bool has_next = current_message_index_ < messages_.size() - 1;
        current_message->ShowNavigationButtons(has_previous, has_next);
    }
}

void CommunicationStream::OptimizeForMessageTransition() const {
    // Czasowo zmniejszamy priorytet odświeżania animacji tła podczas przechodzenia między wiadomościami
    static QElapsedTimer transition_timer;
    static bool in_transition = false;

    if (!in_transition) {
        in_transition = true;
        transition_timer.start();

        // Podczas przejścia zmniejszamy częstotliwość odświeżania tła
        if (animation_timer_->interval() == 16) {
            animation_timer_->setInterval(33); // Tymczasowo 30fps zamiast 60fps
        }

        // Po 500ms wracamy do normalnego odświeżania
        QTimer::singleShot(500, this, [this]() {
            animation_timer_->setInterval(16); // Powrót do 60fps
            in_transition = false;
        });
    }
}

void CommunicationStream::UpdateMessagePosition() {
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        StreamMessage* message = messages_[current_message_index_];
        // Centrujemy wiadomość na fali (w środku ekranu)
        message->move((width() - message->width()) / 2, (height() - message->height()) / 2);
    }
}
