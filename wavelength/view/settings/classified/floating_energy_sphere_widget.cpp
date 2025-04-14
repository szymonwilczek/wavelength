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
    out float vDeformationFactor;

    float noise(vec3 p) {
        return fract(sin(dot(p, vec3(12.9898, 78.233, 54.53))) * 43758.5453);
    }

    float evolvingWave(vec3 pos, float baseFreq, float freqVariation, float amp, float speed, float timeOffset) {
        float freqTimeFactor = 0.23; // Lekko wolniej
        float offsetTimeFactor = 0.16; // Lekko wolniej
        float currentFreq = baseFreq + sin(time * freqTimeFactor + timeOffset * 2.0) * freqVariation;
        float currentOffset = timeOffset + cos(time * offsetTimeFactor + pos.x * 0.1) * 2.0;
        return sin(pos.y * currentFreq + time * speed + currentOffset) *
               cos(pos.x * currentFreq * 0.8 + time * speed * 0.7 + currentOffset * 0.5) *
               sin(pos.z * currentFreq * 1.2 + time * speed * 1.1 + currentOffset * 1.2) *
               amp;
    }

    void main()
    {
        vPos = aPos;
        vec3 normal = normalize(aPos);

        // --- Lekko zmniejszona amplituda/prędkość ---
        float baseAmplitude = 0.26; // <<< Zmniejszono (było 0.28)
        float baseSpeed = 0.33;     // <<< Zmniejszono (było 0.35)

        float deformation1 = evolvingWave(aPos, 2.0, 0.55, baseAmplitude, baseSpeed * 0.8, 0.0); // Mniejsza freqVariation
        float deformation2 = evolvingWave(aPos * 1.5, 3.5, 1.1, baseAmplitude * 0.7, baseSpeed * 1.5, 1.5); // Mniejsza freqVariation
        float thinkingAmplitudeFactor = 0.85; // <<< Zmniejszono (było 0.9)
        float thinkingDeformation = evolvingWave(aPos * 0.5, 1.0, 0.35, baseAmplitude * thinkingAmplitudeFactor, baseSpeed * 0.15, 5.0); // Mniejsza freqVariation
        float noiseDeformation = (noise(aPos * 2.5 + time * 0.04) - 0.5) * baseAmplitude * 0.15;

        float totalDeformation = deformation1 + deformation2 + thinkingDeformation + noiseDeformation;
        vDeformationFactor = smoothstep(0.0, baseAmplitude * 2.0, abs(totalDeformation)); // Dostosowany zakres

        vec3 displacedPos = aPos + normal * totalDeformation;

        gl_Position = projection * view * model * vec4(displacedPos, 1.0);

        // --- Jeszcze płynniejsza zmiana rozmiaru punktu ---
        float basePointSize = 0.9; // Utrzymujemy mały rozmiar
        float sizeVariation = 0.5; // <<< Znacznie zmniejszono wariację (było 0.7)
        float smoothTimeFactor = 0.3; // <<< Spowolniono zmianę w czasie (było 0.5)
        // Rozmiar zależy teraz głównie od bazowego rozmiaru i lekko od deformacji/czasu
        gl_PointSize = basePointSize + sizeVariation * vDeformationFactor * (1.0 + 0.3 * sin(time * smoothTimeFactor + aPos.y * 0.5));
        gl_PointSize = max(0.6, gl_PointSize); // Utrzymujemy mały minimalny rozmiar
    }
)";

// Fragment Shader: Zmniejszona bazowa alpha
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 vPos;
    in float vDeformationFactor;
    uniform float time;

    void main()
    {
        vec3 colorBlue = vec3(0.1, 0.3, 1.0);
        vec3 colorViolet = vec3(0.7, 0.2, 0.9);
        vec3 colorPink = vec3(1.0, 0.4, 0.8);

        float factor = (normalize(vPos).y + 1.0) / 2.0;

        vec3 baseColor;
        if (factor < 0.5) {
            baseColor = mix(colorBlue, colorViolet, factor * 2.0);
        } else {
            baseColor = mix(colorViolet, colorPink, (factor - 0.5) * 2.0);
        }

        float glow = 0.4 + 0.8 * vDeformationFactor + 0.3 * abs(sin(vPos.x * 2.0 + time * 1.5));
        vec3 finalColor = baseColor * (glow + 0.2);

        // --- Zmniejszona bazowa alpha ---
        float baseAlpha = 0.75; // <<< Zmniejszono (było 0.85)
        float finalAlpha = baseAlpha;
        FragColor = vec4(finalColor, finalAlpha);
    }
)";
// --- Koniec Shaders ---


