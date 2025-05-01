#include "opengl_disintegration_effect.h"

OpenGLDisintegration::OpenGLDisintegration(QWidget *parent): QOpenGLWidget(parent), progress_(0.0), texture_(nullptr), shader_program_(nullptr) {
    // Zastosuj przezroczyste tło
    setAttribute(Qt::WA_TranslucentBackground);

    // Timer animacji
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &OpenGLDisintegration::UpdateAnimation);

    // Format dla OpenGL
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(4); // MSAA
    setFormat(format);
}

OpenGLDisintegration::~OpenGLDisintegration() {
    makeCurrent();
    vao_.destroy();
    vbo_.destroy();
    delete texture_;
    delete shader_program_;
    doneCurrent();
}

void OpenGLDisintegration::SetSourcePixmap(const QPixmap &pixmap) {
    source_pixmap_ = pixmap;

    // Ponownie utwórz teksturę przy następdym renderowaniu
    if (texture_) {
        makeCurrent();
        delete texture_;
        texture_ = nullptr;
        doneCurrent();
    }

    update();
}

void OpenGLDisintegration::SetProgress(const qreal progress) {
    progress_ = qBound(0.0, progress, 1.0);
    update();
}

void OpenGLDisintegration::StartAnimation(const int duration) {
    progress_ = 0.0;
    animation_duration_ = duration;
    animation_start_ = QTime::currentTime();
    timer_->start(16); // ~60 FPS
}

void OpenGLDisintegration::StopAnimation() const {
    timer_->stop();
}

void OpenGLDisintegration::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    shader_program_ = new QOpenGLShaderProgram();

    // Vertex shader
    const auto vertex_shader_source = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            uniform mat4 model;
            uniform float progress;
            uniform vec2 resolution;

            // Funkcja pseudolosowa
            float random(vec2 st) {
                return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
            }

            void main() {
                TexCoord = aTexCoord;

                // Efekt dezintegracji
                vec3 position = aPos;

                // Dodajemy losowe przesunięcie bazujące na czasie i pozycji
                if (progress > 0.0) {
                    float rnd = random(aTexCoord);
                    float noise = random(aTexCoord + progress);

                    // Przesunięcie proporcjonalne do postępu animacji
                    position.x += sin(noise * 10.0) * progress * 0.5;
                    position.y += cos(noise * 10.0) * progress * 0.5;

                    // Alpha zależy od postępu i pozycji losowej
                    gl_Position = model * vec4(position, 1.0 - step(rnd, progress * 0.8));
                } else {
                    gl_Position = model * vec4(position, 1.0);
                }
            }
        )";

    // Fragment shader
    const auto fragment_shader_source = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 TexCoord;

            uniform sampler2D texture1;
            uniform float progress;

            void main() {
                vec4 texColor = texture(texture1, TexCoord);

                // Zmniejszamy alpha w miarę postępu
                texColor.a *= (1.0 - progress * 0.8);

                // Dodajemy lekki efekt odbarwienia przy zaniku
                texColor.rgb = mix(texColor.rgb, vec3(0.2, 0.5, 1.0) * texColor.rgb, progress * 0.5);

                FragColor = texColor;
            }
        )";

    shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source);
    shader_program_->link();

    // Utwórz VAO i VBO
    vao_.create();
    vao_.bind();

    vbo_.create();
    vbo_.bind();

    // Przygotuj dane wierzchołków dla siatki cząstek
    CreateParticleGrid(20, 20); // 20x20 cząstek

    vbo_.release();
    vao_.release();
}

void OpenGLDisintegration::resizeGL(const int w, const int h) {
    glViewport(0, 0, w, h);
}

void OpenGLDisintegration::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (source_pixmap_.isNull() || vertices_.isEmpty())
        return;

    // Stwórz teksturę jeśli nie istnieje
    if (!texture_) {
        texture_ = new QOpenGLTexture(source_pixmap_.toImage());
        texture_->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        texture_->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    shader_program_->bind();

    // Macierz modelu (tożsamościowa)
    const QMatrix4x4 model;
    shader_program_->setUniformValue("model", model);
    shader_program_->setUniformValue("progress", static_cast<float>(progress_));
    shader_program_->setUniformValue("resolution", QVector2D(width(), height()));

    vao_.bind();
    texture_->bind(0);
    shader_program_->setUniformValue("texture1", 0);

    // Włącz blending dla przezroczystości
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Rysuj cząstki jako punkty
    glDrawArrays(GL_POINTS, 0, vertices_.size() / 5);

    texture_->release();
    vao_.release();
    shader_program_->release();
}

void OpenGLDisintegration::UpdateAnimation() {
    const int elapsed = animation_start_.msecsTo(QTime::currentTime());
    const qreal new_progress = static_cast<qreal>(elapsed) / animation_duration_;

    if (new_progress >= 1.0) {
        progress_ = 1.0;
        timer_->stop();
        emit animationFinished();
    } else {
        progress_ = new_progress;
    }

    update();
}

void OpenGLDisintegration::CreateParticleGrid(const int cols, const int rows) {
    vertices_.clear();

    // Tworzymy siatkę cząstek
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float xPos = -1.0f + (x / static_cast<float>(cols - 1)) * 2.0f;
            float yPos = -1.0f + (y / static_cast<float>(rows - 1)) * 2.0f;

            // Pozycja (x, y, z) i koordynaty tekstury (u, v)
            vertices_.append(xPos);
            vertices_.append(yPos);
            vertices_.append(0.0f);
            vertices_.append(x / static_cast<float>(cols - 1));
            vertices_.append(y / static_cast<float>(rows - 1));
        }
    }

    // Załaduj dane do bufora
    vbo_.bind();
    vbo_.allocate(vertices_.data(), vertices_.size() * sizeof(float));

    // Pozycja
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    // Koordynaty tekstury
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
}
