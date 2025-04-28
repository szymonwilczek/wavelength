#include "opengl_disintegration_effect.h"

OpenGLDisintegration::OpenGLDisintegration(QWidget *parent): QOpenGLWidget(parent), m_progress(0.0), m_texture(nullptr), m_program(nullptr) {
    // Zastosuj przezroczyste tło
    setAttribute(Qt::WA_TranslucentBackground);

    // Timer animacji
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &OpenGLDisintegration::updateAnimation);

    // Format dla OpenGL
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(4); // MSAA
    setFormat(format);
}

OpenGLDisintegration::~OpenGLDisintegration() {
    makeCurrent();
    m_vao.destroy();
    m_vbo.destroy();
    delete m_texture;
    delete m_program;
    doneCurrent();
}

void OpenGLDisintegration::setSourcePixmap(const QPixmap &pixmap) {
    m_sourcePixmap = pixmap;

    // Ponownie utwórz teksturę przy następdym renderowaniu
    if (m_texture) {
        makeCurrent();
        delete m_texture;
        m_texture = nullptr;
        doneCurrent();
    }

    update();
}

void OpenGLDisintegration::setProgress(qreal progress) {
    m_progress = qBound(0.0, progress, 1.0);
    update();
}

void OpenGLDisintegration::startAnimation(int duration) {
    m_progress = 0.0;
    m_animationDuration = duration;
    m_animationStart = QTime::currentTime();
    m_timer->start(16); // ~60 FPS
}

void OpenGLDisintegration::stopAnimation() const {
    m_timer->stop();
}

void OpenGLDisintegration::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    m_program = new QOpenGLShaderProgram();

    // Vertex shader
    const char* vertexShaderSource = R"(
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
    const char* fragmentShaderSource = R"(
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

    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    // Utwórz VAO i VBO
    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();

    // Przygotuj dane wierzchołków dla siatki cząstek
    createParticleGrid(20, 20); // 20x20 cząstek

    m_vbo.release();
    m_vao.release();
}

void OpenGLDisintegration::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void OpenGLDisintegration::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_sourcePixmap.isNull() || m_vertices.isEmpty())
        return;

    // Stwórz teksturę jeśli nie istnieje
    if (!m_texture) {
        m_texture = new QOpenGLTexture(m_sourcePixmap.toImage());
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    m_program->bind();

    // Macierz modelu (tożsamościowa)
    QMatrix4x4 model;
    m_program->setUniformValue("model", model);
    m_program->setUniformValue("progress", static_cast<float>(m_progress));
    m_program->setUniformValue("resolution", QVector2D(width(), height()));

    m_vao.bind();
    m_texture->bind(0);
    m_program->setUniformValue("texture1", 0);

    // Włącz blending dla przezroczystości
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Rysuj cząstki jako punkty
    glDrawArrays(GL_POINTS, 0, m_vertices.size() / 5);

    m_texture->release();
    m_vao.release();
    m_program->release();
}

void OpenGLDisintegration::updateAnimation() {
    int elapsed = m_animationStart.msecsTo(QTime::currentTime());
    qreal newProgress = static_cast<qreal>(elapsed) / m_animationDuration;

    if (newProgress >= 1.0) {
        m_progress = 1.0;
        m_timer->stop();
        emit animationFinished();
    } else {
        m_progress = newProgress;
    }

    update();
}

void OpenGLDisintegration::createParticleGrid(int cols, int rows) {
    m_vertices.clear();

    // Tworzymy siatkę cząstek
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float xPos = -1.0f + (x / float(cols - 1)) * 2.0f;
            float yPos = -1.0f + (y / float(rows - 1)) * 2.0f;

            // Pozycja (x, y, z) i koordynaty tekstury (u, v)
            m_vertices.append(xPos);
            m_vertices.append(yPos);
            m_vertices.append(0.0f);
            m_vertices.append(x / float(cols - 1));
            m_vertices.append(y / float(rows - 1));
        }
    }

    // Załaduj dane do bufora
    m_vbo.bind();
    m_vbo.allocate(m_vertices.data(), m_vertices.size() * sizeof(float));

    // Pozycja
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    // Koordynaty tekstury
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
}
