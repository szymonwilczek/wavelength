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

auto vertex_shader_source = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform float time;
    uniform float audioAmplitude;
    uniform float u_destructionFactor;

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

        // --- Efekt zniszczenia ---
        // Skalowanie pozycji w kierunku środka (0,0,0)
        // Używamy potęgi, aby kolaps przyspieszał pod koniec
        float collapseFactor = pow(u_destructionFactor, 2.0);
        vec3 finalPos = displacedPos * (1.0 - collapseFactor); // <<< KURCZENIE SIĘ

        // Dodaj chaotyczne przesunięcie na zewnątrz (opcjonalne, można dostosować siłę)
        float chaosStrength = u_destructionFactor * 0.5;
        vec3 chaosOffset = normal * (noise(vPos * 5.0 + time * 2.0) - 0.5) * chaosStrength;
        finalPos += chaosOffset; // Dodaje trochę "rozpadania się" podczas kurczenia

        // --- Zaktualizuj vDeformationFactor ---
        if (u_destructionFactor == 0.0) {
             float maxExpectedDeformation = baseAmplitude * 2.0 + audioDeformationScale;
             vDeformationFactor = smoothstep(0.0, max(0.1, maxExpectedDeformation + abs(maxDepth)), abs(totalIdleDeformation + audioEffect + totalImpactDeformation));
        } else {
             vDeformationFactor = u_destructionFactor; // Użyj postępu niszczenia dla koloru/blasku
        }

        gl_Position = projection * view * model * vec4(finalPos, 1.0); // Użyj finalPos

        // --- Rozmiar punktu ---
        float basePointSize = 0.9;
        float sizeVariation = 0.5;
        float smoothTimeFactor = 0.3;
        float idlePointSizeVariation = sizeVariation * vDeformationFactor * (0.3 * sin(time * smoothTimeFactor + aPos.y * 0.5));
        float audioPointSizeVariation = audioAmplitude * 1.5;
        gl_PointSize = basePointSize + idlePointSizeVariation + audioPointSizeVariation;

        // Skaluj rozmiar punktu w dół podczas niszczenia
        gl_PointSize *= (1.0 - u_destructionFactor); // <<< ZMNIEJSZANIE PUNKTÓW
        gl_PointSize = max(0.1, gl_PointSize); // Zapobiegaj natychmiastowemu zniknięciu punktów
    }
)";

// Fragment Shader: Bez zmian
auto fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 vPos;
    in float vDeformationFactor; // Teraz zawiera postęp niszczenia
    uniform float time;
    uniform float u_destructionFactor; // Można też przekazać bezpośrednio

    void main()
    {
        vec3 colorBlue = vec3(0.1, 0.3, 1.0);
        vec3 colorViolet = vec3(0.7, 0.2, 0.9);
        vec3 colorPink = vec3(1.0, 0.4, 0.8);
        vec3 colorRed = vec3(1.0, 0.1, 0.1); // Kolor dla zniszczenia

        float factor = (normalize(vPos).y + 1.0) / 2.0;

        vec3 baseColor;
        if (factor < 0.5) {
            baseColor = mix(colorBlue, colorViolet, factor * 2.0);
        } else {
            baseColor = mix(colorViolet, colorPink, (factor - 0.5) * 2.0);
        }

        // Zmień kolor na czerwony podczas niszczenia
        baseColor = mix(baseColor, colorRed, u_destructionFactor);

        float glow = 0.4 + 0.8 * vDeformationFactor + 0.3 * abs(sin(vPos.x * 2.0 + time * 1.5));
        vec3 finalColor = baseColor * (glow + 0.2);

        float baseAlpha = 0.75;
        // Opcjonalnie: Zmniejszaj alpha podczas niszczenia (może powodować problemy z przezroczystością tła)
        // float finalAlpha = baseAlpha * (1.0 - u_destructionFactor);
        float finalAlpha = baseAlpha;
        FragColor = vec4(finalColor, finalAlpha);
    }
)";
// --- Koniec Shaders ---


