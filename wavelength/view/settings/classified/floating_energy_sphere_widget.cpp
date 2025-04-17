#include "floating_energy_sphere_widget.h"
#include <QOpenGLShader>
#include <QVector3D>
#include <cmath>
#include <QDebug>
#include <QApplication>
#include <random>
#include <QMediaContent> // <<< Dodano
#include <QUrl> // <<< Dodano
#include <limits> // <<< Dodano dla std::numeric_limits
#include <QDir>
#include <qmath.h>

// --- Zaktualizowane Shaders ---
// Vertex Shader: Dodano uniform 'audioAmplitude' i uwzględniono go w deformacji
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time;
    uniform float audioAmplitude;

    // --- Uniformy dla uderzeń ---
    #define MAX_IMPACTS 5
    uniform vec3 u_impactPoints[MAX_IMPACTS];
    uniform float u_impactStartTimes[MAX_IMPACTS];
    // Nie potrzebujemy activeImpactCount, jeśli sprawdzamy startTime > 0

    out vec3 vPos;
    out float vDeformationFactor; // Będzie teraz uwzględniać uderzenia

    float noise(vec3 p) {
        return fract(sin(dot(p, vec3(12.9898, 78.233, 54.53))) * 43758.5453);
    }

    float evolvingWave(vec3 pos, float baseFreq, float freqVariation, float amp, float speed, float timeOffset) {
        float freqTimeFactor = 0.23;
        float offsetTimeFactor = 0.16;
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

        // --- Deformacje Idle i Audio (bez zmian) ---
        float baseAmplitude = 0.26;
        float baseSpeed = 0.33;
        float deformation1 = evolvingWave(aPos, 2.0, 0.55, baseAmplitude, baseSpeed * 0.8, 0.0);
        float deformation2 = evolvingWave(aPos * 1.5, 3.5, 1.1, baseAmplitude * 0.7, baseSpeed * 1.5, 1.5);
        float thinkingAmplitudeFactor = 0.85;
        float thinkingDeformation = evolvingWave(aPos * 0.5, 1.0, 0.35, baseAmplitude * thinkingAmplitudeFactor, baseSpeed * 0.15, 5.0);
        float noiseDeformation = (noise(aPos * 2.5 + time * 0.04) - 0.5) * baseAmplitude * 0.15;
        float totalIdleDeformation = deformation1 + deformation2 + thinkingDeformation + noiseDeformation;
        float audioDeformationScale = 1.8;
        float audioEffect = audioAmplitude * audioDeformationScale;

        // --- Obliczanie deformacji od uderzeń ---
        float totalImpactDeformation = 0.0;
        float impactDuration = 2.8; // Czas trwania efektu w sekundach
        float waveSpeed = 3.5;      // Prędkość rozchodzenia się fali
        float maxDepth = -0.18;     // Maksymalna głębokość wgniecenia (ujemna)
        float waveLength = 0.7;     // Długość fali (wpływa na częstotliwość przestrzenną)
        float PI = 3.14159265;

        for (int i = 0; i < MAX_IMPACTS; ++i) {
            // Sprawdź, czy uderzenie jest aktywne (czas startu > 0)
            if (u_impactStartTimes[i] > 0.0) {
                float timeSinceImpact = time - u_impactStartTimes[i];

                // Sprawdź, czy efekt uderzenia jest jeszcze aktywny czasowo
                if (timeSinceImpact > 0.0 && timeSinceImpact < impactDuration) {
                    // Oblicz odległość kątową na powierzchni sfery
                    vec3 impactPointNorm = normalize(u_impactPoints[i]); // Upewnij się, że jest znormalizowany
                    float dotProduct = dot(normal, impactPointNorm);
                    dotProduct = clamp(dotProduct, -1.0, 1.0); // Unikaj błędów numerycznych
                    float distance = acos(dotProduct); // Odległość kątowa w radianach

                    // Oblicz, jak daleko dotarł front fali
                    float waveFrontDistance = timeSinceImpact * waveSpeed;

                    // Sprawdź, czy wierzchołek jest w zasięgu fali
                    if (distance < waveFrontDistance + waveLength * 0.5) { // Trochę zapasu dla gładkości
                        // Funkcja fali (np. tłumiony cosinus)
                        // Chcemy, aby fala zaczynała się od wgniecenia i rozchodziła
                        float phase = (distance - waveFrontDistance) / waveLength * 2.0 * PI;
                        float ripple = cos(phase);

                        // Obwiednia czasowa (szybkie narastanie, wolniejsze zanikanie)
                        float timeEnvelope = smoothstep(0.0, 0.2, timeSinceImpact) * smoothstep(impactDuration, impactDuration * 0.6, timeSinceImpact);

                        // Obwiednia odległościowa (efekt słabnie dalej od frontu fali)
                        // float distanceEnvelope = smoothstep(waveFrontDistance + waveLength * 0.5, waveFrontDistance - waveLength * 0.5, distance);
                        // Prostsza obwiednia: efekt najsilniejszy przy froncie
                         float distanceEnvelope = 1.0 - smoothstep(0.0, waveLength * 1.5, abs(distance - waveFrontDistance));


                        // Połącz wszystko
                        float impactDeform = ripple * timeEnvelope * distanceEnvelope * maxDepth;

                        totalImpactDeformation += impactDeform;
                    }
                }
                 // Opcjonalnie: Oznacz jako nieaktywne po czasie trwania (można też w C++)
                 // if (timeSinceImpact >= impactDuration) {
                 //     // Jak przekazać informację zwrotną do C++? Trudne. Lepiej obsłużyć w C++.
                 // }
            }
        }

        // --- Połącz wszystkie deformacje ---
        vec3 displacedPos = aPos + normal * (totalIdleDeformation + audioEffect + totalImpactDeformation);

        // --- Zaktualizuj vDeformationFactor ---
        float maxExpectedDeformation = baseAmplitude * 2.0 + audioDeformationScale;
        // Uwzględnij maksymalną głębokość uderzenia w oczekiwanej deformacji
        vDeformationFactor = smoothstep(0.0, max(0.1, maxExpectedDeformation + abs(maxDepth)), abs(totalIdleDeformation + audioEffect + totalImpactDeformation));

        gl_Position = projection * view * model * vec4(displacedPos, 1.0);

        // --- Rozmiar punktu (bez zmian) ---
        float basePointSize = 0.9;
        float sizeVariation = 0.5;
        float smoothTimeFactor = 0.3;
        float idlePointSizeVariation = sizeVariation * vDeformationFactor * (0.3 * sin(time * smoothTimeFactor + aPos.y * 0.5));
        float audioPointSizeVariation = audioAmplitude * 1.5;
        gl_PointSize = basePointSize + idlePointSizeVariation + audioPointSizeVariation;
        gl_PointSize = max(0.6, gl_PointSize);
    }
)";

