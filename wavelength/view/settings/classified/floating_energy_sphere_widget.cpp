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

// --- Zaktualizowane Shaders ---
// Vertex Shader: Dodano uniform 'audioAmplitude' i uwzględniono go w deformacji
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time; // Globalny czas
    uniform float audioAmplitude;
    // uniform float animationBlendFactor; // <<< Usunięto

    out vec3 vPos;
    out float vDeformationFactor;

    float noise(vec3 p) {
        return fract(sin(dot(p, vec3(12.9898, 78.233, 54.53))) * 43758.5453);
    }

    // <<< Przywrócono 6-parametrową definicję evolvingWave >>>
    float evolvingWave(vec3 pos, float baseFreq, float freqVariation, float amp, float speed, float timeOffset) {
        float freqTimeFactor = 0.23;
        float offsetTimeFactor = 0.16;
        // Używamy globalnego 'time'
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

        // float idleTime = time * animationBlendFactor; // <<< Usunięto

        float baseAmplitude = 0.26;
        float baseSpeed = 0.33;
        // Wywołania używają globalnego 'time' (6 parametrów)
        float deformation1 = evolvingWave(aPos, 2.0, 0.55, baseAmplitude, baseSpeed * 0.8, 0.0);
        float deformation2 = evolvingWave(aPos * 1.5, 3.5, 1.1, baseAmplitude * 0.7, baseSpeed * 1.5, 1.5);
        float thinkingAmplitudeFactor = 0.85;
        float thinkingDeformation = evolvingWave(aPos * 0.5, 1.0, 0.35, baseAmplitude * thinkingAmplitudeFactor, baseSpeed * 0.15, 5.0);
        // Funkcja noise używa globalnego 'time'
        float noiseDeformation = (noise(aPos * 2.5 + time * 0.04) - 0.5) * baseAmplitude * 0.15;

        float totalIdleDeformation = deformation1 + deformation2 + thinkingDeformation + noiseDeformation;

        float audioDeformationScale = 1.8;
        float audioEffect = audioAmplitude * audioDeformationScale;

        // Deformacja idle jest zawsze dodawana do deformacji audio
        float totalDeformation = totalIdleDeformation + audioEffect;

        float maxExpectedDeformation = baseAmplitude * 2.0 + audioDeformationScale;
        vDeformationFactor = smoothstep(0.0, max(0.1, maxExpectedDeformation), abs(totalDeformation));

        vec3 displacedPos = aPos + normal * totalDeformation;

        gl_Position = projection * view * model * vec4(displacedPos, 1.0);

        float basePointSize = 0.9;
        float sizeVariation = 0.5;
        float smoothTimeFactor = 0.3;
        // Rozmiar punktu używa globalnego 'time'
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
      m_dampingFactor(0.96f),
      m_player(nullptr), // <<< Inicjalizacja
      m_decoder(nullptr), // <<< Inicjalizacja
      m_audioBufferDurationMs(0), // <<< Inicjalizacja
      m_currentAudioAmplitude(0.0f), // <<< Inicjalizacja
      m_audioReady(false) // <<< Inicjalizacja
{

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
    qDebug() << "Setting up audio using QMediaPlayer/QAudioDecoder with local files...";

    // Sprawdź, czy obiekty zostały utworzone w konstruktorze
    if (!m_player || !m_decoder) {
        qCritical() << "Audio objects (player/decoder) are null!";
        return;
    }

    // --- Obsługa błędów ---
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &FloatingEnergySphereWidget::handleMediaPlayerError);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &FloatingEnergySphereWidget::handleAudioDecoderError);

    // --- Konfiguracja dekodera ---
    connect(m_decoder, &QAudioDecoder::bufferReady, this, &FloatingEnergySphereWidget::processAudioBuffer);
    connect(m_decoder, &QAudioDecoder::finished, this, &FloatingEnergySphereWidget::audioDecodingFinished);

    // --- Znajdź ścieżkę do katalogu aplikacji ---
    QString appDirPath = QCoreApplication::applicationDirPath();
    qDebug() << "Application directory:" << appDirPath;

    // --- Skonstruuj pełne ścieżki do plików audio ---
    // Zakładamy, że pliki są w podkatalogu "assets/audio" względem pliku .exe
    QString audioSubDir = "/assets/audio/"; // Użyj '/' dla kompatybilności
    QString wavFilePath = QDir::cleanPath(appDirPath + audioSubDir + "JOI.wav");
    QString mp3FilePath = QDir::cleanPath(appDirPath + audioSubDir + "JOImp3.mp3");

    QString chosenFilePath;
    QUrl chosenFileUrl;

    // --- Wybór pliku audio (priorytet dla WAV) ---
    if (QFile::exists(wavFilePath)) {
        chosenFilePath = wavFilePath;
        chosenFileUrl = QUrl::fromLocalFile(chosenFilePath);
        qDebug() << "Using WAV audio file:" << chosenFilePath;
    } else if (QFile::exists(mp3FilePath)) {
        chosenFilePath = mp3FilePath;
        chosenFileUrl = QUrl::fromLocalFile(chosenFilePath);
        qDebug() << "WAV not found, using MP3 audio file:" << chosenFilePath;
    } else {
        qCritical() << "Neither WAV nor MP3 audio file found in expected location:" << QDir::cleanPath(appDirPath + audioSubDir);
        qCritical() << "Checked paths:" << wavFilePath << "and" << mp3FilePath;
        qCritical() << "Ensure the 'assets/audio' folder with sound files is copied next to the executable.";
        // Nie twórz obiektów playera/dekodera, jeśli plik nie istnieje
        // delete m_player; m_player = nullptr; // Już niepotrzebne, są tworzone w konstruktorze
        // delete m_decoder; m_decoder = nullptr;
        return;
    }

    // --- Uruchomienie dekodera ---
    m_decoder->setSourceFilename(chosenFilePath); // QAudioDecoder używa QString
    m_decoder->start();
    qDebug() << "Audio decoder started for:" << chosenFilePath;

    // --- Konfiguracja odtwarzacza ---
    m_player->setMedia(chosenFileUrl); // QMediaPlayer używa QUrl::fromLocalFile
    m_player->setVolume(75);
    m_player->play();
    qDebug() << "Audio player started for:" << chosenFileUrl.toString();

    // Sprawdź stan od razu po wywołaniu play()
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
    glViewport(0, 0, w, h);
    m_projectionMatrix.setToIdentity();
    float aspect = float(w) / float(h ? h : 1);
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
    m_modelMatrix.rotate(m_rotation);

    m_program->setUniformValue("projection", m_projectionMatrix);
    m_program->setUniformValue("view", m_viewMatrix);
    m_program->setUniformValue("model", m_modelMatrix);
    m_program->setUniformValue("time", m_timeValue);
    m_program->setUniformValue("audioAmplitude", m_currentAudioAmplitude); // <<< Przekazanie amplitudy do shadera

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

void FloatingEnergySphereWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "FloatingEnergySphereWidget closeEvent triggered. Closable:" << m_closable;
    if (m_closable) {
        // Zatrzymaj audio przed zamknięciem
        if(m_player) m_player->stop();
        if(m_decoder) m_decoder->stop();
        emit widgetClosed();
        QWidget::closeEvent(event);
    } else {
        qDebug() << "Closing prevented by m_closable flag.";
        event->ignore();
    }
}