FloatingEnergySphereWidget::FloatingEnergySphereWidget(const bool is_first_time, QWidget *parent)
    : QOpenGLWidget(parent),
      is_first_time_(is_first_time),
      time_value_(0.0f),
      shader_program_(nullptr),
      vbo_(QOpenGLBuffer::VertexBuffer),
      camera_position_(0.0f, 0.0f, 3.5f),
      camera_distance_(3.5f),
      mouse_pressed_(false),
      vertex_count_(0),
      closable_(true),
      angular_velocity_(0.0f, 0.0f, 0.0f),
      damping_factor_(0.96f),
      last_frame_time_secs_(0.0f), // <<< Inicjalizacja
      media_player_(nullptr), // <<< Inicjalizacja
      audio_decoder_(nullptr), // <<< Inicjalizacja
      audio_buffer_duration_ms_(0), // <<< Inicjalizacja
      current_audio_amplitude_(0.0f), // <<< Inicjalizacja
      audio_ready_(false),
      next_impact_index_(0),
      konami_code_({Qt::Key_Up, Qt::Key_Down, Qt::Key_Up, Qt::Key_Down,
                    Qt::Key_Left, Qt::Key_Right, Qt::Key_Left, Qt::Key_Right,
                    Qt::Key_B, Qt::Key_A, Qt::Key_Return}),
      hint_timer_(new QTimer(this)),
      is_destroying_(false),
      destruction_progress_(0.0f),
      click_simulation_timer_(new QTimer(this))
{
    if constexpr (MAX_IMPACTS > 0) { // Upewnij się, że MAX_IMPACTS jest dodatnie
        impacts_.resize(MAX_IMPACTS); // <<< DODAJ ZMIANĘ ROZMIARU
    } else {
        qWarning() << "MAX_IMPACTS is not positive, impacts will not work.";
    }
    for(auto& impact : impacts_) {
        impact.active = false;
        impact.start_time = -1.0f; // Oznacz jako nieaktywne
    }

    media_player_ = new QMediaPlayer(this);
    audio_decoder_ = new QAudioDecoder(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    resize(600, 600);

    elapsed_timer_.start();
    last_frame_time_secs_ = elapsed_timer_.elapsed() / 1000.0f;

    connect(&timer_, &QTimer::timeout, this, &FloatingEnergySphereWidget::UpdateAnimation);
    timer_.start(16);

    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    setFormat(fmt);

    connect(&timer_, &QTimer::timeout, this, &FloatingEnergySphereWidget::UpdateAnimation);
    connect(hint_timer_, &QTimer::timeout, this, &FloatingEnergySphereWidget::PlayKonamiHint);
    connect(media_player_, &QMediaPlayer::stateChanged, this, &FloatingEnergySphereWidget::HandlePlayerStateChanged);

    connect(click_simulation_timer_, &QTimer::timeout, this, &FloatingEnergySphereWidget::SimulateClick);
    click_simulation_timer_->setInterval(1000);

    timer_.start(16);
    click_simulation_timer_->start();
}

FloatingEnergySphereWidget::~FloatingEnergySphereWidget()
{
    makeCurrent();
    timer_.stop();
    click_simulation_timer_->stop();
    if (hint_timer_) hint_timer_->stop();
    if (media_player_) media_player_->stop();
    if (audio_decoder_) audio_decoder_->stop();

    vbo_.destroy();
    vao_.destroy();
    delete shader_program_;
    doneCurrent();
}

void FloatingEnergySphereWidget::SetClosable(const bool closable) {
    closable_ = closable;
}

void FloatingEnergySphereWidget::initializeGL()
{
    if (!initializeOpenGLFunctions()) {
        qCritical() << "Failed to initialize OpenGL functions!";
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    SetupShaders();
    if (!shader_program_ || !shader_program_->isLinked()) {
        qCritical() << "Shader program not linked after setupShaders()!";
        if(shader_program_) qCritical() << "Shader Log:" << shader_program_->log();
        return;
    }
    qDebug() << "Shaders set up successfully.";

    SetupSphereGeometry(128, 256);

    vao_.create();
    vao_.bind();

    vbo_.create();
    vbo_.bind();
    vbo_.allocate(vertices_.data(), vertices_.size() * sizeof(GLfloat));

    shader_program_->enableAttributeArray(0);
    shader_program_->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(GLfloat));

    vao_.release();
    vbo_.release();
    shader_program_->release();


    // --- Konfiguracja Audio ---
    SetupAudio(); // <<< Wywołanie konfiguracji audio

    qDebug() << "FloatingEnergySphereWidget::initializeGL() finished successfully.";
}

// <<< Nowa funkcja do konfiguracji audio >>>
void FloatingEnergySphereWidget::SetupAudio()
{
    qDebug() << "Setting up audio. Is first time:" << is_first_time_;

    if (!media_player_ || !audio_decoder_) {
        qCritical() << "Audio objects (player/decoder) are null!";
        return;
    }

    connect(media_player_, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &FloatingEnergySphereWidget::HandleMediaPlayerError);
    connect(audio_decoder_, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &FloatingEnergySphereWidget::HandleAudioDecoderError);
    connect(audio_decoder_, &QAudioDecoder::bufferReady, this, &FloatingEnergySphereWidget::ProcessAudioBuffer);
    connect(audio_decoder_, &QAudioDecoder::finished, this, &FloatingEnergySphereWidget::AudioDecodingFinished);

    const QString app_dir_path = QCoreApplication::applicationDirPath();
    const QString audio_sub_dir = "/assets/audio/";

    // <<< Wybór nazwy pliku na podstawie flagi >>>
    QString audio_file_name;
    if (is_first_time_) {
        audio_file_name = "JOI.wav"; // Plik dla pierwszego uruchomienia
        qDebug() << "Playing initial audio file:" << audio_file_name;
    } else {
        audio_file_name = "hello_again.wav"; // Plik dla kolejnych uruchomień
        qDebug() << "Playing subsequent audio file:" << audio_file_name;
    }
    // <<< Koniec wyboru nazwy pliku >>>

    QString chosen_file_path = QDir::cleanPath(app_dir_path + audio_sub_dir + audio_file_name);
    QUrl chosen_file_url;

    if (QFile::exists(chosen_file_path)) {
        chosen_file_url = QUrl::fromLocalFile(chosen_file_path);
        qDebug() << "Using audio file:" << chosen_file_path;
    } else {
        // Spróbuj alternatywnego pliku (np. MP3), jeśli główny nie istnieje
        // LUB zgłoś krytyczny błąd, jeśli wybrany plik jest wymagany
        const QString fallback_file_name = is_first_time_ ? "JOImp3.mp3" : ""; // Można dodać fallback dla hello_again
        if (!fallback_file_name.isEmpty()) {
             const QString fallback_file_path = QDir::cleanPath(app_dir_path + audio_sub_dir + fallback_file_name);
             if (QFile::exists(fallback_file_path)) {
                 chosen_file_path = fallback_file_path;
                 chosen_file_url = QUrl::fromLocalFile(chosen_file_path);
                 qWarning() << "Primary audio file not found, using fallback:" << chosen_file_path;
             } else {
                 qCritical() << "Chosen audio file AND fallback not found:" << chosen_file_path << "and" << fallback_file_path;
                 return;
             }
        } else {
             qCritical() << "Chosen audio file not found:" << chosen_file_path;
             qCritical() << "Ensure the 'assets/audio' folder with '" << audio_file_name << "' is copied next to the executable.";
             return;
        }
    }

    audio_decoder_->setSourceFilename(chosen_file_path);
    audio_decoder_->start();
    qDebug() << "Audio decoder started for:" << chosen_file_path;

    media_player_->setMedia(chosen_file_url);
    media_player_->setVolume(75);
    media_player_->play();
    qDebug() << "Audio player started for:" << chosen_file_url.toString();

    if (media_player_->state() == QMediaPlayer::StoppedState && media_player_->error() != QMediaPlayer::NoError) {
         qWarning() << "Player entered StoppedState immediately after play() call. Error:" << media_player_->errorString();
    }
}


void FloatingEnergySphereWidget::SetupShaders()
{
    shader_program_ = new QOpenGLShaderProgram(this);

    if (!shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source)) {
        qCritical() << "Vertex shader compilation failed:" << shader_program_->log();
        delete shader_program_; shader_program_ = nullptr;
        return;
    }

    if (!shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source)) {
        qCritical() << "Fragment shader compilation failed:" << shader_program_->log();
        delete shader_program_; shader_program_ = nullptr;
        return;
    }

    if (!shader_program_->link()) {
        qCritical() << "Shader program linking failed:" << shader_program_->log();
        delete shader_program_; shader_program_ = nullptr;
        return;
    }
}

