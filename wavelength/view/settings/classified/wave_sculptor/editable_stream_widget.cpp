#include "editable_stream_widget.h"
#include <QOpenGLShader>
#include <QVector2D>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath> // Dla std::abs, std::max, sin, fract, dot, floor
#include <QParallelAnimationGroup>
#include <QVector4D> // Dla koloru

EditableStreamWidget::EditableStreamWidget(QWidget* parent)
    : QOpenGLWidget(parent),
      m_waveAmplitude(0.05), // Mniejsza amplituda początkowa
      m_waveFrequency(1.5),
      m_waveSpeed(0.5),
      m_glitchIntensity(0.0),
      m_waveThickness(0.008), // Grubość jak w CommunicationStream
      m_timeOffset(0.0),
      m_shaderProgram(nullptr),
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer), // Ustaw typ bufora
      m_initialized(false)
{
    setMinimumSize(400, 150);
    // Usunięto setCursor(Qt::CrossCursor);

    // Timer animacji
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &EditableStreamWidget::updateAnimation);
    m_animTimer->setTimerType(Qt::PreciseTimer);
    m_animTimer->start(16); // ~60 FPS

    // Timer wygaszania glitcha (logika bez zmian)
    m_glitchDecayTimer = new QTimer(this);
    m_glitchDecayTimer->setInterval(50);
    connect(m_glitchDecayTimer, &QTimer::timeout, this, [this]() {
        if (m_glitchIntensity > 0.0) {
            m_glitchIntensity = qMax(0.0, m_glitchIntensity - 0.05);
            if (m_glitchIntensity == 0.0) {
                m_glitchDecayTimer->stop();
            }
            update();
        } else {
            m_glitchDecayTimer->stop();
        }
    });

    // Format dla OpenGL (bez zmian)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapInterval(1);
    setFormat(format);

    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // Usunięto initializeWavePoints();
}

EditableStreamWidget::~EditableStreamWidget() {
    makeCurrent();
    m_vao.destroy();
    m_vertexBuffer.destroy();
    delete m_shaderProgram;
    doneCurrent();
}

// Usunięto initializeWavePoints()

void EditableStreamWidget::initializeGL() {
    qDebug() << "EditableStreamWidget::initializeGL() started...";
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Przezroczyste tło

    m_shaderProgram = new QOpenGLShaderProgram();

    // Vertex shader (prosty, dla quada) - jak w CommunicationStream
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    // Fragment shader - skopiowany z CommunicationStream
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
            float glow = 0.03; // Szerokość poświaty (współrzędne Y)
            vec3 lineColor = vec3(0.0, 0.7, 1.0);  // Neonowy niebieski
            vec3 glowColor = vec3(0.0, 0.4, 0.8);  // Ciemniejszy niebieski dla poświaty

            // Podstawowa fala
            float xFreq = frequency * 3.14159;
            float tOffset = time; // * speed jest już w m_timeOffset
            float wave = sin(uv.x * xFreq + tOffset) * amplitude;

            // Przesuwające się zakłócenie
            if (glitchIntensity > 0.01) {
                float direction = sin(floor(time * 0.2) * 0.5) > 0.0 ? 1.0 : -1.0;
                float glitchPos = fract(time * 0.5) * 2.0 - 1.0;
                glitchPos = direction > 0.0 ? glitchPos : -glitchPos;
                float glitchWidth = 0.1 + glitchIntensity * 0.2;
                float distFromGlitch = abs(uv.x - glitchPos);
                float glitchFactor = smoothstep(glitchWidth, 0.0, distFromGlitch);
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
            float alpha = line + glowFactor * 0.6 + 0.1; // Minimalna alpha dla siatki

            FragColor = vec4(color, alpha);
        }
    )";

    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Vertex shader compilation failed:" << m_shaderProgram->log();
        return;
    }
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Fragment shader compilation failed:" << m_shaderProgram->log();
        return;
    }

    qDebug() << "Shaders compiled, linking program...";
    if (!m_shaderProgram->link()) {
        qCritical() << "Shader program linking failed:" << m_shaderProgram->log();
        return;
    }
    qDebug() << "Shader program linked successfully.";

    // VAO
    qDebug() << "Creating VAO...";
    m_vao.create();
    if (!m_vao.isCreated()) {
        qCritical() << "VAO creation failed!";
        return;
    }
    qDebug() << "VAO created.";
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    // VBO dla quada - jak w CommunicationStream
    qDebug() << "Creating VBO for quad...";
    static const GLfloat vertices[] = {
       -1.0f,  1.0f,
       -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f
    };
    m_vertexBuffer.create();
    if (!m_vertexBuffer.isCreated()) {
        qCritical() << "VBO creation failed!";
        return;
    }
     qDebug() << "VBO created.";
    m_vertexBuffer.bind();
    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw); // Quad jest statyczny
    qDebug() << "Allocating VBO for quad...";
    m_vertexBuffer.allocate(vertices, sizeof(vertices));
    qDebug() << "VBO allocated.";

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    m_vertexBuffer.release();
    // vaoBinder zwalnia VAO

    m_initialized = true; // Ustaw flagę
    qDebug() << "EditableStreamWidget::initializeGL() completed successfully.";
    update(); // Wymuś odświeżenie
}

void EditableStreamWidget::resizeGL(int w, int h) {
    if (!m_initialized) return;
    glViewport(0, 0, w, h);
}

