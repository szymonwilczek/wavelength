#include "editable_stream_widget.h"
#include <QDebug>
#include <cmath>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>

EditableStreamWidget::EditableStreamWidget(QWidget* parent)
    : QOpenGLWidget(parent),
      m_waveAmplitude(0.01),
      m_waveFrequency(2.0),
      m_waveSpeed(1.0),
      m_glitchIntensity(0.0),
      m_waveThickness(0.008),
      m_timeOffset(0.0),
      m_shaderProgram(nullptr),
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_waveTexture(nullptr),
      m_initialized(false),
      m_numControlPoints(10),  // Liczba punktów kontrolnych - możesz dostosować
      m_selectedPointIndex(-1),
      m_isEditing(false),
      m_helpTextOpacity(0),
      m_helpTextVisible(false)
{
    setMinimumSize(400, 150);
    setMouseTracking(true);  // Potrzebne do śledzenia myszy nad punktami
    setFocusPolicy(Qt::StrongFocus);  // Potrzebne do obsługi klawisza ESCAPE

    // Timer animacji
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &EditableStreamWidget::updateAnimation);
    m_animTimer->setTimerType(Qt::PreciseTimer);
    m_animTimer->start(16); // ~60 FPS

    // Timer wygaszania glitcha (bez zmian)
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

    // Timer dla etykiety pomocy
    m_helpTextTimer = new QTimer(this);
    m_helpTextTimer->setInterval(50);
    connect(m_helpTextTimer, &QTimer::timeout, this, &EditableStreamWidget::updateEditHelpText);

    // Format dla OpenGL (bez zmian)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapInterval(1);
    setFormat(format);

    setAttribute(Qt::WA_OpaquePaintEvent, false);  // Zmieniamy na false, aby paintEvent również działał
    setAttribute(Qt::WA_NoSystemBackground, true);

    // Tworzymy etykietę pomocy
    m_helpLabel = new QLabel(this);
    m_helpLabel->setText("Naciśnij ESCAPE, aby zakończyć edycję");
    m_helpLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 180); padding: 5px; border-radius: 3px;");
    m_helpLabel->setAlignment(Qt::AlignCenter);
    m_helpLabel->setVisible(false);

    // Inicjalizujemy punkty kontrolne
    initializeControlPoints();
}

EditableStreamWidget::~EditableStreamWidget() {
    makeCurrent();
    m_vao.destroy();
    m_vertexBuffer.destroy();
    delete m_shaderProgram;
    delete m_waveTexture;
    doneCurrent();
}

void EditableStreamWidget::initializeControlPoints() {
    m_controlPoints.resize(m_numControlPoints);

    // Inicjalizujemy punkty kontrolne na płaskiej linii
    for (int i = 0; i < m_numControlPoints; ++i) {
        qreal x = -1.0 + 2.0 * static_cast<qreal>(i) / (m_numControlPoints - 1);
        m_controlPoints[i] = QPointF(x, 0.0);  // Zawsze płaska linia
    }
}

void EditableStreamWidget::resetControlPoints() {
    // Resetujemy wszystkie punkty kontrolne do płaskiej linii (y=0)
    for (int i = 0; i < m_numControlPoints; ++i) {
        qreal x = -1.0 + 2.0 * static_cast<qreal>(i) / (m_numControlPoints - 1);
        m_controlPoints[i] = QPointF(x, 0.0);
    }

    // Aktualizujemy teksturę po zresetowaniu punktów
    updateWaveTexture();
}

QPointF EditableStreamWidget::windowToGL(const QPoint& windowPos) {
    // Konwersja ze współrzędnych okna do współrzędnych OpenGL (-1..1)
    return QPointF(
        (2.0 * windowPos.x() / width()) - 1.0,
        1.0 - (2.0 * windowPos.y() / height())
    );
}

QPoint EditableStreamWidget::glToWindow(const QPointF& glPos) {
    // Konwersja ze współrzędnych OpenGL (-1..1) do współrzędnych okna
    return QPoint(
        static_cast<int>((glPos.x() + 1.0) * width() / 2.0),
        static_cast<int>((1.0 - glPos.y()) * height() / 2.0)
    );
}

int EditableStreamWidget::findClosestControlPoint(const QPointF& pos) {
    int closestIndex = -1;
    qreal minDistSq = 0.01;  // Minimalny kwadrat odległości (próg)

    for (int i = 0; i < m_numControlPoints; ++i) {
        qreal dx = pos.x() - m_controlPoints[i].x();
        qreal dy = pos.y() - m_controlPoints[i].y();
        qreal distSq = dx * dx + dy * dy;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestIndex = i;
        }
    }

    return closestIndex;
}