void FloatingEnergySphereWidget::SetupSphereGeometry(const int rings, const int sectors)
{
    vertices_.clear();
    float const ring_factor = 1. / static_cast<float>(rings - 1);
    float const sector_factor = 1. / static_cast<float>(sectors - 1);
    vertices_.resize(rings * sectors * 3);
    auto v = vertices_.begin();
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            constexpr float radius = 1.0f;
            constexpr float PI = 3.14159265359f;
            float const y = sin(-PI / 2 + PI * r * ring_factor);
            float const x = cos(2 * PI * s * sector_factor) * sin(PI * r * ring_factor);
            float const z = sin(2 * PI * s * sector_factor) * sin(PI * r * ring_factor);
            *v++ = x * radius;
            *v++ = y * radius;
            *v++ = z * radius;
        }
    }
    vertex_count_ = rings * sectors;
}


void FloatingEnergySphereWidget::resizeGL(const int w, int h)
{
    if (h == 0) h = 1; // Zapobieganie dzieleniu przez zero

    // Ustawienie viewportu OpenGL na cały widget
    glViewport(0, 0, w, h);

    // --- Dynamiczne obliczanie FOV dla stałego rozmiaru sfery ---
    const float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);
    constexpr float sphere_radius = 1.0f; // Promień sfery w przestrzeni modelu
    // Współczynnik określający, jaką część mniejszego wymiaru widgetu ma zajmować sfera
    // Np. 1.1 oznacza, że sfera zajmie ok. 91% mniejszego wymiaru, dając mały margines.
    // Zwiększ tę wartość, aby sfera była mniejsza (większy margines).
    // Zmniejsz (bliżej 1.0), aby była większa (mniejszy margines).
    constexpr float padding_factor = 2.0f;

    // Oblicz tangens połowy wymaganego kąta widzenia, aby zmieścić sferę z marginesem
    // Musimy zapewnić, że zmieści się zarówno w pionie, jak i w poziomie.
    // Wybieramy większy z wymaganych kątów (co odpowiada mniejszemu wymiarowi).
    const float required_tan_half_fov_vertical = (sphere_radius * padding_factor) / camera_distance_;
    const float required_tan_half_fov_horizontal = (sphere_radius * padding_factor) / camera_distance_; // Dla sfery to to samo

    // Oblicz tangens połowy pionowego FOV, który zapewni dopasowanie w obu osiach
    const float tan_half_fov_y = qMax(required_tan_half_fov_vertical, required_tan_half_fov_horizontal / aspect_ratio);

    // Oblicz pionowy FOV w radianach, a następnie przekonwertuj na stopnie
    const float fov_y_radians = 2.0f * atan(tan_half_fov_y);
    const float fov_y_degrees = qRadiansToDegrees(fov_y_radians);

    // Ustawienie macierzy projekcji perspektywicznej z obliczonym FOV
    projection_matrix_.setToIdentity();
    projection_matrix_.perspective(fov_y_degrees, aspect_ratio, 0.1f, 100.0f); // Bliska i daleka płaszczyzna odcięcia

    // ---------------------------------------------------------
}

void FloatingEnergySphereWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shader_program_ || !shader_program_->isLinked()) {
        return;
    }

    shader_program_->bind();
    vao_.bind();

    view_matrix_.setToIdentity();
    // Kamera jest już ustawiona w world space, nie musimy jej transformować
    view_matrix_.lookAt(camera_position_, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    model_matrix_.setToIdentity();
    model_matrix_.rotate(rotation_);

    shader_program_->setUniformValue("projection", projection_matrix_);
    shader_program_->setUniformValue("view", view_matrix_);
    shader_program_->setUniformValue("model", model_matrix_);
    shader_program_->setUniformValue("time", time_value_);
    shader_program_->setUniformValue("audioAmplitude", current_audio_amplitude_);
    shader_program_->setUniformValue("u_destructionFactor", destruction_progress_);

    // --- Przekaż dane uderzeń do shadera ---
    QVector3D impact_points_gl[MAX_IMPACTS];
    float impact_start_times_gl[MAX_IMPACTS];
    for (int i = 0; i < MAX_IMPACTS; ++i) {
        if (impacts_[i].active) {
            // Sprawdź, czy uderzenie nie jest za stare (opcjonalne czyszczenie tutaj)
            if (time_value_ - impacts_[i].start_time < 3.0f) { // Użyj wartości nieco większej niż impactDuration
                 impact_points_gl[i] = impacts_[i].point;
                 impact_start_times_gl[i] = impacts_[i].start_time;
            } else {
                 // Oznacz jako nieaktywne, jeśli czas minął
                 impacts_[i].active = false;
                 impact_points_gl[i] = QVector3D(0,0,0); // Wyzeruj dane
                 impact_start_times_gl[i] = -1.0f;
            }
        } else {
            impact_points_gl[i] = QVector3D(0,0,0); // Wyzeruj dane dla nieaktywnych
            impact_start_times_gl[i] = -1.0f;
        }
    }
    // Użyj setUniformValueArray dla tablic (wymaga Qt 5.6+)
    // lub ustawiaj pojedynczo w pętli dla starszych wersji
    shader_program_->setUniformValueArray("u_impactPoints", impact_points_gl, MAX_IMPACTS);
    shader_program_->setUniformValueArray("u_impactStartTimes", impact_start_times_gl, MAX_IMPACTS, 1);
    // ------------------------------------

    glDrawArrays(GL_POINTS, 0, vertex_count_);

    vao_.release();
    shader_program_->release();
}

void FloatingEnergySphereWidget::UpdateAnimation()
{
    const float current_time_secs = elapsed_timer_.elapsed() / 1000.0f;
    float delta_time = current_time_secs - last_frame_time_secs_;
    last_frame_time_secs_ = current_time_secs;
    delta_time = qMin(delta_time, 0.05f);

    time_value_ += delta_time;
    if (time_value_ > 3600.0f) time_value_ -= 3600.0f;

    if (is_destroying_) {
        // Zwiększaj postęp zniszczenia
        // Dostosuj czas trwania do długości dźwięku "goodbye.wav" lub własnych preferencji
        constexpr float destruction_duration = 8.0f;
        destruction_progress_ += delta_time / destruction_duration;
        destruction_progress_ = qBound(0.0f, destruction_progress_, 1.0f);

        // Zatrzymaj obrót i reakcję na audio podczas niszczenia
        angular_velocity_ = QVector3D(0.0f, 0.0f, 0.0f);
        current_audio_amplitude_ = 0.0f; // Płynnie wyzeruj amplitudę
        target_audio_amplitude_ = 0.0f;

        // Nie aktualizuj normalnej animacji/obrotu
        update();
        return; // Zakończ updateAnimation tutaj dla stanu niszczenia
    }

    const bool is_playing = (media_player_ && media_player_->state() == QMediaPlayer::PlayingState);

    // --- Usunięto logikę blend factor ---
    // float targetBlendFactor = isPlaying ? 0.0f : 1.0f;
    // float blendSmoothingFactor = 5.0f;
    // m_animationBlendFactor += (targetBlendFactor - m_animationBlendFactor) * blendSmoothingFactor * deltaTime;
    // m_animationBlendFactor = qBound(0.0f, m_animationBlendFactor, 1.0f);

    // --- Aktualizacja docelowej amplitudy (pozostaje bez zmian) ---
    if (is_playing && audio_ready_ && audio_buffer_duration_ms_ > 0) {
        const qint64 current_position_ms = media_player_->position();
        const int buffer_index = static_cast<int>(current_position_ms / audio_buffer_duration_ms_);
        if (buffer_index >= 0 && buffer_index < audio_amplitudes_.size()) {
            target_audio_amplitude_ = audio_amplitudes_[buffer_index];
        } else {
            target_audio_amplitude_ = 0.0f;
        }
    } else {
        target_audio_amplitude_ = 0.0f;
    }

    // --- Płynne przejście dla aktualnej amplitudy (pozostaje bez zmian) ---
    constexpr float amplitude_smoothing_factor = 10.0f;
    current_audio_amplitude_ += (target_audio_amplitude_ - current_audio_amplitude_) * amplitude_smoothing_factor * delta_time;

    // --- Przywrócono oryginalną logikę obrotu (zawsze aktywna) ---
    if (!mouse_pressed_ && angular_velocity_.lengthSquared() > 0.0001f) {
        const float rotation_angle = angular_velocity_.length() * delta_time;
        const QQuaternion delta_rotation = QQuaternion::fromAxisAndAngle(angular_velocity_.normalized(), rotation_angle);
        rotation_ = delta_rotation * rotation_;
        rotation_.normalize();
        // Oryginalne tłumienie
        angular_velocity_ *= pow(damping_factor_, delta_time * 60.0f);
        if (angular_velocity_.lengthSquared() < 0.0001f) {
            angular_velocity_ = QVector3D(0.0f, 0.0f, 0.0f);
        }
    } else if (!mouse_pressed_) {
        // Oryginalny stały obrót
        const QQuaternion slow_spin = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 2.0f * delta_time);
        rotation_ = slow_spin * rotation_;
        rotation_.normalize();
    }
    // --- Koniec przywróconej logiki obrotu ---

    update(); // Zawsze aktualizuj
}