// Fragment Shader: Bez zmian
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

        float baseAlpha = 0.75;
        float finalAlpha = baseAlpha;
        FragColor = vec4(finalColor, finalAlpha);
    }
)";
// --- Koniec Shaders ---


FloatingEnergySphereWidget::FloatingEnergySphereWidget(bool isFirstTime, QWidget *parent)
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
      m_dampingFactor(0.96f),
      m_player(nullptr), // <<< Inicjalizacja
      m_decoder(nullptr), // <<< Inicjalizacja
      m_audioBufferDurationMs(0), // <<< Inicjalizacja
      m_currentAudioAmplitude(0.0f), // <<< Inicjalizacja
      m_audioReady(false), // <<< Inicjalizacja
      m_isFirstTime(isFirstTime),
      m_hintTimer(new QTimer(this))
{
    m_impacts.resize(MAX_IMPACTS); // Utwórz miejsce dla maksymalnej liczby uderzeń
    for(auto& impact : m_impacts) {
        impact.active = false;
        impact.startTime = -1.0f; // Oznacz jako nieaktywne
    }

    m_player = new QMediaPlayer(this);
    m_decoder = new QAudioDecoder(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    resize(600, 600);

    m_elapsedTimer.start();
    m_lastFrameTimeSecs = m_elapsedTimer.elapsed() / 1000.0f;

    connect(&m_timer, &QTimer::timeout, this, &FloatingEnergySphereWidget::updateAnimation);
    m_timer.start(16);

    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    setFormat(fmt);

    connect(&m_timer, &QTimer::timeout, this, &FloatingEnergySphereWidget::updateAnimation);
    connect(m_hintTimer, &QTimer::timeout, this, &FloatingEnergySphereWidget::playKonamiHint);
    connect(m_player, &QMediaPlayer::stateChanged, this, &FloatingEnergySphereWidget::handlePlayerStateChanged);

    m_timer.start(16);
}

FloatingEnergySphereWidget::~FloatingEnergySphereWidget()
{
    makeCurrent();
    m_vbo.destroy();
    m_vao.destroy();
    delete m_program;
    doneCurrent();

    if (m_hintTimer && m_hintTimer->isActive()) {
        m_hintTimer->stop();
    }

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

    // --- Konfiguracja Audio ---
    setupAudio(); // <<< Wywołanie konfiguracji audio

    qDebug() << "FloatingEnergySphereWidget::initializeGL() finished successfully.";
}

// <<< Nowa funkcja do konfiguracji audio >>>
void FloatingEnergySphereWidget::setupAudio()
{
    qDebug() << "Setting up audio. Is first time:" << m_isFirstTime;

    if (!m_player || !m_decoder) {
        qCritical() << "Audio objects (player/decoder) are null!";
        return;
    }

    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &FloatingEnergySphereWidget::handleMediaPlayerError);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &FloatingEnergySphereWidget::handleAudioDecoderError);
    connect(m_decoder, &QAudioDecoder::bufferReady, this, &FloatingEnergySphereWidget::processAudioBuffer);
    connect(m_decoder, &QAudioDecoder::finished, this, &FloatingEnergySphereWidget::audioDecodingFinished);

    QString appDirPath = QCoreApplication::applicationDirPath();
    QString audioSubDir = "/assets/audio/";

    // <<< Wybór nazwy pliku na podstawie flagi >>>
    QString audioFileName;
    if (m_isFirstTime) {
        audioFileName = "JOI.wav"; // Plik dla pierwszego uruchomienia
        qDebug() << "Playing initial audio file:" << audioFileName;
    } else {
        audioFileName = "hello_again.wav"; // Plik dla kolejnych uruchomień
        qDebug() << "Playing subsequent audio file:" << audioFileName;
    }
    // <<< Koniec wyboru nazwy pliku >>>

    QString chosenFilePath = QDir::cleanPath(appDirPath + audioSubDir + audioFileName);
    QUrl chosenFileUrl;

    if (QFile::exists(chosenFilePath)) {
        chosenFileUrl = QUrl::fromLocalFile(chosenFilePath);
        qDebug() << "Using audio file:" << chosenFilePath;
    } else {
        // Spróbuj alternatywnego pliku (np. MP3), jeśli główny nie istnieje
        // LUB zgłoś krytyczny błąd, jeśli wybrany plik jest wymagany
        QString fallbackFileName = m_isFirstTime ? "JOImp3.mp3" : ""; // Można dodać fallback dla hello_again
        if (!fallbackFileName.isEmpty()) {
             QString fallbackFilePath = QDir::cleanPath(appDirPath + audioSubDir + fallbackFileName);
             if (QFile::exists(fallbackFilePath)) {
                 chosenFilePath = fallbackFilePath;
                 chosenFileUrl = QUrl::fromLocalFile(chosenFilePath);
                 qWarning() << "Primary audio file not found, using fallback:" << chosenFilePath;
             } else {
                 qCritical() << "Chosen audio file AND fallback not found:" << chosenFilePath << "and" << fallbackFilePath;
                 return;
             }
        } else {
             qCritical() << "Chosen audio file not found:" << chosenFilePath;
             qCritical() << "Ensure the 'assets/audio' folder with '" << audioFileName << "' is copied next to the executable.";
             return;
        }
    }

    m_decoder->setSourceFilename(chosenFilePath);
    m_decoder->start();
    qDebug() << "Audio decoder started for:" << chosenFilePath;

    m_player->setMedia(chosenFileUrl);
    m_player->setVolume(75);
    m_player->play();
    qDebug() << "Audio player started for:" << chosenFileUrl.toString();

    if (m_player->state() == QMediaPlayer::StoppedState && m_player->error() != QMediaPlayer::NoError) {
         qWarning() << "Player entered StoppedState immediately after play() call. Error:" << m_player->errorString();
    }
}


