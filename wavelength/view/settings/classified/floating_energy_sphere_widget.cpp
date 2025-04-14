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
    layout (location = 0) in vec3 aPos; // Pozycja wejściowa

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time; // Czas do animacji

    out vec3 vPos; // Przekazanie oryginalnej pozycji do fragment shadera
    out float vDeformation; // Przekazanie wartości deformacji

    // Prosta funkcja szumu (można zastąpić bardziej złożoną)
    float noise(vec3 p) {
        // Prosty przykład, można użyć bardziej zaawansowanych funkcji szumu
        return fract(sin(dot(p, vec3(12.9898, 78.233, 54.53))) * 43758.5453);
    }

    void main()
    {
        vPos = aPos; // Zapamiętaj oryginalną pozycję

        // Deformacja oparta na czasie i pozycji
        float frequency = 3.0;
        float amplitude = 0.15;
        float speed = 0.5;

        // Deformacja oparta na sinusie (falowanie)
        float deformation1 = sin(aPos.y * frequency + time * speed) * amplitude;
        // Dodatkowa deformacja dla bardziej organicznego wyglądu
        float deformation2 = cos(aPos.x * frequency * 0.8 + time * speed * 1.2) * amplitude * 0.7;
        // Deformacja oparta na prostym szumie
        float deformation3 = (noise(aPos * 2.0 + time * 0.1) - 0.5) * amplitude * 0.5;

        vDeformation = deformation1 + deformation2 + deformation3; // Suma deformacji

        vec3 normal = normalize(aPos); // Przybliżona normalna dla kuli
        vec3 displacedPos = aPos + normal * vDeformation;

        gl_Position = projection * view * model * vec4(displacedPos, 1.0);

        // Ustawienie rozmiaru punktu (może być dynamiczne)
        gl_PointSize = 2.0 + 3.0 * (sin(time * 2.0 + aPos.x * 5.0) + 1.0) / 2.0; // Dynamiczny rozmiar
        // gl_PointSize = 2.0; // Stały rozmiar
    }
)";