// <<< Nowy slot do przetwarzania buforów audio >>>
void FloatingEnergySphereWidget::ProcessAudioBuffer()
{
    if (!audio_decoder_) return;

    const QAudioBuffer buffer = audio_decoder_->read();
    if (!buffer.isValid()) {
        qWarning() << "Invalid audio buffer received from decoder.";
        return;
    }

    if (!audio_ready_) { // Przy pierwszym buforze zapisz format i oblicz czas trwania
        audio_format_ = buffer.format();
        if (audio_format_.isValid() && audio_format_.sampleRate() > 0 && audio_format_.bytesPerFrame() > 0) {
            audio_buffer_duration_ms_ = buffer.duration() / 1000; // duration() jest w mikrosekundach
            qDebug() << "Audio format set:" << audio_format_ << "Buffer duration (ms):" << audio_buffer_duration_ms_;
            // Oszacuj całkowitą liczbę buforów, jeśli dekoder podaje czas trwania
            const qint64 total_duration_ms = audio_decoder_->duration() / 1000;
             if (total_duration_ms > 0 && audio_buffer_duration_ms_ > 0) {
                 const int estimated_buffers = static_cast<int>(total_duration_ms / audio_buffer_duration_ms_) + 1;
                 audio_amplitudes_.reserve(estimated_buffers);
             }
        } else {
             qWarning() << "Decoder provided invalid audio format initially.";
             // Ustaw domyślny czas trwania, aby uniknąć dzielenia przez zero, ale wizualizacja może nie działać
             audio_buffer_duration_ms_ = 50; // np. 50 ms
        }
    }

    // Oblicz i zapisz amplitudę RMS dla tego bufora
    const float amplitude = CalculateRMSAmplitude(buffer);
    target_audio_amplitude_ = amplitude;
    audio_amplitudes_.push_back(amplitude);

    // Oznacz, że mamy już jakieś dane audio
    if (!audio_ready_ && !audio_amplitudes_.empty()) {
        audio_ready_ = true;
        qDebug() << "First audio buffer processed, audio is ready.";
    }
}

// <<< Nowa funkcja do obliczania amplitudy RMS >>>
float FloatingEnergySphereWidget::CalculateRMSAmplitude(const QAudioBuffer& buffer) const {
    if (!buffer.isValid() || buffer.frameCount() == 0 || !audio_format_.isValid()) {
        return 0.0f;
    }

    const qint64 frame_count = buffer.frameCount();
    const int channel_count = audio_format_.channelCount();
    const int bytes_per_sample = audio_format_.bytesPerFrame();

    if (channel_count <= 0 || bytes_per_sample <= 0) return 0.0f;

    double sum_of_squares = 0.0;
    const auto data_ptr = static_cast<const unsigned char*>(buffer.constData());

    // Przetwarzaj tylko pierwszy kanał dla uproszczenia
    for (qint64 i = 0; i < frame_count; ++i) {
        float sample_value = 0.0f;

        // Wskaźnik na początek próbki dla pierwszego kanału w bieżącej ramce
        const unsigned char* sample_ptr = data_ptr + i * audio_format_.bytesPerFrame();

        // Dekodowanie próbki w zależności od formatu
        if (audio_format_.sampleType() == QAudioFormat::SignedInt) {
            if (bytes_per_sample == 1) { // 8-bit Signed Int
                sample_value = static_cast<float>(*reinterpret_cast<const qint8*>(sample_ptr)) / std::numeric_limits<qint8>::max();
            } else if (bytes_per_sample == 2) { // 16-bit Signed Int
                sample_value = static_cast<float>(*reinterpret_cast<const qint16*>(sample_ptr)) / std::numeric_limits<qint16>::max();
            } else if (bytes_per_sample == 4) { // 32-bit Signed Int
                 sample_value = static_cast<float>(*reinterpret_cast<const qint32*>(sample_ptr)) / std::numeric_limits<qint32>::max();
            }
        } else if (audio_format_.sampleType() == QAudioFormat::UnSignedInt) {
             if (bytes_per_sample == 1) { // 8-bit Unsigned Int
                sample_value = (static_cast<float>(*reinterpret_cast<const quint8*>(sample_ptr)) - 128.0f) / 128.0f;
            } else if (bytes_per_sample == 2) { // 16-bit Unsigned Int
                 sample_value = (static_cast<float>(*reinterpret_cast<const quint16*>(sample_ptr)) - std::numeric_limits<quint16>::max()/2.0f) / (std::numeric_limits<quint16>::max()/2.0f);
            } else if (bytes_per_sample == 4) { // 32-bit Unsigned Int
                 sample_value = (static_cast<double>(*reinterpret_cast<const quint32*>(sample_ptr)) - std::numeric_limits<quint32>::max()/2.0) / (std::numeric_limits<quint32>::max()/2.0);
            }
        } else if (audio_format_.sampleType() == QAudioFormat::Float) {
            if (bytes_per_sample == 4) { // 32-bit Float
                sample_value = *reinterpret_cast<const float*>(sample_ptr);
            }
             else if (bytes_per_sample == 8) { // 64-bit Float (Double)
                 sample_value = static_cast<float>(*reinterpret_cast<const double*>(sample_ptr));
             }
        }

        sum_of_squares += static_cast<double>(sample_value * sample_value);
    }

    // Oblicz RMS i znormalizuj (bardzo prosta normalizacja, może wymagać dostosowania)
    const double mean_square = sum_of_squares / frame_count;
    const float rms = static_cast<float>(sqrt(mean_square));

    // Prosta normalizacja - zakładamy, że maksymalna amplituda RMS to ok. 0.707 dla pełnej fali sinusoidalnej
    // Można dostosować ten współczynnik lub wprowadzić dynamiczną normalizację
    const float normalized_rms = qBound(0.0f, rms / 0.707f, 1.0f);

    return normalized_rms;
}