void FloatingEnergySphereWidget::setupShaders()
{
    m_program = new QOpenGLShaderProgram(this);

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Vertex shader compilation failed:" << m_program->log();
        delete m_program; m_program = nullptr;
        return;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Fragment shader compilation failed:" << m_program->log();
        delete m_program; m_program = nullptr;
        return;
    }

    if (!m_program->link()) {
        qCritical() << "Shader program linking failed:" << m_program->log();
        delete m_program; m_program = nullptr;
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
    if (h == 0) h = 1; // Zapobieganie dzieleniu przez zero

    // Ustawienie viewportu OpenGL na cały widget
    glViewport(0, 0, w, h);

    // --- Dynamiczne obliczanie FOV dla stałego rozmiaru sfery ---
    float aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    float sphereRadius = 1.0f; // Promień sfery w przestrzeni modelu
    // Współczynnik określający, jaką część mniejszego wymiaru widgetu ma zajmować sfera
    // Np. 1.1 oznacza, że sfera zajmie ok. 91% mniejszego wymiaru, dając mały margines.
    // Zwiększ tę wartość, aby sfera była mniejsza (większy margines).
    // Zmniejsz (bliżej 1.0), aby była większa (mniejszy margines).
    float paddingFactor = 2.0f;

    // Oblicz tangens połowy wymaganego kąta widzenia, aby zmieścić sferę z marginesem
    // Musimy zapewnić, że zmieści się zarówno w pionie, jak i w poziomie.
    // Wybieramy większy z wymaganych kątów (co odpowiada mniejszemu wymiarowi).
    float requiredTanHalfFovVertical = (sphereRadius * paddingFactor) / m_cameraDistance;
    float requiredTanHalfFovHorizontal = (sphereRadius * paddingFactor) / m_cameraDistance; // Dla sfery to to samo

    // Oblicz tangens połowy pionowego FOV, który zapewni dopasowanie w obu osiach
    float tanHalfFovY = qMax(requiredTanHalfFovVertical, requiredTanHalfFovHorizontal / aspectRatio);

    // Oblicz pionowy FOV w radianach, a następnie przekonwertuj na stopnie
    float fovYRadians = 2.0f * atan(tanHalfFovY);
    float fovYDegrees = qRadiansToDegrees(fovYRadians);

    // Ustawienie macierzy projekcji perspektywicznej z obliczonym FOV
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(fovYDegrees, aspectRatio, 0.1f, 100.0f); // Bliska i daleka płaszczyzna odcięcia

    qDebug() << "Resized to" << w << "x" << h << "Aspect:" << aspectRatio << "Calculated FOVy:" << fovYDegrees;
    // ---------------------------------------------------------
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
    // Kamera jest już ustawiona w world space, nie musimy jej transformować
    m_viewMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    m_modelMatrix.setToIdentity();
    m_modelMatrix.rotate(m_rotation);

    m_program->setUniformValue("projection", m_projectionMatrix);
    m_program->setUniformValue("view", m_viewMatrix);
    m_program->setUniformValue("model", m_modelMatrix);
    m_program->setUniformValue("time", m_timeValue);
    m_program->setUniformValue("audioAmplitude", m_currentAudioAmplitude);

    // --- Przekaż dane uderzeń do shadera ---
    QVector3D impactPointsGL[MAX_IMPACTS];
    float impactStartTimesGL[MAX_IMPACTS];
    for (int i = 0; i < MAX_IMPACTS; ++i) {
        if (m_impacts[i].active) {
            // Sprawdź, czy uderzenie nie jest za stare (opcjonalne czyszczenie tutaj)
            if (m_timeValue - m_impacts[i].startTime < 3.0f) { // Użyj wartości nieco większej niż impactDuration
                 impactPointsGL[i] = m_impacts[i].point;
                 impactStartTimesGL[i] = m_impacts[i].startTime;
            } else {
                 // Oznacz jako nieaktywne, jeśli czas minął
                 m_impacts[i].active = false;
                 impactPointsGL[i] = QVector3D(0,0,0); // Wyzeruj dane
                 impactStartTimesGL[i] = -1.0f;
            }
        } else {
            impactPointsGL[i] = QVector3D(0,0,0); // Wyzeruj dane dla nieaktywnych
            impactStartTimesGL[i] = -1.0f;
        }
    }
    // Użyj setUniformValueArray dla tablic (wymaga Qt 5.6+)
    // lub ustawiaj pojedynczo w pętli dla starszych wersji
    m_program->setUniformValueArray("u_impactPoints", impactPointsGL, MAX_IMPACTS);
    m_program->setUniformValueArray("u_impactStartTimes", impactStartTimesGL, MAX_IMPACTS, 1);
    // ------------------------------------

    glDrawArrays(GL_POINTS, 0, m_vertexCount);

    m_vao.release();
    m_program->release();
}