// Fragment Shader: Koloruje punkty na podstawie pozycji i czasu, tworzy gradient
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 vPos; // Oryginalna pozycja z vertex shadera
    in float vDeformation; // Wartość deformacji z vertex shadera
    uniform float time; // Czas do animacji

    void main()
    {
        // Definicja kolorów gradientu
        vec3 colorBlue = vec3(0.1, 0.2, 0.9);
        vec3 colorViolet = vec3(0.6, 0.1, 0.8);
        vec3 colorPink = vec3(0.9, 0.3, 0.7);

        // Współczynnik do interpolacji kolorów (np. na podstawie pozycji Y)
        float factor = (normalize(vPos).y + 1.0) / 2.0; // Normalizowana pozycja Y (zakres 0-1)

        // Interpolacja między kolorami
        vec3 baseColor;
        if (factor < 0.5) {
            baseColor = mix(colorBlue, colorViolet, factor * 2.0);
        } else {
            baseColor = mix(colorViolet, colorPink, (factor - 0.5) * 2.0);
        }

        // Dodanie efektu "blasku" / luminancji - modulacja jasności
        // Użyjemy deformacji i czasu do modulacji
        float glow = 0.6 + 0.4 * abs(sin(vDeformation * 10.0 + time * 1.5)); // Pulsujący blask

        // Końcowy kolor z blaskiem
        vec3 finalColor = baseColor * glow;

        // Można dodać efekt przezroczystości w zależności od kąta patrzenia lub odległości od centrum
        // float alpha = clamp(dot(normalize(vPos), vec3(0,0,1)), 0.1, 1.0); // Przykład alpha
        float alpha = 0.8; // Stała alpha dla lepszego blendingu

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
      // m_ebo usunięte
      m_cameraPosition(0.0f, 0.0f, 3.5f), // Trochę dalej na start
      m_cameraDistance(3.5f),
      m_mousePressed(false),
      m_vertexCount(0), // Zmieniono z m_indexCount
      m_closable(true)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    // WA_TranslucentBackground może być potrzebne dla blendingu punktów z tłem, ale zacznijmy bez
    // setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground); // Upewnij się, że tło systemowe nie jest rysowane
    // setAttribute(Qt::WA_PaintOnScreen);     // <<< ZAKOMENTUJ LUB USUŃ TĘ LINIĘ
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    resize(600, 600); // Zwiększmy trochę rozmiar

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

    // Ustaw kolor czyszczenia na czarny, nieprzezroczysty
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Włącz test głębokości, aby punkty bliżej kamery przesłaniały dalsze
    glEnable(GL_DEPTH_TEST);

    // Włącz blending dla punktów, aby uzyskać efekt półprzezroczystości/blasku
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standardowy blending alpha
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Alternatywnie: Additive blending dla jaśniejszego efektu

    // Włącz możliwość ustawiania rozmiaru punktu w vertex shaderze
    glEnable(GL_PROGRAM_POINT_SIZE);

    qDebug() << "Setting up shaders...";
    setupShaders();
    if (!m_program || !m_program->isLinked()) {
        qCritical() << "Shader program not linked after setupShaders()!";
        if(m_program) qCritical() << "Shader Log:" << m_program->log();
        return;
    }
    qDebug() << "Shaders set up successfully.";

    qDebug() << "Setting up sphere geometry (points)...";
    // Zwiększamy liczbę segmentów dla większej gęstości punktów
    setupSphereGeometry(64, 128); // Więcej punktów (64 pierścienie, 128 sektorów)
    qDebug() << "Sphere geometry set up with" << m_vertexCount << "vertices.";

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    // Alokujemy tylko dane wierzchołków (pozycja x, y, z)
    m_vbo.allocate(m_vertices.data(), m_vertices.size() * sizeof(GLfloat));

    // Konfiguracja EBO jest usunięta

    // Atrybut wierzchołków (tylko pozycja)
    m_program->enableAttributeArray(0);
    // Krok (stride) to 3 * sizeof(GLfloat), bo mamy tylko pozycję (x, y, z)
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(GLfloat));

    m_vao.release();
    m_vbo.release();
    // m_ebo.release() usunięte
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
    // m_indices.clear() usunięte

    const float PI = 3.14159265359f;
    float radius = 1.0f; // Promień bazowej sfery

    float const R = 1. / (float)(rings - 1);
    float const S = 1. / (float)(sectors - 1);

    m_vertices.resize(rings * sectors * 3); // Tylko 3 floaty na wierzchołek (x, y, z)
    auto v = m_vertices.begin();
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            // Współrzędne sferyczne
            float const y = sin(-PI / 2 + PI * r * R);
            float const x = cos(2 * PI * s * S) * sin(PI * r * R);
            float const z = sin(2 * PI * s * S) * sin(PI * r * R);

            // Zapisanie pozycji wierzchołka
            *v++ = x * radius;
            *v++ = y * radius;
            *v++ = z * radius;
        }
    }

    // Nie generujemy już indeksów
    // m_indices.resize(...) usunięte
    // Pętla generująca indeksy usunięta

    m_vertexCount = rings * sectors; // Zapisujemy liczbę wierzchołków
    // m_indexCount usunięte
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
    // Czyszczenie buforów koloru i głębokości na czarno
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_program || !m_program->isLinked()) {
        qWarning() << "PaintGL called but shader program is not linked!";
        return;
    }

    m_program->bind();
    m_vao.bind(); // Powiązanie VAO (automatycznie powiązuje VBO)

    // Ustawienie macierzy widoku (kamera)
    m_viewMatrix.setToIdentity();
    // Pozycja kamery jest teraz kontrolowana przez zoom (m_cameraDistance)
    m_cameraPosition = QVector3D(0, 0, m_cameraDistance);
    m_viewMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    // Ustawienie macierzy modelu (transformacje obiektu - obrót myszką)
    m_modelMatrix.setToIdentity();
    m_modelMatrix.rotate(m_rotation); // Zastosuj obrót myszką

    // Przekazanie macierzy i czasu do shadera
    m_program->setUniformValue("projection", m_projectionMatrix);
    m_program->setUniformValue("view", m_viewMatrix);
    m_program->setUniformValue("model", m_modelMatrix);
    m_program->setUniformValue("time", m_timeValue); // Przekazanie czasu do animacji

    // Rysowanie punktów zamiast trójkątów
    // qDebug() << "Drawing points:" << m_vertexCount; // Opcjonalny log
    glDrawArrays(GL_POINTS, 0, m_vertexCount); // Rysuj punkty

    m_vao.release();
    m_program->release();
}

void FloatingEnergySphereWidget::updateAnimation()
{
    // Aktualizuj czas - napędza animację w shaderach
    m_timeValue += 0.016f; // Zwiększaj czas (przy założeniu ~60 FPS)
    if (m_timeValue > 3600.0f) m_timeValue -= 3600.0f; // Resetuj co jakiś czas, aby uniknąć dużych liczb

    // Automatyczny, powolny obrót (można zakomentować, jeśli preferujesz tylko obrót myszką)
    // m_rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 0.1f) * m_rotation;
    // m_rotation.normalize();

    update(); // Poproś o przerysowanie widgetu
}

// --- Metody obsługi myszy (mousePressEvent, mouseMoveEvent, mouseReleaseEvent, wheelEvent) ---
// Pozostają bez zmian - kontrolują obrót i zoom kamery

void FloatingEnergySphereWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
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

        float angleX = (float)delta.y() * 0.25f;
        float angleY = (float)delta.x() * 0.25f;

        QQuaternion rotationX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, angleX);
        QQuaternion rotationY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, angleY);

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