// <<< Nowy slot wywoływany po zakończeniu dekodowania >>>
void FloatingEnergySphereWidget::AudioDecodingFinished() const {
    qDebug() << "Audio decoding finished. Total amplitude values stored:" << audio_amplitudes_.size();
    // Można tu zwolnić dekoder, jeśli nie jest już potrzebny
    // delete m_decoder;
    // m_decoder = nullptr;
}

// <<< Nowy slot do obsługi błędów odtwarzacza >>>
void FloatingEnergySphereWidget::HandleMediaPlayerError() const {
     if(media_player_) {
        qCritical() << "Media Player Error:" << media_player_->errorString();
     } else {
        qCritical() << "Media Player Error: Player object is null.";
     }
}

// <<< Nowy slot do obsługi błędów dekodera >>>
void FloatingEnergySphereWidget::HandleAudioDecoderError()
{
    if(audio_decoder_) {
        qCritical() << "Audio Decoder Error:" << audio_decoder_->errorString();
        audio_ready_ = false; // Zaznacz, że dane audio nie są wiarygodne
    } else {
        qCritical() << "Audio Decoder Error: Decoder object is null.";
    }
}


// --- Metody obsługi myszy (mousePressEvent, mouseMoveEvent, mouseReleaseEvent, wheelEvent) ---
// Pozostają bez zmian
void FloatingEnergySphereWidget::mousePressEvent(QMouseEvent *event)
{
   event->accept();
}

void FloatingEnergySphereWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (mouse_pressed_ && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->pos() - last_mouse_position_;
        last_mouse_position_ = event->pos();

        constexpr float sensitivity = 0.2f; // Dostosuj czułość obrotu
        const float angular_speed_x = static_cast<float>(-delta.y()) * sensitivity;
        const float angular_speed_y = static_cast<float>(-delta.x()) * sensitivity;

        // Ustawiamy prędkość kątową (teraz w stopniach/klatkę, skalowanie w updateAnimation)
        angular_velocity_ = QVector3D(angular_speed_x, angular_speed_y, 0.0f);


        // Natychmiastowy obrót podczas przeciągania dla responsywności
        const QQuaternion rotation_x = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, angular_speed_x);
        const QQuaternion rotation_y = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, angular_speed_y);
        rotation_ = rotation_y * rotation_x * rotation_;
        rotation_.normalize();


        event->accept();
        update(); // Wymuś aktualizację, aby obrót był widoczny od razu
    } else {
        event->ignore();
    }
}


void FloatingEnergySphereWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouse_pressed_ = false;
        // Prędkość kątowa została ustawiona w mouseMoveEvent, bezwładność zadziała w updateAnimation
        event->accept();
    } else {
        event->ignore();
    }
}

void FloatingEnergySphereWidget::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta > 0) {
        camera_distance_ *= 0.95f;
    } else if (delta < 0) {
        camera_distance_ *= 1.05f;
    }
    camera_distance_ = qBound(1.5f, camera_distance_, 15.0f);

    event->accept();
    update();
}


