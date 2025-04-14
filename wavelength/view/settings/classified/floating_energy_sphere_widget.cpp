#include "floating_energy_sphere_widget.h"
#include <QOpenGLShader>
#include <QVector3D>
#include <cmath>
#include <QDebug>
#include <QApplication>
#include <random>

// --- Nowe Shaders ---
// Vertex Shader: Deformuje pozycję wierzchołka w czasie, ustawia rozmiar punktu
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time;

    out vec3 vPos;
    out float vDeformationFactor; // Przekazanie współczynnika deformacji do fragment shadera

    // Prosta funkcja szumu (nadal prosta, ale użyta inaczej)
    float noise(vec3 p) {
        return fract(sin(dot(p, vec3(12.9898, 78.233, 54.53))) * 43758.5453);
    }

    // Funkcja do tworzenia płynniejszych fal
    float smoothWave(vec3 pos, float freq, float amp, float speed, float timeOffset) {
        return sin(pos.y * freq + time * speed + timeOffset) *
               cos(pos.x * freq * 0.8 + time * speed * 0.7 + timeOffset * 0.5) *
               sin(pos.z * freq * 1.2 + time * speed * 1.1 + timeOffset * 1.2) *
               amp;
    }

    void main()
    {
        vPos = aPos;
        vec3 normal = normalize(aPos); // Normalna dla kuli

        // --- Bardziej złożona i płynna deformacja ---
        float baseAmplitude = 0.12; // Zmniejszona amplituda bazowa dla subtelności
        float baseSpeed = 0.3;      // Zmniejszona prędkość bazowa

        // Warstwa 1: Wolne, duże fale
        float deformation1 = smoothWave(aPos, 2.5, baseAmplitude, baseSpeed, 0.0);
        // Warstwa 2: Szybsze, mniejsze fale nałożone na pierwsze
        float deformation2 = smoothWave(aPos * 1.8, 4.0, baseAmplitude * 0.6, baseSpeed * 1.5, 1.5);
        // Warstwa 3: Subtelny szum dla dodania tekstury
        float deformation3 = (noise(aPos * 3.0 + time * 0.05) - 0.5) * baseAmplitude * 0.2;

        // Połączenie deformacji
        float totalDeformation = deformation1 + deformation2 + deformation3;
        vDeformationFactor = smoothstep(0.0, baseAmplitude * 1.5, abs(totalDeformation)); // Normalizowany współczynnik dla koloru/blasku

        vec3 displacedPos = aPos + normal * totalDeformation;

        gl_Position = projection * view * model * vec4(displacedPos, 1.0);

        // --- Płynniejsza zmiana rozmiaru punktu ---
        float basePointSize = 1.8;
        float sizeVariation = 1.5;
        // Użycie sinusa z różnymi częstotliwościami dla płynniejszej zmiany
        gl_PointSize = basePointSize + sizeVariation * (sin(time * 1.0 + aPos.x * 3.0) + cos(time * 0.7 + aPos.y * 4.0) + 1.0) / 2.0;
        // Ograniczenie minimalnego rozmiaru
        gl_PointSize = max(1.0, gl_PointSize);
    }
)";

// Fragment Shader: Okrągłe punkty, wykorzystanie vDeformationFactor
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 vPos;
    in float vDeformationFactor; // Współczynnik deformacji (0 do ~1)
    uniform float time;

    void main()
    {
        /* --- Tymczasowo wyłączone okrągłe punkty ---
        vec2 coord = gl_PointCoord - vec2(0.5); // Współrzędne wewnątrz punktu (-0.5 do 0.5)
        if(dot(coord, coord) > 0.25) { // Jeśli poza okręgiem o promieniu 0.5
            discard; // Odrzuć fragment
        }
        float edgeFactor = 1.0 - smoothstep(0.20, 0.25, dot(coord, coord)); // Wygładzenie krawędzi
        */ // --- Koniec tymczasowego wyłączenia ---

        vec3 colorBlue = vec3(0.1, 0.3, 1.0); // Nieco jaśniejszy niebieski
        vec3 colorViolet = vec3(0.7, 0.2, 0.9);
        vec3 colorPink = vec3(1.0, 0.4, 0.8); // Nieco jaśniejszy róż

        float factor = (normalize(vPos).y + 1.0) / 2.0;

        vec3 baseColor;
        if (factor < 0.5) {
            baseColor = mix(colorBlue, colorViolet, factor * 2.0);
        } else {
            baseColor = mix(colorViolet, colorPink, (factor - 0.5) * 2.0);
        }

        // Blask modulowany przez vDeformationFactor i czas dla subtelniejszego efektu
        float glow = 0.5 + 0.5 * vDeformationFactor + 0.2 * abs(sin(time * 1.2)); // Mniej intensywny, zależny od deformacji

        vec3 finalColor = baseColor * (glow + 0.3); // Dodanie lekkiej jasności bazowej

        // Użyj stałej alpha lub edgeFactor jeśli przywrócisz okrągłe punkty
        float alpha = 0.8; // * edgeFactor; // Użyj stałej alpha na razie

        FragColor = vec4(finalColor, alpha);
    }
)";
// --- Koniec Shaders ---


