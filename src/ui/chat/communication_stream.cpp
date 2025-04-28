#include "communication_stream.h"

#include <QParallelAnimationGroup>

CommunicationStream::CommunicationStream(QWidget *parent): QOpenGLWidget(parent),
                                                           m_baseWaveAmplitude(0.05), // Bazowa amplituda w stanie Idle
                                                           m_amplitudeScale(0.25),    // Mnożnik dla amplitudy audio (dostosuj wg potrzeb)
                                                           m_waveAmplitude(m_baseWaveAmplitude),
                                                           m_targetWaveAmplitude(m_baseWaveAmplitude), // Docelowa amplituda
                                                           m_waveFrequency(2.0), m_waveSpeed(1.0),
                                                           m_glitchIntensity(0.0), m_waveThickness(0.008),
                                                           m_state(Idle), m_currentMessageIndex(-1),
                                                           m_initialized(false), m_timeOffset(0.0), m_shaderProgram(nullptr),
                                                           m_vertexBuffer(QOpenGLBuffer::VertexBuffer) {
    setMinimumSize(600, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Timer animacji
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &CommunicationStream::updateAnimation);
    m_animTimer->setTimerType(Qt::PreciseTimer);
    m_animTimer->start(16); // ~60fps

    // Timer losowych zakłóceń w trybie Idle
    m_glitchTimer = new QTimer(this);
    connect(m_glitchTimer, &QTimer::timeout, this, &CommunicationStream::triggerRandomGlitch);
    m_glitchTimer->start(3000 + QRandomGenerator::global()->bounded(2000)); // Częstsze zakłócenia

    // Etykieta nazwy strumienia
    m_streamNameLabel = new QLabel("COMMUNICATION STREAM", this);
    m_streamNameLabel->setStyleSheet(
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
    m_streamNameLabel->setAlignment(Qt::AlignCenter);
    m_streamNameLabel->adjustSize();
    m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);

    // --- NOWY WSKAŹNIK NADAJĄCEGO ---
    m_transmittingUserLabel = new UserInfoLabel(this);
    m_transmittingUserLabel->setStyleSheet(
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
    m_transmittingUserLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_transmittingUserLabel->hide();
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
    m_vao.destroy();
    m_vertexBuffer.destroy();
    delete m_shaderProgram;
    doneCurrent();
}

StreamMessage * CommunicationStream::addMessageWithAttachment(const QString &content, const QString &sender,
    const StreamMessage::MessageType type, const QString &messageId) {
    startReceivingAnimation();

    // Tworzymy nową wiadomość, przekazując messageId
    const auto message = new StreamMessage(content, sender, type, messageId, this); // Przekazujemy ID!
    message->hide();

    qDebug() << "Dodawanie załącznika dla wiadomości ID:" << messageId << " Content:" << content.left(30) << "...";
    message->addAttachment(content);

    m_messages.append(message);

    // Podłączamy sygnały
    connectSignalsForMessage(message); // Używamy metody pomocniczej

    // Jeśli nie ma wyświetlanej wiadomości, pokaż tę po zakończeniu animacji odbioru
    if (m_currentMessageIndex == -1) {
        QTimer::singleShot(1200, this, [this]() {
            // Sprawdź, czy lista nie jest pusta przed pokazaniem
            if (!m_messages.isEmpty()) {
                showMessageAtIndex(m_messages.size() - 1);
            }
        });
    } else {
        // Aktualizuj przyciski nawigacyjne istniejącej wiadomości
        updateNavigationButtonsForCurrentMessage();
    }

    return message;
}

void CommunicationStream::setWaveAmplitude(const qreal amplitude) {
    m_waveAmplitude = amplitude;
    update();
}

void CommunicationStream::setWaveFrequency(const qreal frequency) {
    m_waveFrequency = frequency;
    update();
}

void CommunicationStream::setWaveSpeed(const qreal speed) {
    m_waveSpeed = speed;
    update();
}

void CommunicationStream::setGlitchIntensity(const qreal intensity) {
    m_glitchIntensity = intensity;
    update();
}

void CommunicationStream::setWaveThickness(const qreal thickness) {
    m_waveThickness = thickness;
    update();
}

void CommunicationStream::setStreamName(const QString &name) const {
    m_streamNameLabel->setText(name);
    m_streamNameLabel->adjustSize();
    m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);
}