void EditableStreamWidget::updateWaveTexture() {
    if (!m_initialized || !m_waveTexture) return;

    makeCurrent();

    // Tworzymy dane dla tekstury (wartości fali)
    QVector<float> textureData(TEXTURE_SIZE);

    // Interpolujemy wartości między punktami kontrolnymi
    for (int i = 0; i < TEXTURE_SIZE; ++i) {
        qreal x = -1.0 + 2.0 * static_cast<qreal>(i) / (TEXTURE_SIZE - 1);

        // Znajdź odpowiedni segment między punktami kontrolnymi
        // Uproszczona i bardziej niezawodna metoda
        int leftIndex = 0;
        int rightIndex = 0;

        // Znajdź dwa sąsiednie punkty kontrolne
        for (int j = 0; j < m_numControlPoints - 1; ++j) {
            if (x >= m_controlPoints[j].x() && x <= m_controlPoints[j + 1].x()) {
                leftIndex = j;
                rightIndex = j + 1;
                break;
            }
        }

        // Interpolacja liniowa między punktami
        qreal x1 = m_controlPoints[leftIndex].x();
        qreal y1 = m_controlPoints[leftIndex].y();
        qreal x2 = m_controlPoints[rightIndex].x();
        qreal y2 = m_controlPoints[rightIndex].y();

        // Zabezpieczenie przed dzieleniem przez zero
        qreal t = (x2 == x1) ? 0.0 : (x - x1) / (x2 - x1);
        t = qBound(0.0, t, 1.0);  // Upewnij się, że t jest w zakresie [0,1]

        textureData[i] = y1 + t * (y2 - y1);
    }

    // Aktualizujemy teksturę - tworzymy nową teksturę zamiast aktualizować
    // To rozwiązuje problem z aktualizacją fragmentów tekstury
    if (m_waveTexture) {
        delete m_waveTexture;
    }

    m_waveTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
    m_waveTexture->setSize(TEXTURE_SIZE);
    m_waveTexture->setFormat(QOpenGLTexture::R32F);
    m_waveTexture->allocateStorage();
    m_waveTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float16, textureData.constData());
    m_waveTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_waveTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_waveTexture->setMagnificationFilter(QOpenGLTexture::Linear);

    update();
}