FloatingEnergySphereWidget::FloatingEnergySphereWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      QOpenGLFunctions_3_3_Core(),
      m_timeValue(0.0f),
      m_program(nullptr),
      m_vbo(QOpenGLBuffer::VertexBuffer),
      m_cameraPosition(0.0f, 0.0f, 3.5f),
      m_cameraDistance(3.5f),
      m_mousePressed(false),
      m_vertexCount(0),
      m_closable(true),
      m_angularVelocity(0.0f, 0.0f, 0.0f), // Inicjalizacja prędkości kątowej
      m_dampingFactor(0.96f)              // Współczynnik tłumienia (bliżej 1 = dłuższa bezwładność)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_NoSystemBackground);
    // setAttribute(Qt::WA_PaintOnScreen); // Upewnij się, że to jest zakomentowane/usunięte
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true); // Ważne dla płynnego ruchu myszy
    resize(600, 600);

    connect(&m_timer, &QTimer::timeout, this, &FloatingEnergySphereWidget::updateAnimation);
    m_timer.start(16); // ~60 FPS
}

FloatingEnergySphereWidget::~FloatingEnergySphereWidget()
{
    makeCurrent();
    m_vbo.destroy();
    // m_ebo.destroy() usunięte
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
    qDebug() << "FloatingEnergySphereWidget::initializeGL() started.";
    if (!initializeOpenGLFunctions()) {
        qCritical() << "Failed to initialize OpenGL functions!";
        return;
    }
    qDebug() << "OpenGL functions initialized.";

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // --- Włącz wygładzanie punktów ---
    // glEnable(GL_POINT_SMOOTH); // Może, ale nie musi działać dobrze na wszystkich sterownikach
    // --- Koniec wygładzania punktów ---

    qDebug() << "Setting up shaders...";
    setupShaders();
    if (!m_program || !m_program->isLinked()) {
        qCritical() << "Shader program not linked after setupShaders()!";
        if(m_program) qCritical() << "Shader Log:" << m_program->log();
        return;
    }
    qDebug() << "Shaders set up successfully.";

    qDebug() << "Setting up sphere geometry (points)...";
    setupSphereGeometry(64, 128);
    qDebug() << "Sphere geometry set up with" << m_vertexCount << "vertices.";

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(m_vertices.data(), m_vertices.size() * sizeof(GLfloat));

    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(GLfloat));

    m_vao.release();
    m_vbo.release();
    m_program->release();

    qDebug() << "VAO/VBO configured for points.";
    qDebug() << "FloatingEnergySphereWidget::initializeGL() finished successfully.";
}

void FloatingEnergySphereWidget::setupShaders()
{
    // Ta funkcja pozostaje bez zmian, kompiluje i linkuje nowe shadery
    m_program = new QOpenGLShaderProgram(this);

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Vertex shader compilation failed:" << m_program->log();
        delete m_program; m_program = nullptr; // Dodaj sprzątanie w razie błędu
        return;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Fragment shader compilation failed:" << m_program->log();
        delete m_program; m_program = nullptr; // Dodaj sprzątanie w razie błędu
        return;
    }

    if (!m_program->link()) {
        qCritical() << "Shader program linking failed:" << m_program->log();
        delete m_program; m_program = nullptr; // Dodaj sprzątanie w razie błędu
        return;
    }
}

void FloatingEnergySphereWidget::setupSphereGeometry(int rings, int sectors)
{
    m_vertices.clear();

    const float PI = 3.14159265359f;
    float radius = 1.0f;

    float const R = 1. / (float)(rings - 1);
    float const S = 1. / (float)(sectors - 1);

    m_vertices.resize(rings * sectors * 3);
    auto v = m_vertices.begin();
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            float const y = sin(-PI / 2 + PI * r * R);
            float const x = cos(2 * PI * s * S) * sin(PI * r * R);
            float const z = sin(2 * PI * s * S) * sin(PI * r * R);

            *v++ = x * radius;
            *v++ = y * radius;
            *v++ = z * radius;
        }
    }
    m_vertexCount = rings * sectors;
}


void FloatingEnergySphereWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_projectionMatrix.setToIdentity();
    float aspect = float(w) / float(h ? h : 1);
    // Zmniejszamy nieco kąt widzenia dla lepszego efektu "wypełnienia"
    m_projectionMatrix.perspective(40.0f, aspect, 0.1f, 100.0f);
}

void FloatingEnergySphereWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_program || !m_program->isLinked()) {
        return;
    }

    m_program->bind();
    m_vao.bind();

    m_viewMatrix.setToIdentity();
    m_cameraPosition = QVector3D(0, 0, m_cameraDistance);
    m_viewMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    m_modelMatrix.setToIdentity();
    m_modelMatrix.rotate(m_rotation); // Zastosuj aktualny obrót (z bezwładnością)

    m_program->setUniformValue("projection", m_projectionMatrix);
    m_program->setUniformValue("view", m_viewMatrix);
    m_program->setUniformValue("model", m_modelMatrix);
    m_program->setUniformValue("time", m_timeValue);

    glDrawArrays(GL_POINTS, 0, m_vertexCount);

    m_vao.release();
    m_program->release();
}

