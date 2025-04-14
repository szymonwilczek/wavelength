#include "floating_energy_sphere_widget.h"
#include <QOpenGLShader>
#include <QVector3D>
#include <cmath>
#include <QDebug>
#include <QApplication> // Dla qApp->quit() w closeEvent
#include <random>

// --- Proste Shaders ---
// Vertex Shader: Transformuje wierzchołki i przekazuje pozycję/normalną do fragment shadera
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos; // Pozycja bazowa punktu

    out float PointSize; // Rozmiar punktu przekazywany do fragment shadera (opcjonalnie)
    out float Intensity; // Intensywność punktu (np. do koloru)

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time;
    uniform float glitchIntensity; // Kontrola glitcha z CPU

    // Funkcja szumu (prosta wersja)
    float rand(vec2 co){
        return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    }

    void main()
    {
        // Podstawowa pozycja
        vec3 pos = aPos;

        // Efekt fal energii - modyfikacja promienia
        float radius = length(pos);
        float angleY = atan(pos.y, pos.x);
        float angleZ = atan(sqrt(pos.x*pos.x + pos.y*pos.y), pos.z);
        float waveFactor = (sin(angleY * 5.0 + time * 3.0) * 0.5 + 0.5) * (cos(angleZ * 7.0 - time * 2.0) * 0.5 + 0.5);
        float currentRadius = 0.8 + waveFactor * 0.4; // Bazowy promień + fale

        // Glitch - losowe przemieszczenie wierzchołków
        float glitchAmount = rand(pos.xy + time) * glitchIntensity * 0.5; // Losowe przemieszczenie
        vec3 glitchOffset = normalize(pos) * glitchAmount;

        // Zastosuj nowy promień i glitch
        pos = normalize(pos) * currentRadius + glitchOffset;

        // Transformacje standardowe
        gl_Position = projection * view * model * vec4(pos, 1.0);

        // Ustaw rozmiar punktu (może być stały lub zależny od odległości/fali)
        gl_PointSize = mix(2.0, 6.0, waveFactor); // Rozmiar zależny od fali
        PointSize = gl_PointSize;

        // Ustaw intensywność (np. na podstawie fali)
        Intensity = smoothstep(0.3, 0.7, waveFactor);
    }
)";

// Fragment Shader: Koloruje punkty, tworzy efekt poświaty
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in float Intensity; // Intensywność z vertex shadera

    uniform float time;
    uniform float glitchIntensity;

    // Funkcja szumu
    float rand(vec2 co){
        return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    }

    void main()
    {
        // Kształt punktu (okrągły zamiast kwadratowego)
        vec2 coord = gl_PointCoord - vec2(0.5);
        if(length(coord) > 0.5) {
            discard; // Odrzuć piksele poza okręgiem
        }

        // Kolor bazowy (neonowy niebieski/fioletowy) z modulacją intensywności
        vec3 baseColor = mix(vec3(0.1, 0.2, 0.9), vec3(0.6, 0.1, 0.8), Intensity);
        vec3 glowColor = vec3(0.8, 0.9, 1.0); // Jaśniejszy kolor poświaty

        // Mieszanie koloru na podstawie odległości od środka punktu (efekt poświaty)
        float glow = 1.0 - smoothstep(0.3, 0.5, length(coord));
        vec3 finalColor = mix(baseColor, glowColor, glow * 0.8);

        // Glitch koloru
        float colorGlitch = rand(gl_PointCoord + time * 0.1) * glitchIntensity * 0.5;
        if (colorGlitch > 0.4) {
             finalColor = mix(finalColor, vec3(1.0, 0.2, 0.2), smoothstep(0.4, 0.5, colorGlitch)); // Czerwony glitch
        } else if (colorGlitch > 0.3) {
             finalColor = mix(finalColor, vec3(0.2, 1.0, 0.2), smoothstep(0.3, 0.4, colorGlitch)); // Zielony glitch
        }

        FragColor = vec4(finalColor, 1.0);
    }
)";
// --- Koniec Shaders ---


