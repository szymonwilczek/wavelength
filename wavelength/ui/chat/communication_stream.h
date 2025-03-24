#ifndef COMMUNICATION_STREAM_H
#define COMMUNICATION_STREAM_H

#include <qcoreapplication.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QGraphicsEffect>
#include <QSequentialAnimationGroup>
#include <QKeyEvent>
#include <QQueue>
#include <QPainterPath>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QVBoxLayout>

#include "stream_message.h"

// Nowy komponent sidebar do wyświetlania listy wiadomości po prawej stronie
class MessageSidebar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int width READ width WRITE setFixedWidth)

public:
    explicit MessageSidebar(QWidget* parent = nullptr) : QWidget(parent), m_expanded(false) {
        setFixedWidth(0); // Początkowo zwinięty
        setMinimumHeight(200);

        // Główny układ pionowy
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(1);

        // Przycisk rozwijania/zwijania
        m_toggleButton = new QPushButton(">", this);
        m_toggleButton->setFixedSize(30, 60);
        m_toggleButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(0, 80, 120, 0.7);"
            "  color: #00ccff;"
            "  border: 1px solid #00aaff;"
            "  border-top-left-radius: 15px;"
            "  border-bottom-left-radius: 15px;"
            "  border-right: none;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: rgba(0, 100, 150, 0.8); }");

        // Obszar przewijania dla wiadomości
        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setStyleSheet(
            "QScrollArea { background-color: transparent; border: none; }"
            "QScrollBar:vertical { background: rgba(20, 30, 40, 0.5); width: 10px; }"
            "QScrollBar::handle:vertical { background: rgba(0, 150, 200, 0.5); border-radius: 4px; min-height: 20px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");

        // Kontener na wiadomości
        m_messageContainer = new QWidget();
        m_messageContainer->setStyleSheet("background-color: transparent;");
        m_messageLayout = new QVBoxLayout(m_messageContainer);
        m_messageLayout->setAlignment(Qt::AlignTop);
        m_messageLayout->setContentsMargins(2, 5, 2, 5);
        m_messageLayout->setSpacing(5);

        m_scrollArea->setWidget(m_messageContainer);
        mainLayout->addWidget(m_scrollArea);

        // Umieszczamy przycisk na lewo od panelu
        m_toggleButton->move(-m_toggleButton->width(), height()/2 - m_toggleButton->height()/2);
        m_toggleButton->raise();

        // Podłączamy sygnał przycisku
        connect(m_toggleButton, &QPushButton::clicked, this, &MessageSidebar::toggleExpanded);

        // Włączamy płynne przewijanie dotykiem
        QScroller::grabGesture(m_scrollArea->viewport(), QScroller::TouchGesture);
    }

    void addMessageItem(const QString& sender, const QString& preview, StreamMessage::MessageType type, int index) {
        // Tworzymy element reprezentujący wiadomość
        QWidget* item = new QWidget(m_messageContainer);
        item->setProperty("index", index);

        QVBoxLayout* itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(8, 8, 8, 8);
        itemLayout->setSpacing(3);

        // Etykieta nadawcy
        QLabel* senderLabel = new QLabel(sender, item);
        senderLabel->setStyleSheet(
            "color: #ffffff; font-weight: bold; font-size: 9pt; "
            "font-family: 'Consolas', monospace;");

        // Podgląd treści
        QLabel* previewLabel = new QLabel(preview.left(20) + (preview.length() > 20 ? "..." : ""), item);
        previewLabel->setStyleSheet("color: #cccccc; font-size: 8pt;");

        itemLayout->addWidget(senderLabel);
        itemLayout->addWidget(previewLabel);

        // Stylizacja elementu zależnie od typu wiadomości
        QString borderColor;
        switch (type) {
            case StreamMessage::Transmitted:
                borderColor = "#00ccff";
                break;
            case StreamMessage::Received:
                borderColor = "#cc55ff";
                break;
            case StreamMessage::System:
                borderColor = "#ffcc33";
                break;
        }

        item->setStyleSheet(QString(
            "QWidget {"
            "  background-color: rgba(10, 15, 20, 180);"
            "  border-left: 3px solid %1;"
            "  border-radius: 2px;"
            "}"
            "QWidget:hover {"
            "  background-color: rgba(20, 30, 40, 220);"
            "}").arg(borderColor));

        item->setCursor(Qt::PointingHandCursor);

        // Instalujemy filtr zdarzeń do obsługi kliknięć
        item->installEventFilter(this);

        // Dodajemy element do layoutu
        m_messageLayout->addWidget(item);

        // Przewijamy do najnowszego elementu
        QTimer::singleShot(100, this, [this]() {
            m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
        });
    }

    void clearItems() {
        // Usuń wszystkie elementy
        QLayoutItem* item;
        while ((item = m_messageLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }

public slots:
    void toggleExpanded() {
        m_expanded = !m_expanded;

        // Animacja rozwijania/zwijania
        QPropertyAnimation* anim = new QPropertyAnimation(this, "width");
        anim->setDuration(300);
        anim->setStartValue(width());
        anim->setEndValue(m_expanded ? 200 : 0);
        anim->setEasingCurve(QEasingCurve::OutCubic);

        // Zmiana tekstu przycisku
        m_toggleButton->setText(m_expanded ? "<" : ">");

        // Po zakończeniu animacji aktualizujemy pozycję przycisku
        connect(anim, &QPropertyAnimation::finished, this, [this]() {
            updateToggleButtonPosition();
        });

        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        updateToggleButtonPosition();
    }

signals:
    void messageSelected(int index);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            QWidget* widget = qobject_cast<QWidget*>(watched);
            if (widget && widget->property("index").isValid()) {
                int index = widget->property("index").toInt();
                emit messageSelected(index);
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

private:
    void updateToggleButtonPosition() {
        // Ustawiamy przycisk po lewej stronie sidebara
        m_toggleButton->move(-m_toggleButton->width(), height()/2 - m_toggleButton->height()/2);
        m_toggleButton->raise();
    }

    bool m_expanded;
    QPushButton* m_toggleButton;
    QScrollArea* m_scrollArea;
    QWidget* m_messageContainer;
    QVBoxLayout* m_messageLayout;
};

// Główny widget strumienia komunikacji - poprawiona wersja z OpenGL
class CommunicationStream : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude)
    Q_PROPERTY(qreal waveFrequency READ waveFrequency WRITE setWaveFrequency)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    Q_PROPERTY(qreal waveThickness READ waveThickness WRITE setWaveThickness)

public:
    enum StreamState {
        Idle,        // Prosta linia z okazjonalnymi zakłóceniami
        Receiving,   // Aktywna animacja podczas otrzymywania wiadomości
        Displaying   // Wyświetlanie wiadomości
    };

    explicit CommunicationStream(QWidget* parent = nullptr)
        : QOpenGLWidget(parent),
          m_waveAmplitude(0.01), m_waveFrequency(2.0), m_waveSpeed(1.0),
          m_glitchIntensity(0.0), m_waveThickness(0.008), // Grubsza linia
          m_state(Idle), m_currentMessageIndex(-1),
          m_initialized(false), m_timeOffset(0.0), m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
          m_shaderProgram(nullptr)
    {
        setMinimumSize(600, 200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // Timer animacji
        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, &CommunicationStream::updateAnimation);
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

        // Sidebar z wiadomościami
        m_sidebar = new MessageSidebar(this);
        m_sidebar->move(width(), 0);
        connect(m_sidebar, &MessageSidebar::messageSelected, this, &CommunicationStream::showMessageAtIndex);

        // Format dla OpenGL
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setSamples(4); // MSAA
        setFormat(format);
    }

    ~CommunicationStream() {
        makeCurrent();
        m_vao.destroy();
        m_vertexBuffer.destroy();
        delete m_shaderProgram;
        doneCurrent();
    }

    StreamMessage* addMessageWithAttachment(const QString& content, const QString& sender, StreamMessage::MessageType type, const QString& messageId = QString()) {
        // Najpierw rozpoczynamy animację odbioru
        startReceivingAnimation();

        // Tworzymy nową wiadomość, ale nie wyświetlamy jej od razu
        StreamMessage* message = new StreamMessage(content, sender, type, this);
        message->hide();

        // Dodajemy obsługę załącznika
        // Najpierw wypisujemy info o zawartości dla debugowania
        qDebug() << "Dodawanie załącznika dla wiadomości: " << content.left(30) << "...";
        message->addAttachment(content); // Ten punkt może być problemem

        // Dodajemy wiadomość do kolejki
        m_messages.append(message);

        // Dodajemy element w sidebarze
        m_sidebar->addMessageItem(sender, content, type, m_messages.size() - 1);

        // Podłączamy sygnały do nawigacji między wiadomościami
        connect(message, &StreamMessage::messageRead, this, &CommunicationStream::onMessageRead);
        connect(message->nextButton(), &QPushButton::clicked, this, &CommunicationStream::showNextMessage);
        connect(message->prevButton(), &QPushButton::clicked, this, &CommunicationStream::showPreviousMessage);

        // Jeśli nie ma wyświetlanej wiadomości, pokaż tę po zakończeniu animacji odbioru
        if (m_currentMessageIndex == -1) {
            QTimer::singleShot(1200, this, [this]() {
                showMessageAtIndex(m_messages.size() - 1);
            });
        }

        return message;
    }

    // Settery i gettery dla animacji fali
    qreal waveAmplitude() const { return m_waveAmplitude; }
    void setWaveAmplitude(qreal amplitude) {
        m_waveAmplitude = amplitude;
        update();
    }

    qreal waveFrequency() const { return m_waveFrequency; }
    void setWaveFrequency(qreal frequency) {
        m_waveFrequency = frequency;
        update();
    }

    qreal waveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(qreal speed) {
        m_waveSpeed = speed;
        update();
    }

    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity) {
        m_glitchIntensity = intensity;
        update();
    }

    qreal waveThickness() const { return m_waveThickness; }
    void setWaveThickness(qreal thickness) {
        m_waveThickness = thickness;
        update();
    }

    void setStreamName(const QString& name) {
        m_streamNameLabel->setText(name);
        m_streamNameLabel->adjustSize();
        m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);
    }

    StreamMessage *addMessage(const QString &content, const QString &sender, StreamMessage::MessageType type) {
        // Najpierw rozpoczynamy animację odbioru
        startReceivingAnimation();

        // Tworzymy nową wiadomość, ale nie wyświetlamy jej od razu
        StreamMessage* message = new StreamMessage(content, sender, type, this);
        message->hide();

        // Dodajemy wiadomość do kolejki
        m_messages.append(message);

        // Dodajemy element w sidebarze
        m_sidebar->addMessageItem(sender, content, type, m_messages.size() - 1);

        // Podłączamy sygnały do nawigacji między wiadomościami
        connect(message, &StreamMessage::messageRead, this, &CommunicationStream::onMessageRead);
        connect(message->nextButton(), &QPushButton::clicked, this, &CommunicationStream::showNextMessage);
        connect(message->prevButton(), &QPushButton::clicked, this, &CommunicationStream::showPreviousMessage);

        // Jeśli nie ma wyświetlanej wiadomości lub używamy nowego podejścia z zakładkami,
        // pokazujemy tę wiadomość po zakończeniu animacji odbioru
        if (m_currentMessageIndex == -1) {
            QTimer::singleShot(1200, this, [this]() {
                showMessageAtIndex(m_messages.size() - 1);
            });
        }
        return message;
    }

    void clearMessages() {
        // Ukrywamy i usuwamy wszystkie wiadomości
        for (auto message : m_messages) {
            message->deleteLater();
        }

        m_messages.clear();
        m_sidebar->clearItems();
        m_currentMessageIndex = -1;
        returnToIdleAnimation();
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Inicjalizujemy shader program
        m_shaderProgram = new QOpenGLShaderProgram();

        // Vertex shader
        const char* vertexShaderSource = R"(
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
const char* fragmentShaderSource = R"(
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
        static const GLfloat vertices[] = {
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

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);

        // Aktualizujemy pozycję etykiety nazwy strumienia
        m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);

        // Aktualizujemy pozycję sidebara
        m_sidebar->setFixedHeight(height());
        m_sidebar->move(width() - m_sidebar->width(), 0);

        // Aktualizujemy pozycję aktualnie wyświetlanej wiadomości
        updateMessagePosition();
    }

    void paintGL() override {
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

    void keyPressEvent(QKeyEvent* event) override {
        // Globalna obsługa klawiszy
        if (event->key() == Qt::Key_Tab) {
            // Przełączamy widoczność sidebara
            m_sidebar->toggleExpanded();
            event->accept();
        } else if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
            // Przekazujemy obsługę klawiszy do aktywnej wiadomości
            QKeyEvent* newEvent = new QKeyEvent(event->type(), event->key(), event->modifiers(),
                                               event->text(), event->isAutoRepeat(), event->count());
            QCoreApplication::postEvent(m_messages[m_currentMessageIndex], newEvent);
            event->accept();
        } else {
            QOpenGLWidget::keyPressEvent(event);
        }
    }