void FloatingEnergySphereWidget::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();
    qDebug() << "Key pressed:" << QKeySequence(key).toString();

    // Dodaj klawisz do sekwencji
    key_sequence_.push_back(key);

    if (konami_code_.empty()) {
        qWarning() << "Konami code is empty, sequence matching will not work.";
        event->accept(); // Mimo wszystko akceptuj, aby widget nie tracił fokusu
        return;
    }

    // Utrzymaj rozmiar sekwencji równy długości kodu Konami
    while (key_sequence_.size() > konami_code_.size()) {
        key_sequence_.pop_front();
    }

    // Sprawdź, czy sekwencja pasuje do kodu Konami
    if (key_sequence_.size() == konami_code_.size()) {
        bool match = true;
        for (size_t i = 0; i < konami_code_.size(); ++i) {
            if (key_sequence_[i] != konami_code_[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            qDebug() << "KONAMI CODE ENTERED! Starting destruction sequence.";
            // Wywołaj nową funkcję zamiast emitować sygnał od razu
            StartDestructionSequence(); // <<< ZMIANA TUTAJ
            key_sequence_.clear();
            if (hint_timer_->isActive()) {
                hint_timer_->stop();
            }
            // Nie emituj już konamiCodeEntered tutaj
            // emit konamiCodeEntered();
        }
    }

    // Przekaż zdarzenie dalej, jeśli nie zostało obsłużone (lub zawsze)
    // QOpenGLWidget::keyPressEvent(event); // Raczej niepotrzebne
    event->accept(); // Akceptuj zdarzenie, aby nie było przekazywane wyżej
}

void FloatingEnergySphereWidget::StartDestructionSequence()
{
    if (is_destroying_) return; // Już w trakcie niszczenia

    is_destroying_ = true;
    destruction_progress_ = 0.0f;
    SetClosable(false); // Zapobiegaj zamknięciu przez użytkownika podczas animacji
    click_simulation_timer_->stop();

    // Zatrzymaj obecne audio (np. podpowiedź), jeśli gra
    if (media_player_->state() == QMediaPlayer::PlayingState) {
        media_player_->stop();
    }
    if (audio_decoder_ && audio_decoder_->state() != QAudioDecoder::StoppedState) {
        audio_decoder_->stop(); // Zatrzymaj też dekoder, jeśli działał
    }

    // Znajdź i odtwórz dźwięk "goodbye.wav"
    const QString app_dir_path = QCoreApplication::applicationDirPath();
    const QString audio_sub_dir = "/assets/audio/";
    const QString goodbye_file_path = QDir::cleanPath(app_dir_path + audio_sub_dir + kGoodbyeSoundFilename);

    if (QFile::exists(goodbye_file_path)) {
        const QUrl goodbye_file_url = QUrl::fromLocalFile(goodbye_file_path);
        qDebug() << "Playing destruction audio:" << goodbye_file_path;
        media_player_->setMedia(goodbye_file_url);
        media_player_->setVolume(80); // Ustaw głośność
        media_player_->play();
    } else {
        qCritical() << "Destruction audio file not found:" << goodbye_file_path;
        // Co zrobić, jeśli plik nie istnieje? Można od razu emitować sygnał zakończenia
        // lub kontynuować bez dźwięku. Dla bezpieczeństwa emitujmy od razu.
        emit destructionSequenceFinished();
        closable_ = true;
        close();
    }
}

// --- Nowy slot do obsługi zmiany stanu odtwarzacza ---
void FloatingEnergySphereWidget::HandlePlayerStateChanged(const QMediaPlayer::State state)
{

    // --- Logika dla zakończenia dźwięku zniszczenia ---
    if (is_destroying_ && state == QMediaPlayer::StoppedState) {
        // Sprawdź, czy zakończony plik to rzeczywiście dźwięk zniszczenia
        if (media_player_->media().canonicalUrl().fileName() == kGoodbyeSoundFilename) {
            qDebug() << "Destruction audio finished.";
            // Upewnij się, że animacja wizualnie dobiegła końca (opcjonalne opóźnienie lub sprawdzenie progress)
            // Dla uproszczenia emitujemy od razu
            emit destructionSequenceFinished();
            // Pozwól na zamknięcie i zamknij widget
            closable_ = true;
            close(); // Wywołaj closeEvent
            return; // Zakończ obsługę dla tego stanu
        }
    }
    // --- Koniec logiki dla zniszczenia ---

    // Sprawdź, czy audio się zakończyło i czy timer podpowiedzi jeszcze nie działa
    if (!is_destroying_ && state == QMediaPlayer::StoppedState && !hint_timer_->isActive()) {
        // Sprawdź, czy pozycja jest bliska końca (unikaj uruchomienia timera przy błędach na starcie)
        if (media_player_->position() > 0 && media_player_->duration() > 0 &&
            media_player_->position() >= media_player_->duration() - 100) // Mały margines na końcu
        {
            qDebug() << "Initial audio finished. Starting 30s hint timer.";
            hint_timer_->setSingleShot(true); // Upewnij się, że jest jednorazowy
            hint_timer_->start(30000); // 30 sekund
        } else if (media_player_->position() == 0 && media_player_->error() == QMediaPlayer::NoError) {
             // Jeśli stan to Stopped, ale pozycja 0 (np. po ręcznym stop()), nie startuj timera
             qDebug() << "Player stopped manually or before starting, not starting hint timer.";
        } else if (media_player_->error() != QMediaPlayer::NoError) {
             qDebug() << "Player stopped due to error, not starting hint timer.";
        }
    } else if (!is_destroying_ && state == QMediaPlayer::PlayingState) {
        // Jeśli odtwarzanie się wznowi (np. hint), zatrzymaj timer
        if (hint_timer_->isActive()) {
            qDebug() << "Player started playing, stopping hint timer.";
            hint_timer_->stop();
        }
    }
}


// --- Nowy slot do odtwarzania podpowiedzi ---
void FloatingEnergySphereWidget::PlayKonamiHint() const {
    qDebug() << "Hint timer timed out. Attempting to play Konami hint.";

    // Sprawdź, czy widget jest nadal widoczny/aktywny
    if (!isVisible()) {
        qDebug() << "Widget not visible, skipping Konami hint.";
        return;
    }

    const QString app_dir_path = QCoreApplication::applicationDirPath();
    const QString audio_sub_dir = "/assets/audio/";
    const QString hint_file_name = "konami.wav"; // Nazwa pliku podpowiedzi
    const QString file_name = QDir::cleanPath(app_dir_path + audio_sub_dir + hint_file_name);

    if (QFile::exists(file_name)) {
        const QUrl hintFileUrl = QUrl::fromLocalFile(file_name);
        qDebug() << "Playing Konami hint audio:" << file_name;

        // Zatrzymaj dekoder, jeśli działał (nie powinien, ale na wszelki wypadek)
        if (audio_decoder_ && audio_decoder_->state() != QAudioDecoder::StoppedState) {
            audio_decoder_->stop();
        }
        // Zatrzymaj odtwarzacz, jeśli grał coś innego
        if (media_player_->state() != QMediaPlayer::StoppedState) {
            media_player_->stop();
        }

        // Ustaw nowe źródło i odtwórz
        media_player_->setMedia(hintFileUrl);
        media_player_->setVolume(85); // Można ustawić inną głośność dla podpowiedzi
        media_player_->play();

    } else {
        qCritical() << "Konami hint audio file not found:" << file_name;
        qCritical() << "Ensure the 'assets/audio' folder with '" << hint_file_name << "' is copied next to the executable.";
    }
}

void FloatingEnergySphereWidget::closeEvent(QCloseEvent *event)
{
    if (closable_) {
        timer_.stop();
        click_simulation_timer_->stop();
        if(media_player_) media_player_->stop();
        if(audio_decoder_) audio_decoder_->stop();
        if(hint_timer_ && hint_timer_->isActive()) hint_timer_->stop();

        if (!is_destroying_) {
            emit widgetClosed();
        }
        QWidget::closeEvent(event);
    } else {
        qDebug() << "Closing prevented by m_closable flag.";
        event->ignore();
    }
}


void FloatingEnergySphereWidget::TriggerImpactAtScreenPos(const QPoint& screen_position)
{
    if (is_destroying_ || !isVisible()) return; // Nie rób nic, jeśli niszczymy lub widget jest niewidoczny

    makeCurrent(); // Upewnij się, że kontekst GL jest aktywny

    // --- Logika Ray Casting (przeniesiona z mousePressEvent) ---
    const float win_x = screen_position.x();
    const float win_y = height() - screen_position.y(); // Odwróć Y dla OpenGL

    // Potrzebujemy aktualnych macierzy widoku i projekcji
    // Jeśli nie są one składowymi klasy, trzeba je będzie uzyskać/obliczyć tutaj
    // Zakładając, że m_viewMatrix i m_projectionMatrix są aktualne:
    const QMatrix4x4 view_projection = projection_matrix_ * view_matrix_ * model_matrix_; // Połączone macierze

    bool invertible;
    const QMatrix4x4 inverse_view_projection = view_projection.inverted(&invertible);

    if (!invertible) {
        qWarning() << "Cannot invert view-projection matrix for ray casting.";
        doneCurrent();
        return;
    }

    // Normalizowane współrzędne urządzenia (NDC)
    const float ndc_x = (2.0f * win_x) / width() - 1.0f;
    const float ndc_y = (2.0f * win_y) / height() - 1.0f; // Y już odwrócone wcześniej

    // Współrzędne w przestrzeni świata (lub oka, zależnie od macierzy)
    const QVector4D near_point_h = inverse_view_projection * QVector4D(ndc_x, ndc_y, -1.0f, 1.0f); // Bliska płaszczyzna
    const QVector4D far_point_h  = inverse_view_projection * QVector4D(ndc_x, ndc_y,  1.0f, 1.0f); // Daleka płaszczyzna

    if (qAbs(near_point_h.w()) < 1e-6 || qAbs(far_point_h.w()) < 1e-6) {
         qWarning() << "W component is zero during unprojection.";
         doneCurrent();
         return;
    }

    // Podziel przez W, aby uzyskać współrzędne 3D
    const QVector3D near_point = near_point_h.toVector3DAffine(); // nearPointH.toVector3D() / nearPointH.w();
    const QVector3D far_point  = far_point_h.toVector3DAffine(); // farPointH.toVector3D() / farPointH.w();


    // Kierunek promienia (z punktu widzenia kamery)
    const QVector3D ray_direction = (far_point - near_point).normalized();
    // Początek promienia (pozycja kamery w przestrzeni modelu)
    // Można użyć nearPoint lub pozycji kamery, jeśli jest znana
    const QVector3D ray_origin = near_point; // Użycie nearPoint jest często wystarczające


    // Przecięcie Promień-Sfera (centrum w 0,0,0, promień ~1.0)
    constexpr float radius = 1.0f; // Załóżmy promień 1.0
    const float a = QVector3D::dotProduct(ray_direction, ray_direction); // Powinno być 1.0, jeśli znormalizowany
    const float b = 2.0f * QVector3D::dotProduct(ray_origin, ray_direction);
    const float c = QVector3D::dotProduct(ray_origin, ray_origin) - radius * radius;

    const float discriminant = b * b - 4.0f * a * c;

    if (discriminant >= 0) {
        const float t1 = (-b - std::sqrt(discriminant)) / (2.0f * a);
        // float t2 = (-b + std::sqrt(discriminant)) / (2.0f * a); // Drugi punkt przecięcia
        const float t = t1; // Bierzemy bliższy punkt

        if (t > 0.001f) { // Sprawdź, czy przecięcie jest przed kamerą
            const QVector3D intersection_point = ray_origin + t * ray_direction;
            const QVector3D normalized_impact_point = intersection_point.normalized(); // Punkt na sferze jednostkowej

            // --- Dodaj efekt uderzenia ---
            if (MAX_IMPACTS > 0) { // Sprawdź, czy tablica/wektor może istnieć
                // Modulo zapewni, że indeks jest w zakresie [0, MAX_IMPACTS-1]
                const int index_to_use = next_impact_index_ % MAX_IMPACTS;

                // Dodatkowe sprawdzenie (jeśli używasz wektora i nie masz pewności co do rozmiaru)
                // if (indexToUse >= 0 && indexToUse < m_impacts.size()) {
                impacts_[index_to_use].point = normalized_impact_point;
                impacts_[index_to_use].start_time = time_value_;
                impacts_[index_to_use].active = true;
                next_impact_index_ = (index_to_use + 1) % MAX_IMPACTS; // Aktualizuj używając poprawnego indeksu
                // } else {
                //     qWarning() << "Invalid index or impacts container size for simulated impact.";
                // }
            }
            // qDebug() << "Simulated impact triggered at screen pos:" << screenPos << "-> sphere point:" << normalizedImpactPoint;
        } else {
            // qDebug() << "Simulated click ray intersection behind origin.";
        }
    } else {
        // qDebug() << "Simulated click ray missed the sphere.";
    }
    doneCurrent(); // Zwolnij kontekst GL
}

void FloatingEnergySphereWidget::SimulateClick() {
    if (is_destroying_ || !isVisible() || width() <= 0 || height() <= 0) {
        return; // Nie symuluj, jeśli niszczymy, niewidoczny lub rozmiar jest nieprawidłowy
    }

    // Utwórz generator liczb losowych tylko raz lub użyj statycznego
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Wygeneruj losowe współrzędne X, Y w obrębie widgetu
    std::uniform_int_distribution<> distrib_x(0, width() - 1);
    std::uniform_int_distribution<> distrib_y(0, height() - 1);

    const QPoint screen_position(distrib_x(gen), distrib_y(gen));

    // Wywołaj logikę uderzenia dla wylosowanego punktu
    TriggerImpactAtScreenPos(screen_position);
}