void FloatingEnergySphereWidget::updateAnimation()
{
    float currentTimeSecs = m_elapsedTimer.elapsed() / 1000.0f;
    float deltaTime = currentTimeSecs - m_lastFrameTimeSecs;
    m_lastFrameTimeSecs = currentTimeSecs;
    deltaTime = qMin(deltaTime, 0.05f);

    m_timeValue += deltaTime;
    if (m_timeValue > 3600.0f) m_timeValue -= 3600.0f;

    bool isPlaying = (m_player && m_player->state() == QMediaPlayer::PlayingState);

    // --- Usunięto logikę blend factor ---
    // float targetBlendFactor = isPlaying ? 0.0f : 1.0f;
    // float blendSmoothingFactor = 5.0f;
    // m_animationBlendFactor += (targetBlendFactor - m_animationBlendFactor) * blendSmoothingFactor * deltaTime;
    // m_animationBlendFactor = qBound(0.0f, m_animationBlendFactor, 1.0f);

    // --- Aktualizacja docelowej amplitudy (pozostaje bez zmian) ---
    if (isPlaying && m_audioReady && m_audioBufferDurationMs > 0) {
        qint64 currentPositionMs = m_player->position();
        int bufferIndex = static_cast<int>(currentPositionMs / m_audioBufferDurationMs);
        if (bufferIndex >= 0 && bufferIndex < m_audioAmplitudes.size()) {
            m_targetAudioAmplitude = m_audioAmplitudes[bufferIndex];
        } else {
            m_targetAudioAmplitude = 0.0f;
        }
    } else {
        m_targetAudioAmplitude = 0.0f;
    }

    // --- Płynne przejście dla aktualnej amplitudy (pozostaje bez zmian) ---
    float amplitudeSmoothingFactor = 10.0f;
    m_currentAudioAmplitude += (m_targetAudioAmplitude - m_currentAudioAmplitude) * amplitudeSmoothingFactor * deltaTime;

    // --- Przywrócono oryginalną logikę obrotu (zawsze aktywna) ---
    if (!m_mousePressed && m_angularVelocity.lengthSquared() > 0.0001f) {
        float rotationAngle = m_angularVelocity.length() * deltaTime;
        QQuaternion deltaRotation = QQuaternion::fromAxisAndAngle(m_angularVelocity.normalized(), rotationAngle);
        m_rotation = deltaRotation * m_rotation;
        m_rotation.normalize();
        // Oryginalne tłumienie
        m_angularVelocity *= pow(m_dampingFactor, deltaTime * 60.0f);
        if (m_angularVelocity.lengthSquared() < 0.0001f) {
            m_angularVelocity = QVector3D(0.0f, 0.0f, 0.0f);
        }
    } else if (!m_mousePressed) {
        // Oryginalny stały obrót
        QQuaternion slowSpin = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 2.0f * deltaTime);
        m_rotation = slowSpin * m_rotation;
        m_rotation.normalize();
    }
    // --- Koniec przywróconej logiki obrotu ---

    update(); // Zawsze aktualizuj
}