FloatingEnergySphereWidget::FloatingEnergySphereWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      QOpenGLFunctions_3_3_Core(),
      m_timeValue(0.0f),
      m_lastFrameTimeSecs(0.0f),
      m_program(nullptr),
      m_vbo(QOpenGLBuffer::VertexBuffer),
      m_cameraPosition(0.0f, 0.0f, 3.5f),
      m_cameraDistance(3.5f),
      m_mousePressed(false),
      m_vertexCount(0),
      m_closable(true),
      m_angularVelocity(0.0f, 0.0f, 0.0f),
      m_dampingFactor(0.96f)
{
    // --- Ustawienia okna dla przezroczystości ---
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground); // <<< DODAJ TEN ATRYBUT
    setAttribute(Qt::WA_NoSystemBackground);
    // setAttribute(Qt::WA_PaintOnScreen); // Upewnij się, że jest wyłączone

    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    resize(600, 600);

    // --- Uruchomienie timera ---
    m_elapsedTimer.start();
    m_lastFrameTimeSecs = m_elapsedTimer.elapsed() / 1000.0f;

    connect(&m_timer, &QTimer::timeout, this, &FloatingEnergySphereWidget::updateAnimation);
    m_timer.start(16);

    // --- Opcjonalnie: Ustaw format powierzchni OpenGL z kanałem alfa ---
    // Chociaż często działa domyślnie, można to jawnie ustawić
    QSurfaceFormat fmt = format(); // Pobierz bieżący format
    fmt.setAlphaBufferSize(8);     // Poproś o 8-bitowy bufor alfa
    setFormat(fmt);                // Zastosuj nowy format
    // --- Koniec opcjonalnego ustawienia formatu ---
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

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
    setupSphereGeometry(128, 256);
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
    // --- Obliczanie Delta Time ---
    float currentTimeSecs = m_elapsedTimer.elapsed() / 1000.0f;
    float deltaTime = currentTimeSecs - m_lastFrameTimeSecs;
    m_lastFrameTimeSecs = currentTimeSecs;

    // Ograniczenie deltaTime, aby uniknąć "przeskoków" przy dużych opóźnieniach
    // (np. podczas debugowania lub gdy okno było nieaktywne)
    deltaTime = qMin(deltaTime, 0.05f); // Max 50ms (odpowiednik 20 FPS)

    // --- Aktualizacja czasu dla shadera ---
    // Zamiast m_timeValue += 0.016f;
    m_timeValue += deltaTime;
    // Resetowanie m_timeValue nie jest już tak krytyczne, ale nadal dobre dla uniknięcia dużych liczb
    if (m_timeValue > 3600.0f) m_timeValue -= 3600.0f;

    // --- Zastosowanie bezwładności (używa deltaTime) ---
    if (!m_mousePressed && m_angularVelocity.lengthSquared() > 0.0001f) {
        // Prędkość kątowa jest w "stopniach na klatkę" (z mouseMoveEvent),
        // skalujemy ją przez deltaTime, aby była niezależna od FPS.
        // Potrzebujemy współczynnika skalującego, np. 60.0, jeśli prędkość była dostosowana do 60 FPS.
        // LUB: zmodyfikuj mouseMoveEvent, aby prędkość była w stopniach na sekundę.
        // Podejście 1: Skalowanie istniejącej prędkości
        float rotationAngle = m_angularVelocity.length() * deltaTime * 60.0f; // Skalowanie do sekund
        QQuaternion deltaRotation = QQuaternion::fromAxisAndAngle(m_angularVelocity.normalized(), rotationAngle);

        m_rotation = deltaRotation * m_rotation;
        m_rotation.normalize();

        // Tłumienie powinno być również zależne od czasu, aby było spójne
        // m_angularVelocity *= m_dampingFactor; // Tłumienie na klatkę
        m_angularVelocity *= pow(m_dampingFactor, deltaTime * 60.0f); // Tłumienie na sekundę (przybliżone)


        if (m_angularVelocity.lengthSquared() < 0.0001f) {
            m_angularVelocity = QVector3D(0.0f, 0.0f, 0.0f);
        }
        update();
    } else if (!m_mousePressed) {
        // Opcjonalny stały obrót (jeśli jest) też powinien używać deltaTime
        QQuaternion slowSpin = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 0.05f * deltaTime * 60.0f); // Prędkość w stopniach/sek
        m_rotation = slowSpin * m_rotation;
        m_rotation.normalize();
        update();
    }

    // Zawsze wywołuj update() dla animacji shadera, nawet jeśli obiekt się nie obraca
    if (!m_mousePressed && m_angularVelocity.lengthSquared() <= 0.0001f) {
         update();
    }
    // Jeśli mysz jest wciśnięta, update() jest wywoływane w mouseMoveEvent
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

        // --- Obliczanie prędkości kątowej w stopniach na sekundę (przybliżone) ---
        // Zakładamy, że zdarzenie ruchu myszy odpowiada mniej więcej jednej klatce
        // Można by użyć deltaTime z updateAnimation, ale to skomplikowane.
        // Prostsze: dostosuj czułość, aby uzyskać pożądaną prędkość.
        float sensitivity = 25.0f; // Zwiększona czułość, bo teraz reprezentuje stopnie/sek (w przybliżeniu)
        float angularSpeedX = (float)-delta.y() * sensitivity;
        float angularSpeedY = (float)-delta.x() * sensitivity;

        // Ustawiamy prędkość kątową (teraz w przybliżeniu stopnie/sek)
        // W updateAnimation będzie ona skalowana przez deltaTime
        m_angularVelocity = QVector3D(angularSpeedX, angularSpeedY, 0.0f);

        // Natychmiastowy obrót podczas przeciągania (używamy małego kroku, niezależnego od deltaTime)
        float immediateRotationFactor = 0.005f; // Mały stały obrót dla responsywności
        QQuaternion rotationX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, (float)-delta.y() * immediateRotationFactor);
        QQuaternion rotationY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, (float)-delta.x() * immediateRotationFactor);
        m_rotation = rotationY * rotationX * m_rotation;
        m_rotation.normalize();

        event->accept();
        update();
    } else {
        event->ignore();
    }
}


void FloatingEnergySphereWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
        // Prędkość kątowa (w stopniach/sek) została ustawiona w mouseMoveEvent
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
    m_cameraDistance = qBound(1.5f, m_cameraDistance, 15.0f);

    event->accept();
    update();
}

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