void EditableStreamWidget::initializeGL() {
    qDebug() << "EditableStreamWidget::initializeGL() started...";
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Inicjalizujemy shader program - zmodyfikowany, aby używać tekstury
    m_shaderProgram = new QOpenGLShaderProgram();

    // Vertex shader - bez zmian
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    // Fragment shader - zmodyfikowany, aby używać tekstury z punktami kontrolnymi
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
    uniform sampler1D waveTexture;
    uniform bool editMode;

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
        float glow = 0.03;
        vec3 lineColor = vec3(0.0, 0.7, 1.0);  // Neonowy niebieski
        vec3 glowColor = vec3(0.0, 0.4, 0.8);  // Ciemniejszy niebieski dla poświaty

        // Pobierz bazowy kształt fali z tekstury
        float texCoord = (uv.x + 1.0) * 0.5;  // Mapuj z -1..1 na 0..1
        float wave = texture(waveTexture, texCoord).r;

        // W trybie edycji pokazujemy tylko kształt bazowy
        if (!editMode) {
            // W normalnym trybie dodajemy animację i efekty
            float animWave = sin(uv.x * frequency * 3.14159 + time * speed) * amplitude;
            wave += animWave;

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
        float alpha = line + glowFactor * 0.6 + 0.1;

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

    // Tworzymy VAO
    m_vao.create();
    if (!m_vao.isCreated()) {
        qCritical() << "VAO creation failed!";
        return;
    }
    m_vao.bind();

    // Wierzchołki prostokąta dla fullscreen quad
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
    m_vertexBuffer.bind();
    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vertexBuffer.allocate(vertices, sizeof(vertices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    // Inicjalizujemy teksturę z kształtem fali
    QVector<float> initialWaveData(TEXTURE_SIZE, 0.0f);  // Początkowo płaska linia

    m_waveTexture = new QOpenGLTexture(QOpenGLTexture::Target1D);
    m_waveTexture->setSize(TEXTURE_SIZE);
    m_waveTexture->setFormat(QOpenGLTexture::R32F);
    m_waveTexture->allocateStorage();
    m_waveTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float16, initialWaveData.constData());
    m_waveTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_waveTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_waveTexture->setMagnificationFilter(QOpenGLTexture::Linear);

    m_vertexBuffer.release();
    m_vao.release();

    m_initialized = true;
    qDebug() << "EditableStreamWidget::initializeGL() completed successfully.";

    // Aktualizujemy teksturę na podstawie punktów kontrolnych
    updateWaveTexture();
}

void EditableStreamWidget::resizeGL(int w, int h) {
    qDebug() << "EditableStreamWidget::resizeGL called with W=" << w << " H=" << h;
    if (!m_initialized || w <= 0 || h <= 0) return;
    glViewport(0, 0, w, h);

    // Aktualizujemy pozycję etykiety pomocy
    if (m_helpLabel) {
        m_helpLabel->setGeometry((width() - 300) / 2, 10, 300, 30);
    }
}

void EditableStreamWidget::paintGL() {
    if (!m_initialized) {
        glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    m_shaderProgram->bind();

    // Ustawiamy uniformy
    m_shaderProgram->setUniformValue("resolution", QVector2D(width(), height()));
    m_shaderProgram->setUniformValue("time", static_cast<float>(m_timeOffset));
    m_shaderProgram->setUniformValue("amplitude", static_cast<float>(m_waveAmplitude));
    m_shaderProgram->setUniformValue("frequency", static_cast<float>(m_waveFrequency));
    m_shaderProgram->setUniformValue("speed", static_cast<float>(m_waveSpeed));
    m_shaderProgram->setUniformValue("glitchIntensity", static_cast<float>(m_glitchIntensity));
    m_shaderProgram->setUniformValue("waveThickness", static_cast<float>(m_waveThickness));
    m_shaderProgram->setUniformValue("editMode", m_isEditing);  // Przekazujemy informację o trybie edycji

    // Bindujemy teksturę fali
    m_waveTexture->bind(0);  // Tekstura na jednostce 0
    m_shaderProgram->setUniformValue("waveTexture", 0);

    // Rysujemy quad
    m_vao.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_vao.release();

    m_waveTexture->release();
    m_shaderProgram->release();
}

void EditableStreamWidget::paintEvent(QPaintEvent* event) {
    QOpenGLWidget::paintEvent(event);  // Wywołujemy oryginalną implementację

    if (!m_initialized) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysujemy punkty kontrolne i połączenia między nimi
    if (m_isEditing) {
        // Linie łączące punkty
        painter.setPen(QPen(QColor(100, 200, 255, 100), 1, Qt::DashLine));
        for (int i = 0; i < m_numControlPoints - 1; ++i) {
            QPoint p1 = glToWindow(m_controlPoints[i]);
            QPoint p2 = glToWindow(m_controlPoints[i + 1]);
            painter.drawLine(p1, p2);
        }

        // Punkty kontrolne
        for (int i = 0; i < m_numControlPoints; ++i) {
            QPoint center = glToWindow(m_controlPoints[i]);
            if (i == m_selectedPointIndex) {
                // Wyróżniony punkt
                painter.setPen(QPen(Qt::white, 2));
                painter.setBrush(QBrush(QColor(0, 255, 255)));
                painter.drawEllipse(center, 8, 8);
            } else {
                // Normalny punkt
                painter.setPen(QPen(Qt::white, 1));
                painter.setBrush(QBrush(QColor(0, 150, 255)));
                painter.drawEllipse(center, 6, 6);
            }
        }
    }
}

void EditableStreamWidget::updateAnimation() {
    if (!m_initialized) return;

    // Aktualizujemy czas tylko jeśli nie jesteśmy w trybie edycji
    if (!m_isEditing) {
        m_timeOffset += 0.02 * m_waveSpeed;
    }

    // Wymuszamy odświeżenie
    update();
}

void EditableStreamWidget::updateEditHelpText() {
    if (m_isEditing) {
        // Jeśli jesteśmy w trybie edycji, pokazujemy/aktualizujemy etykietę
        if (!m_helpTextVisible) {
            m_helpLabel->setVisible(true);
            m_helpTextVisible = true;
        }

        if (m_helpTextOpacity < 255) {
            m_helpTextOpacity = qMin(255, m_helpTextOpacity + 15);
            m_helpLabel->setStyleSheet(QString("color: white; background-color: rgba(0, 0, 0, 180); padding: 5px; border-radius: 3px; opacity: %1;").arg(m_helpTextOpacity / 255.0));
        }
    } else {
        // Jeśli nie jesteśmy w trybie edycji, ukrywamy etykietę
        if (m_helpTextOpacity > 0) {
            m_helpTextOpacity = qMax(0, m_helpTextOpacity - 15);
            m_helpLabel->setStyleSheet(QString("color: white; background-color: rgba(0, 0, 0, 180); padding: 5px; border-radius: 3px; opacity: %1;").arg(m_helpTextOpacity / 255.0));
        } else if (m_helpTextVisible) {
            m_helpLabel->setVisible(false);
            m_helpTextVisible = false;
            m_helpTextTimer->stop();
        }
    }
}

void EditableStreamWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_initialized) return;

    if (event->button() == Qt::LeftButton) {
        QPointF glPos = windowToGL(event->pos());
        int pointIndex = findClosestControlPoint(glPos);

        if (pointIndex != -1) {
            // Wybrano punkt kontrolny
            if (!m_isEditing) {
                // Jeśli wchodzimy w tryb edycji, resetujemy punkty
                m_isEditing = true;
                resetControlPoints();

                // Uruchamiamy timer dla etykiety pomocy
                m_helpTextTimer->start();
                updateEditHelpText();

                // Informujemy, że rozpoczęto edycję
                emit editingStarted();
            }

            // Wybieramy punkt do edycji
            m_selectedPointIndex = pointIndex;
            update();
        }
    }
}

void EditableStreamWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_initialized) return;

    if (m_isEditing && m_selectedPointIndex != -1) {
        // Przesuwamy wybrany punkt
        QPointF glPos = windowToGL(event->pos());

        // Ograniczamy ruch punktu w pionie
        glPos.setY(qBound(-0.9, glPos.y(), 0.9));

        // Ograniczamy ruch w poziomie, aby utrzymać porządek punktów
        if (m_selectedPointIndex > 0 && m_selectedPointIndex < m_numControlPoints - 1) {
            // Punkty środkowe: ograniczone przez sąsiadów
            qreal minX = m_controlPoints[m_selectedPointIndex - 1].x() + 0.01;
            qreal maxX = m_controlPoints[m_selectedPointIndex + 1].x() - 0.01;
            glPos.setX(qBound(minX, glPos.x(), maxX));
        } else if (m_selectedPointIndex == 0) {
            // Pierwszy punkt: nie może się przesunąć w poziomie
            glPos.setX(m_controlPoints[0].x());
        } else if (m_selectedPointIndex == m_numControlPoints - 1) {
            // Ostatni punkt: nie może się przesunąć w poziomie
            glPos.setX(m_controlPoints[m_numControlPoints - 1].x());
        }

        // Aktualizujemy pozycję punktu
        m_controlPoints[m_selectedPointIndex] = glPos;

        // Aktualizujemy teksturę fali
        updateWaveTexture();

        // Odświeżamy widok
        update();
    } else {
        // Sprawdź, czy kursor jest nad punktem (zmień kursor)
        QPointF glPos = windowToGL(event->pos());
        int pointIndex = findClosestControlPoint(glPos);

        if (pointIndex != -1) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void EditableStreamWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_initialized) return;

    if (event->button() == Qt::LeftButton && m_isEditing) {
        // Nie kończymy edycji po zwolnieniu przycisku myszy
        // Edycję kończymy tylko po naciśnięciu ESC
        m_selectedPointIndex = -1;
        update();
    }
}

void EditableStreamWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && m_isEditing) {
        // Kończymy tryb edycji
        m_isEditing = false;
        m_selectedPointIndex = -1;


        // Informujemy, że zakończono edycję
        emit editingFinished();

        // Odświeżamy widok
        update();
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}

void EditableStreamWidget::triggerGlitch(qreal intensity) {
    qDebug() << "Triggering glitch with intensity:" << intensity;
    m_glitchIntensity = qMax(m_glitchIntensity, intensity);
    if (!m_glitchDecayTimer->isActive()) {
        m_glitchDecayTimer->start();
    }
}

void EditableStreamWidget::triggerReceivingAnimation() {
    qDebug() << "Triggering receiving animation";

    // Animujemy parametry fali (dokładnie jak w CommunicationStream)
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
}

void EditableStreamWidget::returnToIdleAnimation() {
    qDebug() << "Returning to idle animation";

    // Animujemy powrót do stanu bezczynności (jak w CommunicationStream)
    QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
    ampAnim->setDuration(1500);
    ampAnim->setStartValue(m_waveAmplitude);
    ampAnim->setEndValue(0.01);
    ampAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
    freqAnim->setDuration(1500);
    freqAnim->setStartValue(m_waveFrequency);
    freqAnim->setEndValue(2.0);
    freqAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
    speedAnim->setDuration(1500);
    speedAnim->setStartValue(m_waveSpeed);
    speedAnim->setEndValue(1.0);
    speedAnim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* thicknessAnim = new QPropertyAnimation(this, "waveThickness");
    thicknessAnim->setDuration(1500);
    thicknessAnim->setStartValue(m_waveThickness);
    thicknessAnim->setEndValue(0.008);
    thicknessAnim->setEasingCurve(QEasingCurve::OutQuad);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(ampAnim);
    group->addAnimation(freqAnim);
    group->addAnimation(speedAnim);
    group->addAnimation(thicknessAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}