// <<< Nowy slot do przetwarzania buforów audio >>>
void FloatingEnergySphereWidget::processAudioBuffer()
{
    if (!m_decoder) return;

    QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid()) {
        qWarning() << "Invalid audio buffer received from decoder.";
        return;
    }

    if (!m_audioReady) { // Przy pierwszym buforze zapisz format i oblicz czas trwania
        m_audioFormat = buffer.format();
        if (m_audioFormat.isValid() && m_audioFormat.sampleRate() > 0 && m_audioFormat.bytesPerFrame() > 0) {
            m_audioBufferDurationMs = buffer.duration() / 1000; // duration() jest w mikrosekundach
            qDebug() << "Audio format set:" << m_audioFormat << "Buffer duration (ms):" << m_audioBufferDurationMs;
            // Oszacuj całkowitą liczbę buforów, jeśli dekoder podaje czas trwania
            qint64 totalDurationMs = m_decoder->duration() / 1000;
             if (totalDurationMs > 0 && m_audioBufferDurationMs > 0) {
                 int estimatedBuffers = static_cast<int>(totalDurationMs / m_audioBufferDurationMs) + 1;
                 m_audioAmplitudes.reserve(estimatedBuffers);
                 qDebug() << "Reserved space for approx." << estimatedBuffers << "amplitude values.";
             }
        } else {
             qWarning() << "Decoder provided invalid audio format initially.";
             // Ustaw domyślny czas trwania, aby uniknąć dzielenia przez zero, ale wizualizacja może nie działać
             m_audioBufferDurationMs = 50; // np. 50 ms
        }
    }

    // Oblicz i zapisz amplitudę RMS dla tego bufora
    float amplitude = calculateRMSAmplitude(buffer);
    m_targetAudioAmplitude = amplitude;
    m_audioAmplitudes.push_back(amplitude);

    // Oznacz, że mamy już jakieś dane audio
    if (!m_audioReady && !m_audioAmplitudes.empty()) {
        m_audioReady = true;
        qDebug() << "First audio buffer processed, audio is ready.";
    }
}

// <<< Nowa funkcja do obliczania amplitudy RMS >>>
float FloatingEnergySphereWidget::calculateRMSAmplitude(const QAudioBuffer& buffer)
{
    if (!buffer.isValid() || buffer.frameCount() == 0 || !m_audioFormat.isValid()) {
        return 0.0f;
    }

    const qint64 frameCount = buffer.frameCount();
    const int channelCount = m_audioFormat.channelCount();
    const int bytesPerSample = m_audioFormat.bytesPerFrame();

    if (channelCount <= 0 || bytesPerSample <= 0) return 0.0f;

    double sumOfSquares = 0.0;
    const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(buffer.constData());

    // Przetwarzaj tylko pierwszy kanał dla uproszczenia
    for (qint64 i = 0; i < frameCount; ++i) {
        float sampleValue = 0.0f;

        // Wskaźnik na początek próbki dla pierwszego kanału w bieżącej ramce
        const unsigned char* samplePtr = dataPtr + i * m_audioFormat.bytesPerFrame();

        // Dekodowanie próbki w zależności od formatu
        if (m_audioFormat.sampleType() == QAudioFormat::SignedInt) {
            if (bytesPerSample == 1) { // 8-bit Signed Int
                sampleValue = static_cast<float>(*reinterpret_cast<const qint8*>(samplePtr)) / std::numeric_limits<qint8>::max();
            } else if (bytesPerSample == 2) { // 16-bit Signed Int
                sampleValue = static_cast<float>(*reinterpret_cast<const qint16*>(samplePtr)) / std::numeric_limits<qint16>::max();
            } else if (bytesPerSample == 4) { // 32-bit Signed Int
                 sampleValue = static_cast<float>(*reinterpret_cast<const qint32*>(samplePtr)) / std::numeric_limits<qint32>::max();
            }
        } else if (m_audioFormat.sampleType() == QAudioFormat::UnSignedInt) {
             if (bytesPerSample == 1) { // 8-bit Unsigned Int
                sampleValue = (static_cast<float>(*reinterpret_cast<const quint8*>(samplePtr)) - 128.0f) / 128.0f;
            } else if (bytesPerSample == 2) { // 16-bit Unsigned Int
                 sampleValue = (static_cast<float>(*reinterpret_cast<const quint16*>(samplePtr)) - std::numeric_limits<quint16>::max()/2.0f) / (std::numeric_limits<quint16>::max()/2.0f);
            } else if (bytesPerSample == 4) { // 32-bit Unsigned Int
                 sampleValue = (static_cast<double>(*reinterpret_cast<const quint32*>(samplePtr)) - std::numeric_limits<quint32>::max()/2.0) / (std::numeric_limits<quint32>::max()/2.0);
            }
        } else if (m_audioFormat.sampleType() == QAudioFormat::Float) {
            if (bytesPerSample == 4) { // 32-bit Float
                sampleValue = *reinterpret_cast<const float*>(samplePtr);
            }
             else if (bytesPerSample == 8) { // 64-bit Float (Double)
                 sampleValue = static_cast<float>(*reinterpret_cast<const double*>(samplePtr));
             }
        }
        // Inne formaty nie są obsługiwane w tym przykładzie

        sumOfSquares += static_cast<double>(sampleValue * sampleValue);
    }

    // Oblicz RMS i znormalizuj (bardzo prosta normalizacja, może wymagać dostosowania)
    double meanSquare = sumOfSquares / frameCount;
    float rms = static_cast<float>(sqrt(meanSquare));

    // Prosta normalizacja - zakładamy, że maksymalna amplituda RMS to ok. 0.707 dla pełnej fali sinusoidalnej
    // Można dostosować ten współczynnik lub wprowadzić dynamiczną normalizację
    float normalizedRms = qBound(0.0f, rms / 0.707f, 1.0f);

    return normalizedRms;
}