StreamMessage * CommunicationStream::addMessage(const QString &content, const QString &sender,
    const StreamMessage::MessageType type, const QString &messageId) {
    startReceivingAnimation();

    // Tworzymy nową wiadomość, przekazując messageId
    const auto message = new StreamMessage(content, sender, type, messageId, this); // Przekazujemy ID!
    message->hide();

    m_messages.append(message);

    // Podłączamy sygnały
    connectSignalsForMessage(message); // Używamy metody pomocniczej

    // Jeśli nie ma wyświetlanej wiadomości, pokaż tę po zakończeniu animacji odbioru
    if (m_currentMessageIndex == -1) {
        QTimer::singleShot(1200, this, [this]() {
            if (!m_messages.isEmpty()) {
                showMessageAtIndex(m_messages.size() - 1);
            }
        });
    } else {
        // Aktualizuj przyciski nawigacyjne istniejącej wiadomości
        updateNavigationButtonsForCurrentMessage();
    }
    return message;
}

void CommunicationStream::clearMessages() {
    qDebug() << "CommunicationStream::clearMessages - Czyszczenie wszystkich wiadomości.";
    // Ukrywamy i rozłączamy sygnały dla aktualnie wyświetlanej wiadomości, jeśli istnieje
    if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
        StreamMessage* currentMsg = m_messages[m_currentMessageIndex];
        disconnectSignalsForMessage(currentMsg); // Rozłącz sygnały przed usunięciem
        currentMsg->hide();
    }

    // Rozłącz sygnały i usuń wszystkie widgety wiadomości
    for (StreamMessage* msg : qAsConst(m_messages)) {
        disconnectSignalsForMessage(msg); // Upewnij się, że sygnały są rozłączone
        msg->deleteLater(); // Bezpieczne usunięcie widgetu
    }

    m_messages.clear(); // Wyczyść listę wskaźników
    m_currentMessageIndex = -1; // Zresetuj indeks
    returnToIdleAnimation(); // Przywróć animację tła do stanu bezczynności
    update(); // Odśwież widok
}

void CommunicationStream::setTransmittingUser(const QString &userId) const {
    QString displayText = userId;
    // Skracanie ID (bez zmian)
    if (userId.length() > 15 && userId.startsWith("client_")) {
        displayText = "CLIENT " + userId.split('_').last();
    } else if (userId.length() > 15 && userId.startsWith("ws_")) {
        displayText = "HOST " + userId.split('_').last();
    } else if (userId == "You") {
        displayText = "YOU";
    }

    // --- GENERUJ KOLOR I USTAW LABEL ---
    const UserVisuals visuals = generateUserVisuals(userId);
    // m_transmittingUserLabel->setShape(visuals.shape); // USUNIĘTO
    m_transmittingUserLabel->setShapeColor(visuals.color); // Ustaw tylko kolor
    m_transmittingUserLabel->setText(displayText);
    // --- KONIEC USTAWIANIA ---

    m_transmittingUserLabel->adjustSize();
    m_transmittingUserLabel->move(width() - m_transmittingUserLabel->width() - 10, 10);
    m_transmittingUserLabel->show();
}

void CommunicationStream::clearTransmittingUser() const {
    m_transmittingUserLabel->hide();
    m_transmittingUserLabel->setText(""); // Wyczyść tekst na wszelki wypadek
}

void CommunicationStream::setAudioAmplitude(const qreal amplitude) {
    m_targetWaveAmplitude = m_baseWaveAmplitude + qBound(0.0, amplitude, 1.0) * m_amplitudeScale;
}