FloatingEnergySphereWidget::FloatingEnergySphereWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      QOpenGLFunctions_3_3_Core(),
      m_timeValue(0.0f),
      m_glitchIntensity(0.0f), // Początkowo brak glitcha
      m_program(nullptr),
      m_vbo(QOpenGLBuffer::VertexBuffer),
      // m_ebo nie jest już potrzebne dla GL_POINTS
      m_cameraPosition(0.0f, 0.0f, 3.0f),
      m_cameraDistance(3.0f),
      m_mousePressed(false),
      m_pointCount(0),
      m_closable(true) // Domyślnie można zamknąć
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    resize(500, 500); // Trochę większy widget

    setStyleSheet("background-color: rgba(255, 0, 0, 0.5);");

    connect(&m_timer, &QTimer::timeout, this, &FloatingEnergySphereWidget::updateAnimation);
    m_timer.start(16);
}

FloatingEnergySphereWidget::~FloatingEnergySphereWidget()
{
    makeCurrent();
    m_vbo.destroy();
    m_vao.destroy();
    delete m_program;
    doneCurrent();
    qDebug() << "FloatingEnergySphereWidget destroyed";
}

void FloatingEnergySphereWidget::setClosable(bool closable) {
    m_closable = closable;
}

void FloatingEnergySphereWidget::initializeGL()
{
    qDebug() << "FloatingEnergySphereWidget::initializeGL() started."; // Dodaj log
    if (!initializeOpenGLFunctions()) {
        qCritical() << "Failed to initialize OpenGL functions!";
        return;
    }
    qDebug() << "OpenGL functions initialized."; // Dodaj log

    glClearColor(0.0f, 0.0f, 0.0f, 0.001f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE); // Włącz możliwość ustawiania rozmiaru punktu w shaderze

    qDebug() << "Setting up shaders..."; // Dodaj log
    setupShaders();
    if (!m_program || !m_program->isLinked()) {
        qCritical() << "Shader program not linked after setupShaders()!";
        // Można tu dodać dalsze logowanie błędów shadera, jeśli są dostępne
        if(m_program) qCritical() << "Shader Log:" << m_program->log();
        return; // Przerwij, jeśli shadery się nie powiodły
    }
    qDebug() << "Shaders set up successfully."; // Dodaj log
    setupGeometry(); // Używa nowej metody
    qDebug() << "Geometry set up with" << m_pointCount << "points."; // Dodaj log

    m_vao.create();
    m_vao.bind();
    m_vbo.create();

    m_vbo.bind();
    m_vbo.allocate(m_points.data(), m_points.size() * sizeof(GLfloat));

    qDebug() << "VAO/VBO configured."; // Dodaj log
    qDebug() << "FloatingEnergySphereWidget::initializeGL() finished successfully."; // Dodaj log

    // Atrybut wierzchołków (tylko pozycja)
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(GLfloat)); // 3 floaty, krok 3 floaty

    m_vao.release();
    m_vbo.release();
    m_program->release();

    qDebug() << "OpenGL Initialized Successfully.";
}

void FloatingEnergySphereWidget::setupShaders()
{
    m_program = new QOpenGLShaderProgram(this);

    // Kompilacja Vertex Shadera
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Vertex shader compilation failed:" << m_program->log();
        return;
    }

    // Kompilacja Fragment Shadera
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Fragment shader compilation failed:" << m_program->log();
        return;
    }

    // Linkowanie programu
    if (!m_program->link()) {
        qCritical() << "Shader program linking failed:" << m_program->log();
        return;
    }
}

void FloatingEnergySphereWidget::setupGeometry()
{
    m_points.clear();
    int numPoints = 15000; // Zwiększona liczba punktów dla gęstszego efektu
    m_pointCount = numPoints;

    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    std::uniform_real_distribution<float> radiusDistribution(0.7f, 1.1f); // Rozkład promienia

    for (int i = 0; i < numPoints; ++i) {
        // Generuj punkty losowo wewnątrz lub na powierzchni kuli
        float x, y, z, d;
        do {
            x = distribution(generator);
            y = distribution(generator);
            z = distribution(generator);
            d = x*x + y*y + z*z;
        } while (d > 1.0f || d < 0.1f); // Odrzuć punkty zbyt daleko lub zbyt blisko środka

        float radius = radiusDistribution(generator); // Losowy promień bazowy
        float scale = radius / std::sqrt(d);

        m_points.push_back(x * scale);
        m_points.push_back(y * scale);
        m_points.push_back(z * scale);
    }
}


void FloatingEnergySphereWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_projectionMatrix.setToIdentity();
    float aspect = float(w) / float(h ? h : 1);
    m_projectionMatrix.perspective(45.0f, aspect, 0.1f, 100.0f);
}

void FloatingEnergySphereWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_program || !m_program->isLinked()) return;

    m_program->bind();
    m_vao.bind();

    m_viewMatrix.setToIdentity();
    m_cameraPosition = QVector3D(0, 0, m_cameraDistance);
    m_viewMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    m_modelMatrix.setToIdentity();
    m_modelMatrix.rotate(m_rotation);

    m_program->setUniformValue("projection", m_projectionMatrix);
    m_program->setUniformValue("view", m_viewMatrix);
    m_program->setUniformValue("model", m_modelMatrix);
    m_program->setUniformValue("time", m_timeValue);
    m_program->setUniformValue("glitchIntensity", m_glitchIntensity); // Przekaż intensywność glitcha

    // Rysowanie punktów
    glDrawArrays(GL_POINTS, 0, m_pointCount);

    m_vao.release();
    m_program->release();
}

void FloatingEnergySphereWidget::updateAnimation()
{
    m_timeValue += 0.016f;
    if (m_timeValue > 1000.0f) m_timeValue = 0.0f;

    // Prosta logika glitcha - losowe impulsy
    static std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    if (distribution(generator) > 0.95f) { // 5% szansy na rozpoczęcie glitcha
        m_glitchIntensity = distribution(generator) * 0.8f + 0.2f; // Losowa intensywność
    } else {
        m_glitchIntensity *= 0.9f; // Stopniowe wygaszanie glitcha
        if (m_glitchIntensity < 0.01f) {
            m_glitchIntensity = 0.0f;
        }
    }

    // Automatyczny, powolny obrót (opcjonalnie)
    m_rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 0.1f) * m_rotation;
    m_rotation.normalize();

    update();
}

void FloatingEnergySphereWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
        event->accept();
    }
    // Usunięto zamykanie prawym przyciskiem
    // else if (event->button() == Qt::RightButton && m_closable) {
    //     close();
    //     event->accept();
    // }
    else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mousePressed && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        // Obracanie na podstawie ruchu myszy
        float angleX = (float)delta.y() * 0.25f; // Obrót wokół osi X
        float angleY = (float)delta.x() * 0.25f; // Obrót wokół osi Y

        QQuaternion rotationX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, angleX);
        QQuaternion rotationY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, angleY);

        m_rotation = rotationY * rotationX * m_rotation; // Kolejność ma znaczenie
        m_rotation.normalize();

        event->accept();
        update(); // Przerenderuj scenę
    } else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
        event->accept();
    } else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::wheelEvent(QWheelEvent *event)
{
    // Zoom za pomocą kółka myszy
    int delta = event->angleDelta().y();
    if (delta > 0) {
        m_cameraDistance *= 0.95f; // Przybliż
    } else if (delta < 0) {
        m_cameraDistance *= 1.05f; // Oddal
    }
    // Ogranicz zoom
    m_cameraDistance = qBound(1.5f, m_cameraDistance, 10.0f);

    event->accept();
    update(); // Przerenderuj scenę
}

void FloatingEnergySphereWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "FloatingEnergySphereWidget closeEvent triggered. Closable:" << m_closable;
    if (m_closable) {
        emit widgetClosed(); // Powiadom menedżera tylko jeśli można zamknąć
        QWidget::closeEvent(event); // Akceptuj zamknięcie
    } else {
        qDebug() << "Closing prevented by m_closable flag.";
        event->ignore(); // Ignoruj próbę zamknięcia
    }
}