// <<< Nowy slot wywoływany po zakończeniu dekodowania >>>
void FloatingEnergySphereWidget::audioDecodingFinished()
{
    qDebug() << "Audio decoding finished. Total amplitude values stored:" << m_audioAmplitudes.size();
    // Można tu zwolnić dekoder, jeśli nie jest już potrzebny
    // delete m_decoder;
    // m_decoder = nullptr;
}

// <<< Nowy slot do obsługi błędów odtwarzacza >>>
void FloatingEnergySphereWidget::handleMediaPlayerError()
{
     if(m_player) {
        qCritical() << "Media Player Error:" << m_player->errorString();
     } else {
        qCritical() << "Media Player Error: Player object is null.";
     }
}

// <<< Nowy slot do obsługi błędów dekodera >>>
void FloatingEnergySphereWidget::handleAudioDecoderError()
{
    if(m_decoder) {
        qCritical() << "Audio Decoder Error:" << m_decoder->errorString();
        m_audioReady = false; // Zaznacz, że dane audio nie są wiarygodne
    } else {
        qCritical() << "Audio Decoder Error: Decoder object is null.";
    }
}


// --- Metody obsługi myszy (mousePressEvent, mouseMoveEvent, mouseReleaseEvent, wheelEvent) ---
// Pozostają bez zmian
void FloatingEnergySphereWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
        m_angularVelocity = QVector3D(0.0f, 0.0f, 0.0f);

        qDebug() << "mousePressEvent Start. Time:" << m_timeValue; // <<< DEBUG

        // --- Logika Ray Casting ---
        QPointF pos = event->localPos();
        float widgetWidth = width();
        float widgetHeight = height();

        if (widgetWidth <= 0 || widgetHeight <= 0) {
             qWarning() << "Invalid widget dimensions:" << widgetWidth << "x" << widgetHeight;
             event->accept();
             return;
        }

        // 1. Normalizuj
        float ndcX = (pos.x() / widgetWidth) * 2.0f - 1.0f;
        float ndcY = 1.0f - (pos.y() / widgetHeight) * 2.0f;
        qDebug() << "NDC Coords:" << ndcX << ndcY; // <<< DEBUG

        // 2. Clip
        QVector4D clipCoords(ndcX, ndcY, -1.0f, 1.0f);

        // 3. Eye
        qDebug() << "Projection Matrix:\n" << m_projectionMatrix; // <<< DEBUG
        bool invertibleProj;
        QMatrix4x4 invProjection = m_projectionMatrix.inverted(&invertibleProj);
        if (!invertibleProj) { qDebug() << "Projection matrix not invertible"; event->accept(); return; }
        QVector4D eyeCoords = invProjection * clipCoords;
        eyeCoords.setZ(-1.0f);
        eyeCoords.setW(0.0f);
        qDebug() << "Eye Coords:" << eyeCoords; // <<< DEBUG

        // 4. World
        qDebug() << "View Matrix:\n" << m_viewMatrix; // <<< DEBUG
        bool invertibleView;
        QMatrix4x4 invView = m_viewMatrix.inverted(&invertibleView);
         if (!invertibleView) { qDebug() << "View matrix not invertible"; event->accept(); return; }
        QVector4D worldDir4 = invView * eyeCoords;
        QVector3D worldRayDir(worldDir4.x(), worldDir4.y(), worldDir4.z());
        worldRayDir.normalize();
        QVector3D worldRayOrigin = invView.column(3).toVector3D();
        qDebug() << "World Ray Origin:" << worldRayOrigin << "Dir:" << worldRayDir; // <<< DEBUG

        // 5. Model
        qDebug() << "Model Matrix:\n" << m_modelMatrix; // <<< DEBUG
        bool invertibleModel;
        QMatrix4x4 invModel = m_modelMatrix.inverted(&invertibleModel);
        if (!invertibleModel) { qDebug() << "Model matrix not invertible"; event->accept(); return; }
        QVector3D modelRayOrigin = invModel * worldRayOrigin;
        QVector3D modelRayDir = (invModel * QVector4D(worldRayDir, 0.0f)).toVector3D();
        modelRayDir.normalize();
        qDebug() << "Model Ray Origin:" << modelRayOrigin << "Dir:" << modelRayDir; // <<< DEBUG

        // 6. Przecięcie
        float a = QVector3D::dotProduct(modelRayDir, modelRayDir);
        float b = 2.0f * QVector3D::dotProduct(modelRayOrigin, modelRayDir);
        float c = QVector3D::dotProduct(modelRayOrigin, modelRayOrigin) - 1.0f;
        float discriminant = b*b - 4*a*c;
        qDebug() << "Intersection: a=" << a << "b=" << b << "c=" << c << "disc=" << discriminant; // <<< DEBUG

        if (discriminant >= 0) {
            float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
            float t = t1;
            qDebug() << "Intersection t=" << t; // <<< DEBUG

            if (t > 0.001f) {
                QVector3D modelIntersectionPoint = modelRayOrigin + t * modelRayDir;
                QVector3D impactPoint = modelIntersectionPoint.normalized();
                qDebug() << "Calculated Impact Point:" << impactPoint;

                // --- Zapisz uderzenie ---
                // Poprawione obliczanie indeksu, aby zawsze był nieujemny
                int index = (m_nextImpactIndex % MAX_IMPACTS + MAX_IMPACTS) % MAX_IMPACTS; // <<< ZMIANA TUTAJ

                qDebug() << "Attempting to write to m_impacts index:" << index << "(from nextImpactIndex:" << m_nextImpactIndex << ")"; // Dodano log m_nextImpactIndex

                // Sprawdź, czy wektor ma oczekiwany rozmiar (sanity check)
                if (m_impacts.size() != MAX_IMPACTS) {
                     qCritical() << "CRITICAL: m_impacts size mismatch! Expected:" << MAX_IMPACTS << "Actual:" << m_impacts.size();
                     event->accept();
                     return;
                }

                // Sprawdź, czy indeks jest poprawny (teraz powinien zawsze być)
                if (index < 0 || index >= MAX_IMPACTS) {
                    qCritical() << "CRITICAL: Invalid index calculated even after correction:" << index;
                    event->accept();
                    return;
                }

                // Bezpośrednio przed zapisem
                qDebug() << "Writing impact data...";
                m_impacts[index].point = impactPoint;
                m_impacts[index].startTime = m_timeValue;
                m_impacts[index].active = true;
                m_nextImpactIndex++; // Inkrementuj dopiero po udanym zapisie

                qDebug() << "Impact registered successfully at index" << index << "New nextImpactIndex:" << m_nextImpactIndex;

            } else {
                qDebug() << "Intersection is behind or too close to the camera (t =" << t << ")";
            }
        } else {
            qDebug() << "No intersection between ray and sphere.";
        }
        // --- Koniec logiki Ray Casting ---

        qDebug() << "mousePressEvent End.";
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

        float sensitivity = 0.2f; // Dostosuj czułość obrotu
        float angularSpeedX = (float)-delta.y() * sensitivity;
        float angularSpeedY = (float)-delta.x() * sensitivity;

        // Ustawiamy prędkość kątową (teraz w stopniach/klatkę, skalowanie w updateAnimation)
        m_angularVelocity = QVector3D(angularSpeedX, angularSpeedY, 0.0f);


        // Natychmiastowy obrót podczas przeciągania dla responsywności
        QQuaternion rotationX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, angularSpeedX);
        QQuaternion rotationY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, angularSpeedY);
        m_rotation = rotationY * rotationX * m_rotation;
        m_rotation.normalize();


        event->accept();
        update(); // Wymuś aktualizację, aby obrót był widoczny od razu
    } else {
        event->ignore();
    }
}


void FloatingEnergySphereWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
        // Prędkość kątowa została ustawiona w mouseMoveEvent, bezwładność zadziała w updateAnimation
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


void FloatingEnergySphereWidget::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    qDebug() << "Key pressed:" << QKeySequence(key).toString();

    // Dodaj klawisz do sekwencji
    m_keySequence.push_back(key);

    // Utrzymaj rozmiar sekwencji równy długości kodu Konami
    while (m_keySequence.size() > m_konamiCode.size()) {
        m_keySequence.pop_front();
    }

    // Sprawdź, czy sekwencja pasuje do kodu Konami
    if (m_keySequence.size() == m_konamiCode.size()) {
        bool match = true;
        for (size_t i = 0; i < m_konamiCode.size(); ++i) {
            if (m_keySequence[i] != m_konamiCode[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            qDebug() << "KONAMI CODE ENTERED!";
            // Wyemituj sygnał i wyczyść sekwencję, aby uniknąć wielokrotnego wywołania
            emit konamiCodeEntered();
            m_keySequence.clear();
            // Opcjonalnie: zatrzymaj timer podpowiedzi, jeśli był aktywny
            if (m_hintTimer->isActive()) {
                m_hintTimer->stop();
            }
        }
    }

    // Przekaż zdarzenie dalej, jeśli nie zostało obsłużone (lub zawsze)
    // QOpenGLWidget::keyPressEvent(event); // Raczej niepotrzebne
    event->accept(); // Akceptuj zdarzenie, aby nie było przekazywane wyżej
}

// --- Nowy slot do obsługi zmiany stanu odtwarzacza ---
void FloatingEnergySphereWidget::handlePlayerStateChanged(QMediaPlayer::State state)
{
    qDebug() << "Player state changed to:" << state;
    // Sprawdź, czy audio się zakończyło i czy timer podpowiedzi jeszcze nie działa
    if (state == QMediaPlayer::StoppedState && !m_hintTimer->isActive()) {
        // Sprawdź, czy pozycja jest bliska końca (unikaj uruchomienia timera przy błędach na starcie)
        if (m_player->position() > 0 && m_player->duration() > 0 &&
            m_player->position() >= m_player->duration() - 100) // Mały margines na końcu
        {
            qDebug() << "Initial audio finished. Starting 30s hint timer.";
            m_hintTimer->setSingleShot(true); // Upewnij się, że jest jednorazowy
            m_hintTimer->start(30000); // 30 sekund
        } else if (m_player->position() == 0 && m_player->error() == QMediaPlayer::NoError) {
             // Jeśli stan to Stopped, ale pozycja 0 (np. po ręcznym stop()), nie startuj timera
             qDebug() << "Player stopped manually or before starting, not starting hint timer.";
        } else if (m_player->error() != QMediaPlayer::NoError) {
             qDebug() << "Player stopped due to error, not starting hint timer.";
        }
    } else if (state == QMediaPlayer::PlayingState) {
        // Jeśli odtwarzanie się wznowi (np. hint), zatrzymaj timer
        if (m_hintTimer->isActive()) {
            qDebug() << "Player started playing, stopping hint timer.";
            m_hintTimer->stop();
        }
    }
}


// --- Nowy slot do odtwarzania podpowiedzi ---
void FloatingEnergySphereWidget::playKonamiHint()
{
    qDebug() << "Hint timer timed out. Attempting to play Konami hint.";

    // Sprawdź, czy widget jest nadal widoczny/aktywny
    if (!isVisible()) {
        qDebug() << "Widget not visible, skipping Konami hint.";
        return;
    }

    QString appDirPath = QCoreApplication::applicationDirPath();
    QString audioSubDir = "/assets/audio/";
    QString hintFileName = "konami.wav"; // Nazwa pliku podpowiedzi
    QString hintFilePath = QDir::cleanPath(appDirPath + audioSubDir + hintFileName);
    QUrl hintFileUrl;

    if (QFile::exists(hintFilePath)) {
        hintFileUrl = QUrl::fromLocalFile(hintFilePath);
        qDebug() << "Playing Konami hint audio:" << hintFilePath;

        // Zatrzymaj dekoder, jeśli działał (nie powinien, ale na wszelki wypadek)
        if (m_decoder && m_decoder->state() != QAudioDecoder::StoppedState) {
            m_decoder->stop();
        }
        // Zatrzymaj odtwarzacz, jeśli grał coś innego
        if (m_player->state() != QMediaPlayer::StoppedState) {
            m_player->stop();
        }

        // Ustaw nowe źródło i odtwórz
        m_player->setMedia(hintFileUrl);
        m_player->setVolume(85); // Można ustawić inną głośność dla podpowiedzi
        m_player->play();

    } else {
        qCritical() << "Konami hint audio file not found:" << hintFilePath;
        qCritical() << "Ensure the 'assets/audio' folder with '" << hintFileName << "' is copied next to the executable.";
    }
}

void FloatingEnergySphereWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "FloatingEnergySphereWidget closeEvent triggered. Closable:" << m_closable;
    if (m_closable) {
        // Zatrzymaj audio i timery przed zamknięciem
        if(m_player) m_player->stop();
        if(m_decoder) m_decoder->stop();
        if(m_hintTimer && m_hintTimer->isActive()) m_hintTimer->stop(); // <<< Zatrzymaj timer podpowiedzi
        emit widgetClosed();
        QWidget::closeEvent(event);
    } else {
        qDebug() << "Closing prevented by m_closable flag.";
        event->ignore();
    }
}