void CommunicationStream::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Inicjalizujemy shader program
    m_shaderProgram = new QOpenGLShaderProgram();

    // Vertex shader
    const auto vertexShaderSource = R"(
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
    const auto fragmentShaderSource = R"(
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

    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_shaderProgram->link();

    m_vao.create();
    m_vao.bind();

    // Wierzchołki prostokąta dla fullscreen quad
    static constexpr GLfloat vertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f
    };

    m_vertexBuffer.create();
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate(vertices, sizeof(vertices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    m_vertexBuffer.release();
    m_vao.release();

    m_initialized = true;
}

void CommunicationStream::resizeGL(const int w, const int h) {
    glViewport(0, 0, w, h);
    m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);

    // --- AKTUALIZACJA POZYCJI UserInfoLabel ---
    // adjustSize() powinien być wywołany w setTransmittingUser
    m_transmittingUserLabel->move(width() - m_transmittingUserLabel->width() - 10, 10);
    // --- KONIEC AKTUALIZACJI ---

    updateMessagePosition();
}

void CommunicationStream::paintGL() {
    if (!m_initialized) return;

    glClear(GL_COLOR_BUFFER_BIT);

    // Używamy shadera
    m_shaderProgram->bind();

    // Ustawiamy uniformy
    m_shaderProgram->setUniformValue("resolution", QVector2D(width(), height()));
    m_shaderProgram->setUniformValue("time", static_cast<float>(m_timeOffset));
    m_shaderProgram->setUniformValue("amplitude", static_cast<float>(m_waveAmplitude));
    m_shaderProgram->setUniformValue("frequency", static_cast<float>(m_waveFrequency));
    m_shaderProgram->setUniformValue("speed", static_cast<float>(m_waveSpeed));
    m_shaderProgram->setUniformValue("glitchIntensity", static_cast<float>(m_glitchIntensity));
    m_shaderProgram->setUniformValue("waveThickness", static_cast<float>(m_waveThickness));

    // Rysujemy quad
    m_vao.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_vao.release();

    m_shaderProgram->release();
}