private slots:
    void updateAnimation() {
        // Aktualizujemy czas animacji - mniejsza prędkość dla mniej intensywnej animacji
        m_timeOffset += 0.02 * m_waveSpeed;

        // Stopniowo zmniejszamy intensywność zakłóceń
        if (m_glitchIntensity > 0.0) {
            m_glitchIntensity = qMax(0.0, m_glitchIntensity - 0.005);
        }

        // Odświeżamy tylko gdy potrzeba
        if (m_state != Idle || m_glitchIntensity > 0.01) {
            update();
        }
    }

    void triggerRandomGlitch() {
        // Wywołujemy losowe zakłócenia tylko w trybie bezczynności
        if (m_state == Idle) {
            // Silniejsza intensywność dla lepszej widoczności
            float intensity = 0.3 + QRandomGenerator::global()->bounded(40) / 100.0;

            // Wywołujemy animację zakłócenia
            startGlitchAnimation(intensity);
        }

        // Ustawiamy kolejny losowy interwał - częstsze zakłócenia
        m_glitchTimer->setInterval(1000 + QRandomGenerator::global()->bounded(2000));
    }

    void startReceivingAnimation() {
        // Zmieniamy stan
        m_state = Receiving;

        // Animujemy parametry fali
        QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
        ampAnim->setDuration(1000);
        ampAnim->setStartValue(m_waveAmplitude);
        ampAnim->setEndValue(0.15);  // Większa amplituda
        ampAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
        freqAnim->setDuration(1000);
        freqAnim->setStartValue(m_waveFrequency);
        freqAnim->setEndValue(6.0);  // Wyższa częstotliwość
        freqAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
        speedAnim->setDuration(1000);
        speedAnim->setStartValue(m_waveSpeed);
        speedAnim->setEndValue(2.5);  // Szybsza prędkość
        speedAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* thicknessAnim = new QPropertyAnimation(this, "waveThickness");
        thicknessAnim->setDuration(1000);
        thicknessAnim->setStartValue(m_waveThickness);
        thicknessAnim->setEndValue(0.012);  // Grubsza linia podczas aktywności
        thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(1000);
        glitchAnim->setStartValue(0.0);
        glitchAnim->setKeyValueAt(0.3, 0.6);  // Większe zakłócenia
        glitchAnim->setKeyValueAt(0.6, 0.3);
        glitchAnim->setEndValue(0.2);
        glitchAnim->setEasingCurve(QEasingCurve::OutQuad);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(ampAnim);
        group->addAnimation(freqAnim);
        group->addAnimation(speedAnim);
        group->addAnimation(thicknessAnim);
        group->addAnimation(glitchAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);

        // Zmieniamy stan na wyświetlanie po zakończeniu animacji
        connect(group, &QParallelAnimationGroup::finished, this, [this]() {
            m_state = Displaying;
        });
    }

    void returnToIdleAnimation() {
        // Zmieniamy stan
        m_state = Idle;

        // Animujemy powrót do stanu bezczynności
        QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
        ampAnim->setDuration(1500);
        ampAnim->setStartValue(m_waveAmplitude);
        ampAnim->setEndValue(0.01);
        ampAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
        freqAnim->setDuration(1500);
        freqAnim->setStartValue(m_waveFrequency);
        freqAnim->setEndValue(2.0);
        freqAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
        speedAnim->setDuration(1500);
        speedAnim->setStartValue(m_waveSpeed);
        speedAnim->setEndValue(1.0);
        speedAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* thicknessAnim = new QPropertyAnimation(this, "waveThickness");
        thicknessAnim->setDuration(1500);
        thicknessAnim->setStartValue(m_waveThickness);
        thicknessAnim->setEndValue(0.008);  // Powrót do normalnej grubości
        thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(ampAnim);
        group->addAnimation(freqAnim);
        group->addAnimation(speedAnim);
        group->addAnimation(thicknessAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startGlitchAnimation(qreal intensity) {
        // Krótszy czas dla szybszego efektu
        int duration = 600 + QRandomGenerator::global()->bounded(400);

        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(duration);
        glitchAnim->setStartValue(m_glitchIntensity);
        glitchAnim->setKeyValueAt(0.1, intensity); // Szybki wzrost
        glitchAnim->setKeyValueAt(0.5, intensity * 0.7);
        glitchAnim->setEndValue(0.0);
        glitchAnim->setEasingCurve(QEasingCurve::OutQuad);
        glitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void showMessageAtIndex(int index) {
        if (index < 0 || index >= m_messages.size()) return;

        // Ukrywamy aktualnie wyświetlaną wiadomość
        if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
            m_messages[m_currentMessageIndex]->fadeOut();
        }

        m_currentMessageIndex = index;

        // Wyświetlamy wybraną wiadomość
        StreamMessage* message = m_messages[m_currentMessageIndex];
        message->show();
        updateMessagePosition();
        message->fadeIn();

        // Aktualizujemy przyciski nawigacyjne
        bool hasNext = index < m_messages.size() - 1;
        bool hasPrev = index > 0;
        message->showNavigationButtons(hasPrev, hasNext);
    }

    void showNextMessage() {
        if (m_currentMessageIndex >= m_messages.size() - 1) return;
        showMessageAtIndex(m_currentMessageIndex + 1);
    }

    void showPreviousMessage() {
        if (m_currentMessageIndex <= 0) return;
        showMessageAtIndex(m_currentMessageIndex - 1);
    }

    void onMessageRead() {
        if (m_currentMessageIndex < 0 || m_currentMessageIndex >= m_messages.size()) return;

        StreamMessage* message = m_messages[m_currentMessageIndex];

        // Przejdź do następnej wiadomości, jeśli istnieje
        if (m_currentMessageIndex < m_messages.size() - 1) {
            showNextMessage();
        } else {
            // Jeśli to była ostatnia wiadomość, wróć do stanu bezczynności
            m_currentMessageIndex = -1;
            QTimer::singleShot(1500, this, &CommunicationStream::returnToIdleAnimation);
        }

        // Usuń przeczytaną wiadomość z naszej listy
        connect(message, &StreamMessage::hidden, this, [this, message]() {
            int index = m_messages.indexOf(message);
            if (index >= 0) {
                m_messages.removeAt(index);

                // Aktualizujemy indeksy w sidebarze
                m_sidebar->clearItems();
                for (int i = 0; i < m_messages.size(); ++i) {
                    m_sidebar->addMessageItem(
                        m_messages[i]->sender(),
                        m_messages[i]->content(),
                        m_messages[i]->type(),
                        i
                    );
                }

                // Aktualizujemy bieżący indeks
                if (m_currentMessageIndex >= index && m_currentMessageIndex > 0) {
                    m_currentMessageIndex--;
                }
            }

            message->deleteLater();
        });
    }

private:
    void updateMessagePosition() {
        if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.size()) {
            StreamMessage* message = m_messages[m_currentMessageIndex];
            // Centrujemy wiadomość na fali (w środku ekranu)
            message->move((width() - message->width()) / 2, (height() - message->height()) / 2);
        }
    }

    // Parametry animacji fali
    qreal m_waveAmplitude;
    qreal m_waveFrequency;
    qreal m_waveSpeed;
    qreal m_glitchIntensity;
    qreal m_waveThickness;

    StreamState m_state;
    QList<StreamMessage*> m_messages;  // Zmienione z QQueue na QList dla indeksowania
    int m_currentMessageIndex;

    QLabel* m_streamNameLabel;
    MessageSidebar* m_sidebar;
    bool m_initialized;
    qreal m_timeOffset;

    QTimer* m_animTimer;
    QTimer* m_glitchTimer;

    // OpenGL
    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vertexBuffer;
};

#endif // COMMUNICATION_STREAM_H