void FloatingEnergySphereWidget::updateAnimation()
{
    m_timeValue += 0.016f;
    if (m_timeValue > 3600.0f) m_timeValue -= 3600.0f;

    // --- Zastosowanie bezwładności ---
    if (!m_mousePressed && m_angularVelocity.lengthSquared() > 0.0001f) {
        // Oblicz obrót na podstawie prędkości kątowej
        QQuaternion deltaRotation = QQuaternion::fromAxisAndAngle(m_angularVelocity.normalized(), m_angularVelocity.length());
        m_rotation = deltaRotation * m_rotation;
        m_rotation.normalize(); // Normalizuj kwaternion

        // Zastosuj tłumienie
        m_angularVelocity *= m_dampingFactor;

        // Zatrzymaj, jeśli prędkość jest bardzo mała
        if (m_angularVelocity.lengthSquared() < 0.0001f) {
            m_angularVelocity = QVector3D(0.0f, 0.0f, 0.0f);
        }
        update(); // Poproś o przerysowanie tylko jeśli jest ruch bezwładnościowy
    } else if (!m_mousePressed) {
        // Jeśli nie ma wciśniętej myszy i nie ma prędkości, nie rób nic (chyba że chcesz stały obrót)
        // Można tu dodać powolny, stały obrót, jeśli jest pożądany:
        QQuaternion slowSpin = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 0.05f);
        m_rotation = slowSpin * m_rotation;
        m_rotation.normalize();
        update();
    } else {
        // Jeśli mysz jest wciśnięta, update() jest wywoływane w mouseMoveEvent
    }

    // Jeśli nie ma bezwładności ani stałego obrotu, update() jest potrzebne tylko dla animacji shadera
    if (m_mousePressed || m_angularVelocity.lengthSquared() > 0.0001f) {
        // Już wywołano update() wyżej lub w mouseMoveEvent
    } else {
        // Wywołaj update() dla samej animacji shadera, jeśli nie ma ruchu obiektu
        update();
    }
    // --- Koniec bezwładności ---
}

// --- Metody obsługi myszy (mousePressEvent, mouseMoveEvent, mouseReleaseEvent, wheelEvent) ---
// Pozostają bez zmian - kontrolują obrót i zoom kamery

void FloatingEnergySphereWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
        m_angularVelocity = QVector3D(0.0f, 0.0f, 0.0f); // Zatrzymaj bezwładność przy nowym chwyceniu
        event->accept();
    } else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mousePressed && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        // --- Obliczanie prędkości kątowej zamiast bezpośredniego obrotu ---
        float sensitivity = 0.4f; // Dostosuj czułość
        // Odwracamy osie, aby pasowały do intuicyjnego przeciągania obiektu
        float angularSpeedX = (float)-delta.y() * sensitivity;
        float angularSpeedY = (float)-delta.x() * sensitivity;

        // Tworzymy wektor prędkości w lokalnych koordynatach kamery (oś X i Y)
        QVector3D localAngularVelocity = QVector3D(angularSpeedX, angularSpeedY, 0.0f);

        // Transformujemy prędkość do koordynatów świata używając odwrotności rotacji obiektu
        // (lub prościej, zakładamy, że oś Y świata to góra/dół, a X to lewo/prawo na ekranie)
        // Prostsze podejście: bezpośrednie zastosowanie prędkości do osi świata
        m_angularVelocity = QVector3D(angularSpeedX, angularSpeedY, 0.0f);

        // Zastosuj natychmiastowy obrót podczas przeciągania (opcjonalne, ale daje lepsze poczucie kontroli)
        QQuaternion rotationX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, angularSpeedX * 0.1f); // Mniejszy mnożnik dla płynności
        QQuaternion rotationY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, angularSpeedY * 0.1f);
        m_rotation = rotationY * rotationX * m_rotation;
        m_rotation.normalize();
        // --- Koniec obliczania prędkości ---

        event->accept();
        update(); // Poproś o przerysowanie
    } else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
        // Prędkość kątowa została ustawiona w ostatnim mouseMoveEvent,
        // teraz updateAnimation() zajmie się jej wygaszaniem.
        event->accept();
    } else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if (delta > 0) {
        m_cameraDistance *= 0.95f;
    } else if (delta < 0) {
        m_cameraDistance *= 1.05f;
    }
    m_cameraDistance = qBound(1.5f, m_cameraDistance, 15.0f); // Zwiększony max zoom out

    event->accept();
    update();
}


// --- Metoda closeEvent ---
// Pozostaje bez zmian

void FloatingEnergySphereWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "FloatingEnergySphereWidget closeEvent triggered. Closable:" << m_closable;
    if (m_closable) {
        emit widgetClosed();
        QWidget::closeEvent(event);
    } else {
        qDebug() << "Closing prevented by m_closable flag.";
        event->ignore();
    }
}