void CommunicationStream::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Tab) {
        event->accept(); // Zachowaj obsługę Tab
        return; // Zakończ przetwarzanie, aby uniknąć dalszych działań
    }

    if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
        const StreamMessage* currentMsg = m_messages[m_currentMessageIndex];

        switch (event->key()) {
            case Qt::Key_Right:
                if (currentMsg->nextButton()->isVisible()) {
                    showNextMessage();
                    event->accept(); // Akceptuj zdarzenie tylko jeśli akcja została wykonana
                }
                return; // Zakończ przetwarzanie dla tego klawisza

            case Qt::Key_Left:
                if (currentMsg->prevButton()->isVisible()) {
                    showPreviousMessage();
                    event->accept(); // Akceptuj zdarzenie tylko jeśli akcja została wykonana
                }
                return; // Zakończ przetwarzanie dla tego klawisza

            case Qt::Key_Return: // Używaj tylko Enter (Return)
                // case Qt::Key_Enter: // Qt::Key_Return zazwyczaj obejmuje Enter na numpadzie
                onMessageRead(); // Wywołaj zamknięcie wiadomości
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
                if (currentMsg->hasFocus()) {
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

void CommunicationStream::updateAnimation() {
    // Aktualizujemy czas animacji
    m_timeOffset += 0.02 * m_waveSpeed;

    // --- PŁYNNA ZMIANA AMPLITUDY ---
    // Interpolujemy aktualną amplitudę w kierunku docelowej
    // Współczynnik 0.1 oznacza, że w każdej klatce zbliżamy się o 10% do celu
    // Można dostosować (np. 0.2 dla szybszej reakcji, 0.05 dla wolniejszej)
    m_waveAmplitude = m_waveAmplitude * 0.9 + m_targetWaveAmplitude * 0.1;
    // --- KONIEC PŁYNNEJ ZMIANY ---

    // Stopniowo zmniejszamy intensywność zakłóceń (jeśli są używane niezależnie)
    if (m_glitchIntensity > 0.0) {
        m_glitchIntensity = qMax(0.0, m_glitchIntensity - 0.005);
    }

    // Wymuszamy aktualizację warstwy OpenGL
    const QRegion updateRegion(0, 0, width(), height());
    update(updateRegion);
}

void CommunicationStream::triggerRandomGlitch() {
    // Wywołujemy losowe zakłócenia tylko w trybie bezczynności
    if (m_state == Idle) {
        // Silniejsza intensywność dla lepszej widoczności
        const float intensity = 0.3 + QRandomGenerator::global()->bounded(40) / 100.0;

        // Wywołujemy animację zakłócenia
        startGlitchAnimation(intensity);
    }

    // Ustawiamy kolejny losowy interwał - częstsze zakłócenia
    m_glitchTimer->setInterval(1000 + QRandomGenerator::global()->bounded(2000));
}

void CommunicationStream::startReceivingAnimation() {
    // Zmieniamy stan
    m_state = Receiving;

    // Animujemy parametry fali
    const auto ampAnim = new QPropertyAnimation(this, "waveAmplitude");
    ampAnim->setDuration(1000);
    ampAnim->setStartValue(m_waveAmplitude);
    // Zamiast dużej amplitudy, ustawiamy tylko nieznacznie podniesioną bazę
    // Główna animacja amplitudy będzie pochodzić z setAudioAmplitude
    ampAnim->setEndValue(0.15); // Np. podwójna bazowa
    ampAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto freqAnim = new QPropertyAnimation(this, "waveFrequency");
    freqAnim->setDuration(1000);
    freqAnim->setStartValue(m_waveFrequency);
    freqAnim->setEndValue(6.0);  // Wyższa częstotliwość
    freqAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto speedAnim = new QPropertyAnimation(this, "waveSpeed");
    speedAnim->setDuration(1000);
    speedAnim->setStartValue(m_waveSpeed);
    speedAnim->setEndValue(2.5);  // Szybsza prędkość
    speedAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto thicknessAnim = new QPropertyAnimation(this, "waveThickness");
    thicknessAnim->setDuration(1000);
    thicknessAnim->setStartValue(m_waveThickness);
    thicknessAnim->setEndValue(0.012);  // Grubsza linia podczas aktywności
    thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
    glitchAnim->setDuration(1000);
    glitchAnim->setStartValue(0.0);
    glitchAnim->setKeyValueAt(0.3, 0.6);  // Większe zakłócenia
    glitchAnim->setKeyValueAt(0.6, 0.3);
    glitchAnim->setEndValue(0.2);
    glitchAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(ampAnim);
    group->addAnimation(freqAnim);
    group->addAnimation(speedAnim);
    group->addAnimation(thicknessAnim);
    group->addAnimation(glitchAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    // Zmieniamy stan na wyświetlanie po zakończeniu animacji
    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        m_targetWaveAmplitude = 0.15;
        m_state = Displaying;
    });
}

void CommunicationStream::returnToIdleAnimation() {
    // Zmieniamy stan
    m_state = Idle;

    m_targetWaveAmplitude = m_baseWaveAmplitude;
    // --- KONIEC RESETU ---

    // Animujemy powrót do stanu bezczynności
    const auto ampAnim = new QPropertyAnimation(this, "waveAmplitude");
    ampAnim->setDuration(1500);
    ampAnim->setStartValue(m_waveAmplitude);
    ampAnim->setEndValue(0.01); // Powrót do bazowej
    ampAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto freqAnim = new QPropertyAnimation(this, "waveFrequency");
    freqAnim->setDuration(1500);
    freqAnim->setStartValue(m_waveFrequency);
    freqAnim->setEndValue(2.0);
    freqAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto speedAnim = new QPropertyAnimation(this, "waveSpeed");
    speedAnim->setDuration(1500);
    speedAnim->setStartValue(m_waveSpeed);
    speedAnim->setEndValue(1.0);
    speedAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto thicknessAnim = new QPropertyAnimation(this, "waveThickness");
    thicknessAnim->setDuration(1500);
    thicknessAnim->setStartValue(m_waveThickness);
    thicknessAnim->setEndValue(0.008);  // Powrót do normalnej grubości
    thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(ampAnim);
    group->addAnimation(freqAnim);
    group->addAnimation(speedAnim);
    group->addAnimation(thicknessAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void CommunicationStream::startGlitchAnimation(const qreal intensity) {
    // Krótszy czas dla szybszego efektu
    const int duration = 600 + QRandomGenerator::global()->bounded(400);

    const auto glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
    glitchAnim->setDuration(duration);
    glitchAnim->setStartValue(m_glitchIntensity);
    glitchAnim->setKeyValueAt(0.1, intensity); // Szybki wzrost
    glitchAnim->setKeyValueAt(0.5, intensity * 0.7);
    glitchAnim->setEndValue(0.0);
    glitchAnim->setEasingCurve(QEasingCurve::OutQuad);
    glitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CommunicationStream::showMessageAtIndex(int index) {
    optimizeForMessageTransition(); // Optymalizacja (jeśli potrzebna)

    // Sprawdzenie poprawności indeksu i obsługa pustej listy
    if (index < 0 || index >= m_messages.size()) {
        qWarning() << "CommunicationStream::showMessageAtIndex - Nieprawidłowy indeks:" << index << ", rozmiar listy:" << m_messages.size();
        if (!m_messages.isEmpty()) {
            index = 0; // Pokaż pierwszą jako fallback, jeśli lista nie jest pusta
            qWarning() << "CommunicationStream::showMessageAtIndex - Ustawiono indeks na 0.";
        } else {
            m_currentMessageIndex = -1;
            returnToIdleAnimation();
            return; // Zakończ, jeśli lista jest pusta
        }
    }

    // Sprawdź, czy indeks się faktycznie zmienia
    if (index == m_currentMessageIndex && m_messages[index]->isVisible()) {
        qDebug() << "CommunicationStream::showMessageAtIndex - Indeks (" << index << ") jest taki sam jak bieżący i wiadomość jest widoczna. Nic nie robię.";
        // Upewnij się, że ma fokus
        m_messages[index]->setFocus();
        return;
    }


    // Ukrywamy poprzednią wiadomość (jeśli była inna niż nowa)
    if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
        if (m_currentMessageIndex != index) {
            StreamMessage* previousMsgPtr = nullptr;
            previousMsgPtr = m_messages[m_currentMessageIndex];
            qDebug() << "CommunicationStream::showMessageAtIndex - Ukrywanie poprzedniej wiadomości (indeks:" << m_currentMessageIndex << ")";
            disconnectSignalsForMessage(previousMsgPtr);
            previousMsgPtr->hide();
        }
    }

    // Ustaw nowy bieżący indeks
    m_currentMessageIndex = index;
    StreamMessage* message = m_messages[m_currentMessageIndex];
    qDebug() << "CommunicationStream::showMessageAtIndex - Pokazywanie wiadomości (indeks:" << m_currentMessageIndex << ")";

    // Ponownie podłączamy sygnały dla nowej, bieżącej wiadomości
    connectSignalsForMessage(message);

    // Ustawiamy stan
    m_state = Displaying;
    // m_targetWaveAmplitude = m_baseWaveAmplitude * 6;

    // --- ZMIANA KOLEJNOŚCI I METOD ---
    // 1. Upewnij się, że widget ma szansę obliczyć swój rozmiar
    message->adjustSize(); // Spróbuj wymusić obliczenie rozmiaru na podstawie zawartości
    // 2. Ustaw pozycję i rozmiar za pomocą setGeometry
    updateMessagePosition(); // Ta funkcja teraz użyje setGeometry
    // 3. Podnieś na wierzch
    message->raise();
    // 4. Pokaż widget
    message->show();
    // 5. Uruchom animację pojawiania się
    message->fadeIn();
    // 6. Zaktualizuj przyciski nawigacyjne
    updateNavigationButtonsForCurrentMessage();
    // 7. Ustaw fokus
    message->setFocus();
    // 8. Zaktualizuj tło OpenGL
    update();
    // --- KONIEC ZMIANY ---
}

void CommunicationStream::showNextMessage() {
    if (m_currentMessageIndex < m_messages.size() - 1) {
        optimizeForMessageTransition();
        showMessageAtIndex(m_currentMessageIndex + 1);
    }
}

void CommunicationStream::showPreviousMessage() {
    if (m_currentMessageIndex > 0) {
        optimizeForMessageTransition();
        showMessageAtIndex(m_currentMessageIndex - 1);
    }
}

void CommunicationStream::onMessageRead() {
    if (m_currentMessageIndex < 0 || m_currentMessageIndex >= m_messages.size()) {
        qWarning() << "CommunicationStream::onMessageRead - Brak bieżącej wiadomości do rozpoczęcia zamykania.";
        return;
    }

    // --- NOWA LOGIKA DLA ENTER ---
    // Ustaw flagę, że chcemy wyczyścić WSZYSTKIE wiadomości.
    // Proces rozpocznie się od zamknięcia bieżącej wiadomości.
    m_isClearingAllMessages = true;
    qDebug() << "CommunicationStream::onMessageRead - Inicjowanie czyszczenia wszystkich wiadomości. Zamykanie bieżącej (indeks:" << m_currentMessageIndex << ")";
    // --- KONIEC NOWEJ LOGIKI ---

    StreamMessage* message = m_messages[m_currentMessageIndex];
    message->markAsRead(); // Rozpocznij animację zamykania bieżącej wiadomości
    // Dalsza logika czyszczenia zostanie obsłużona w handleMessageHidden po zakończeniu animacji bieżącej wiadomości.
}

void CommunicationStream::handleMessageHidden() {
    const auto hiddenMsg = qobject_cast<StreamMessage*>(sender());
    if (!hiddenMsg) {
        qWarning() << "CommunicationStream::handleMessageHidden - Otrzymano sygnał hidden() od nieznanego obiektu.";
        return;
    }

    qDebug() << "CommunicationStream::handleMessageHidden - Wiadomość została ukryta. ID:" << hiddenMsg->messageId() << "Typ:" << hiddenMsg->type() << "m_isClearingAllMessages:" << m_isClearingAllMessages;

    // Znajdź indeks ukrytej wiadomości PRZED potencjalnym usunięciem
    const int hiddenMsgIndex = m_messages.indexOf(hiddenMsg);

    // Zawsze rozłączamy sygnały od ukrytej wiadomości
    disconnectSignalsForMessage(hiddenMsg);

    // --- NOWA LOGIKA DLA CZYSZCZENIA WSZYSTKICH WIADOMOŚCI ---
    if (m_isClearingAllMessages) {
        qDebug() << "CommunicationStream::handleMessageHidden - Tryb czyszczenia wszystkich wiadomości aktywny.";

        // Usuń ukrytą wiadomość z listy (jeśli tam jest) i zaplanuj jej usunięcie
        if (hiddenMsgIndex != -1) {
            m_messages.removeAt(hiddenMsgIndex);
        } else {
            qWarning() << "CommunicationStream::handleMessageHidden [ClearAll] - Nie znaleziono ukrytej wiadomości w liście!";
        }
        hiddenMsg->deleteLater();

        // Ukryj i usuń wszystkie POZOSTAŁE wiadomości
        // Tworzymy kopię listy wskaźników, aby bezpiecznie iterować i modyfikować oryginał
        QList<StreamMessage*> messagesToRemove = m_messages;
        m_messages.clear(); // Czyścimy oryginalną listę od razu

        qDebug() << "CommunicationStream::handleMessageHidden [ClearAll] - Usuwanie pozostałych" << messagesToRemove.size() << "wiadomości.";
        for (StreamMessage* msg : messagesToRemove) {
            if (msg) {
                disconnectSignalsForMessage(msg); // Rozłącz sygnały na wszelki wypadek
                msg->hide(); // Ukryj natychmiast
                msg->deleteLater(); // Zaplanuj usunięcie
            }
        }

        // Zresetuj stan strumienia
        m_currentMessageIndex = -1;
        m_isClearingAllMessages = false; // Zresetuj flagę
        returnToIdleAnimation();
        update();
        qDebug() << "CommunicationStream::handleMessageHidden [ClearAll] - Zakończono czyszczenie.";
        return; // Zakończ obsługę w tym trybie
    }
    // --- KONIEC NOWEJ LOGIKI ---


    // Poniżej znajduje się istniejąca logika dla normalnego ukrywania (progres, zamknięcie pojedynczej, nawigacja)

    // Sprawdzamy, czy to wiadomość progresu (ma niepuste ID)
    if (!hiddenMsg->messageId().isEmpty()) {
        // ... (istniejąca logika usuwania wiadomości progresu - bez zmian) ...
        qDebug() << "CommunicationStream::handleMessageHidden - Usuwanie wiadomości progresu ID:" << hiddenMsg->messageId();
        if (hiddenMsgIndex != -1) {
            m_messages.removeAt(hiddenMsgIndex);
            // Dostosuj indeks bieżącej wiadomości, jeśli usunięta była przed nią
            if (hiddenMsgIndex < m_currentMessageIndex) {
                m_currentMessageIndex--;
            } else if (hiddenMsgIndex == m_currentMessageIndex) {
                // Jeśli wiadomość progresu była jakimś cudem aktualna, zresetuj indeks
                m_currentMessageIndex = -1;
            }
        }
        hiddenMsg->deleteLater(); // Bezpieczne usunięcie widgetu

        // Sprawdź, czy po usunięciu są jeszcze jakieś wiadomości
        if (m_messages.isEmpty()) {
            m_currentMessageIndex = -1;
            returnToIdleAnimation();
        } else if (m_currentMessageIndex == -1) {
            returnToIdleAnimation();
        }
    } else {
        // To jest zwykła wiadomość, która została ukryta (i NIE jesteśmy w trybie czyszczenia).
        qDebug() << "CommunicationStream::handleMessageHidden - Zwykła wiadomość ukryta (indeks: " << hiddenMsgIndex << ", aktualny: " << m_currentMessageIndex << ")";

        if (hiddenMsgIndex == m_currentMessageIndex) {
            // Wiadomość była aktualnie wyświetlana i została zamknięta przez użytkownika (nie przez Enter w trybie ClearAll).
            qDebug() << "CommunicationStream::handleMessageHidden - Ukryta wiadomość była aktualnie wyświetlana (zamknięcie pojedyncze). Usuwanie i pokazywanie następnej.";

            // Usuwamy wiadomość z listy
            if (hiddenMsgIndex != -1) {
                m_messages.removeAt(hiddenMsgIndex);
            } else {
                qWarning() << "CommunicationStream::handleMessageHidden [SingleClose] - Nie znaleziono zamkniętej wiadomości w liście!";
            }
            hiddenMsg->deleteLater(); // Usuń widget

            // Pokaż następną lub wróć do Idle
            if (m_messages.isEmpty()) {
                qDebug() << "CommunicationStream::handleMessageHidden [SingleClose] - Brak więcej wiadomości, powrót do Idle.";
                m_currentMessageIndex = -1;
                returnToIdleAnimation();
            } else {
                int nextIndexToShow = qMin(hiddenMsgIndex, m_messages.size() - 1);
                qDebug() << "CommunicationStream::handleMessageHidden [SingleClose] - Są inne wiadomości. Próba pokazania indeksu:" << nextIndexToShow;
                QTimer::singleShot(0, this, [this, nextIndexToShow]() {
                    if (nextIndexToShow >= 0 && nextIndexToShow < m_messages.size()) {
                        showMessageAtIndex(nextIndexToShow);
                    } else if (!m_messages.isEmpty()){
                        showMessageAtIndex(0);
                    } else {
                        m_currentMessageIndex = -1;
                        returnToIdleAnimation();
                    }
                });
                m_currentMessageIndex = -1; // Tymczasowo resetuj indeks
            }
        } else {
            // Wiadomość ukryta podczas nawigacji. Nic nie robimy.
            qDebug() << "CommunicationStream::handleMessageHidden - Ukryta wiadomość nie była aktualnie wyświetlana (nawigacja). Nic nie robię.";
        }
    }
    update(); // Ogólna aktualizacja na koniec
}

UserVisuals CommunicationStream::generateUserVisuals(const QString &userId) {
    UserVisuals visuals;
    const quint32 hash = qHash(userId);

    // Wygeneruj kolor na podstawie hasha (w przestrzeni HSL dla lepszych kolorów)
    const int hue = hash % 360; // Odcień 0-359
    // Utrzymuj wysokie nasycenie i jasność dla neonowego efektu
    const int saturation = 230 + ((hash >> 8) % 26); // Nasycenie 230-255 (bardzo nasycone)
    const int lightness = 140 + ((hash >> 16) % 31); // Jasność 140-170 (jasne, ale nie białe)

    visuals.color = QColor::fromHsl(hue, saturation, lightness);
    return visuals;
}

void CommunicationStream::connectSignalsForMessage(StreamMessage *message) {
    if (!message) return;
    // Używamy UniqueConnection, aby uniknąć duplikatów połączeń
    connect(message->nextButton(), &QPushButton::clicked, this, &CommunicationStream::showNextMessage, Qt::UniqueConnection);
    connect(message->prevButton(), &QPushButton::clicked, this, &CommunicationStream::showPreviousMessage, Qt::UniqueConnection);
    connect(message->m_markReadButton, &QPushButton::clicked, this, &CommunicationStream::onMessageRead, Qt::UniqueConnection);
    // Podłączamy sygnał ukrycia wiadomości
    connect(message, &StreamMessage::hidden, this, &CommunicationStream::handleMessageHidden, Qt::UniqueConnection);
    qDebug() << "CommunicationStream::connectSignalsForMessage - Podłączono sygnały dla wiadomości (indeks: " << m_messages.indexOf(message) << ")";
}

void CommunicationStream::disconnectSignalsForMessage(StreamMessage *message) const {
    if (!message) return;
    // Rozłącz wszystkie sygnały od tej wiadomości do tego obiektu (CommunicationStream)
    // To powinno obejmować sygnał hidden() podłączony w connectSignalsForMessage
    disconnect(message, nullptr, this, nullptr);
    qDebug() << "CommunicationStream::disconnectSignalsForMessage - Rozłączono sygnały dla wiadomości (indeks: " << m_messages.indexOf(message) << ")";
}

void CommunicationStream::updateNavigationButtonsForCurrentMessage() {
    if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
        StreamMessage* currentMsg = m_messages[m_currentMessageIndex];
        const bool hasPrev = m_currentMessageIndex > 0;
        const bool hasNext = m_currentMessageIndex < m_messages.size() - 1;
        currentMsg->showNavigationButtons(hasPrev, hasNext);
    }
}

void CommunicationStream::optimizeForMessageTransition() const {
    // Czasowo zmniejszamy priorytet odświeżania animacji tła podczas przechodzenia między wiadomościami
    static QElapsedTimer transitionTimer;
    static bool inTransition = false;

    if (!inTransition) {
        inTransition = true;
        transitionTimer.start();

        // Podczas przejścia zmniejszamy częstotliwość odświeżania tła
        if (m_animTimer->interval() == 16) {
            m_animTimer->setInterval(33); // Tymczasowo 30fps zamiast 60fps
        }

        // Po 500ms wracamy do normalnego odświeżania
        QTimer::singleShot(500, this, [this]() {
            m_animTimer->setInterval(16); // Powrót do 60fps
            inTransition = false;
        });
    }
}

void CommunicationStream::updateMessagePosition() {
    if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
        StreamMessage* message = m_messages[m_currentMessageIndex];
        // Centrujemy wiadomość na fali (w środku ekranu)
        message->move((width() - message->width()) / 2, (height() - message->height()) / 2);
    }
}