void EditableStreamWidget::paintGL() {
    if (!m_initialized) {
        glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT);

    m_shaderProgram->bind();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    // Ustawianie uniformów - jak w CommunicationStream
    m_shaderProgram->setUniformValue("resolution", QVector2D(width(), height()));
    m_shaderProgram->setUniformValue("time", static_cast<float>(m_timeOffset));
    m_shaderProgram->setUniformValue("amplitude", static_cast<float>(m_waveAmplitude));
    m_shaderProgram->setUniformValue("frequency", static_cast<float>(m_waveFrequency));
    m_shaderProgram->setUniformValue("speed", static_cast<float>(m_waveSpeed)); // Speed jest używany w shaderze CommunicationStream? Nie, ale zostawmy na razie.
    m_shaderProgram->setUniformValue("glitchIntensity", static_cast<float>(m_glitchIntensity));
    m_shaderProgram->setUniformValue("waveThickness", static_cast<float>(m_waveThickness));

    // Rysowanie quada - jak w CommunicationStream
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // vaoBinder zwalnia VAO
    m_shaderProgram->release();
    // doneCurrent();
}

void EditableStreamWidget::updateAnimation() {
    if (!m_initialized) return; // Sprawdzenie flagi

    // Aktualizujemy czas animacji - jak w CommunicationStream
    m_timeOffset += 0.02 * m_waveSpeed; // m_waveSpeed kontroluje prędkość upływu czasu

    // Stopniowo zmniejszamy intensywność zakłóceń (bez zmian)
    // if (m_glitchIntensity > 0.0) { ... } // Ta logika jest w timerze m_glitchDecayTimer

    // Wymuszamy odświeżenie widgetu OpenGL
    update(); // update() w QOpenGLWidget automatycznie wywoła paintGL
}

// Usunięto updateVertexBuffer()

// Usunięto mousePressEvent, mouseMoveEvent, mouseReleaseEvent

// Usunięto findClosestPoint()

// --- Metody do wywoływania symulacji ---
// Implementacje triggerGlitch, triggerReceivingAnimation, returnToIdleAnimation
// pozostają takie same, ponieważ modyfikują tylko właściwości (uniformy).

void EditableStreamWidget::triggerGlitch(qreal intensity) {
    qDebug() << "Triggering glitch with intensity:" << intensity;
    m_glitchIntensity = qMax(m_glitchIntensity, intensity);
    if (!m_glitchDecayTimer->isActive()) {
        m_glitchDecayTimer->start();
    }
}

void EditableStreamWidget::triggerReceivingAnimation() {
    qDebug() << "Triggering receiving animation";
    // Animacje QPropertyAnimation bez zmian
    QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
    ampAnim->setDuration(1000);
    ampAnim->setStartValue(m_waveAmplitude);
    ampAnim->setEndValue(0.15);
    ampAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
    freqAnim->setDuration(1000);
    freqAnim->setStartValue(m_waveFrequency);
    freqAnim->setEndValue(6.0);
    freqAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
    speedAnim->setDuration(1000);
    speedAnim->setStartValue(m_waveSpeed);
    speedAnim->setEndValue(2.5);
    speedAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* thicknessAnim = new QPropertyAnimation(this, "waveThickness");
    thicknessAnim->setDuration(1000);
    thicknessAnim->setStartValue(m_waveThickness);
    thicknessAnim->setEndValue(0.012);
    thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
    glitchAnim->setDuration(1000);
    glitchAnim->setStartValue(m_glitchIntensity);
    glitchAnim->setKeyValueAt(0.3, 0.6);
    glitchAnim->setKeyValueAt(0.6, 0.3);
    glitchAnim->setEndValue(0.2);
    glitchAnim->setEasingCurve(QEasingCurve::OutQuad);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(ampAnim);
    group->addAnimation(freqAnim);
    group->addAnimation(speedAnim);
    group->addAnimation(thicknessAnim);
    group->addAnimation(glitchAnim);

    connect(group, &QParallelAnimationGroup::finished, this, &EditableStreamWidget::returnToIdleAnimation);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    if (!m_glitchDecayTimer->isActive()) {
        m_glitchDecayTimer->start();
    }
}

void EditableStreamWidget::returnToIdleAnimation() {
    qDebug() << "Returning to idle animation";
    // Animacje QPropertyAnimation bez zmian
    QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
    ampAnim->setDuration(1500);
    ampAnim->setStartValue(m_waveAmplitude);
    ampAnim->setEndValue(0.05); // Wróć do małej amplitudy (zamiast 0.01 z CommStream)
    ampAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
    freqAnim->setDuration(1500);
    freqAnim->setStartValue(m_waveFrequency);
    freqAnim->setEndValue(1.5); // Wróć do niskiej częstotliwości (zamiast 2.0 z CommStream)
    freqAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
    speedAnim->setDuration(1500);
    speedAnim->setStartValue(m_waveSpeed);
    speedAnim->setEndValue(0.5); // Wróć do niskiej prędkości (zamiast 1.0 z CommStream)
    speedAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* thicknessAnim = new QPropertyAnimation(this, "waveThickness");
    thicknessAnim->setDuration(1500);
    thicknessAnim->setStartValue(m_waveThickness);
    thicknessAnim->setEndValue(0.008); // Wróć do standardowej grubości
    thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(ampAnim);
    group->addAnimation(freqAnim);
    group->addAnimation(speedAnim);
    group->addAnimation(thicknessAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}