#ifndef RADIO_MESSAGE_H
#define RADIO_MESSAGE_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPainterPath>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QRandomGenerator>

class RadioMessage : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    Q_PROPERTY(qreal signalStrength READ signalStrength WRITE setSignalStrength)
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)
    Q_PROPERTY(qreal age READ age WRITE setAge)
    Q_PROPERTY(qreal scanlineOffset READ scanlineOffset WRITE setScanlineOffset)

public:
    enum MessageType {
        Received,
        Transmitted,
        System
    };

    explicit RadioMessage(const QString& message, MessageType type = Received, QWidget* parent = nullptr)
        : QOpenGLWidget(parent), m_type(type), m_glitchIntensity(0.0), m_signalStrength(1.0),
          m_glowIntensity(0.3), m_age(0.0), m_scanlineOffset(0.0), m_initialized(false), m_noiseTexture(nullptr)
    {
        // Podstawowa konfiguracja
        setObjectName("radioMessage");
        setMinimumWidth(200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        // Główny układ
        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(15, 12, 15, 12);
        m_layout->setSpacing(4);

        // Etykieta czasu i informacji o nadawcy
        m_headerLabel = new QLabel(this);
        m_headerLabel->setAlignment(Qt::AlignLeft);
        m_headerLabel->setStyleSheet("border: none; background: transparent; font-family: 'Orbitron', 'Electrolize', 'Consolas';");
        m_layout->addWidget(m_headerLabel);

        // Etykieta treści wiadomości
        m_messageLabel = new QLabel(this);
        m_messageLabel->setAlignment(Qt::AlignLeft);
        m_messageLabel->setWordWrap(true);
        m_messageLabel->setTextFormat(Qt::RichText);
        m_messageLabel->setStyleSheet("border: none; background: transparent; color: #e0e0e0; font-family: 'Orbitron', 'Electrolize', 'Consolas';");
        m_layout->addWidget(m_messageLabel);

        // Pasek siły sygnału - teraz efekt neonowy
        m_signalBar = new QFrame(this);
        m_signalBar->setFixedHeight(2);
        m_layout->addWidget(m_signalBar);

        // Przetwarzamy treść wiadomości
        processMessage(message);

        // Timer dla animacji i efektów
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &RadioMessage::updateEffects);
        m_updateTimer->start(60);

        // Timer dla efektu skanowania
        m_scanlineTimer = new QTimer(this);
        connect(m_scanlineTimer, &QTimer::timeout, this, &RadioMessage::updateScanline);
        m_scanlineTimer->start(16); // 60fps dla płynnego efektu

        // Timer starzenia się wiadomości
        m_ageTimer = new QTimer(this);
        connect(m_ageTimer, &QTimer::timeout, this, &RadioMessage::incrementAge);
        m_ageTimer->start(1000);

        // Zapisujemy czas utworzenia
        m_creationTime = QDateTime::currentDateTime();

        // Zwiększamy szanse na cyfrowe zakłócenia dla świeżych wiadomości
        m_glitchChance = 10.0;

        // Ustaw odpowiedni styl paska sygnału zależnie od typu wiadomości
        updateSignalBarStyle();
    }

    ~RadioMessage() {
        makeCurrent();
        delete m_noiseTexture;
        delete m_shaderProgram;
        doneCurrent();
    }

    // Gettery i settery dla animacji
    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity) {
        m_glitchIntensity = intensity;
        update();
    }

    qreal signalStrength() const { return m_signalStrength; }
    void setSignalStrength(qreal strength) {
        m_signalStrength = qBound(0.0, strength, 1.0);
        m_signalBar->setFixedWidth(width() * m_signalStrength);
        update();
    }

    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity) {
        m_glowIntensity = intensity;
        update();
    }

    qreal age() const { return m_age; }
    void setAge(qreal age) {
        m_age = age;
        // Zmniejszamy szansę na zakłócenia im starsza wiadomość
        m_glitchChance = qMax(0.5, 10.0 - (m_age * 0.4));
        update();
    }

    qreal scanlineOffset() const { return m_scanlineOffset; }
    void setScanlineOffset(qreal offset) {
        m_scanlineOffset = offset;
        update();
    }

    // Metoda uruchamiająca animację pojawienia się
    void startEntryAnimation() {
        // Animacja siły sygnału
        QPropertyAnimation* signalAnim = new QPropertyAnimation(this, "signalStrength");
        signalAnim->setDuration(800);
        signalAnim->setStartValue(0.1);
        signalAnim->setEndValue(0.8 + (QRandomGenerator::global()->bounded(20) / 100.0));
        signalAnim->setEasingCurve(QEasingCurve::OutQuad);

        // Animacja glitcha
        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(1200);
        glitchAnim->setStartValue(0.8);
        glitchAnim->setKeyValueAt(0.4, 0.4);
        glitchAnim->setKeyValueAt(0.7, 0.2);
        glitchAnim->setEndValue(0.0);
        glitchAnim->setEasingCurve(QEasingCurve::OutQuad);

        // Animacja poświaty
        QPropertyAnimation* glowAnim = new QPropertyAnimation(this, "glowIntensity");
        glowAnim->setDuration(1500);
        glowAnim->setStartValue(0.8);
        glowAnim->setKeyValueAt(0.3, 0.5);
        glowAnim->setKeyValueAt(0.6, 0.7);
        glowAnim->setEndValue(0.3);
        glowAnim->setEasingCurve(QEasingCurve::InOutSine);

        // Grupa animacji
        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(signalAnim);
        group->addAnimation(glitchAnim);
        group->addAnimation(glowAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // Metoda symulująca zakłócenia cyfrowe
    void simulateGlitch(qreal severity) {
        // Przeskalujemy siłę zakłóceń w zależności od wieku wiadomości
        severity = severity * qMax(0.2, 1.0 - (m_age * 0.1));

        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(300 + QRandomGenerator::global()->bounded(400));
        glitchAnim->setStartValue(m_glitchIntensity);
        glitchAnim->setKeyValueAt(0.3, severity);
        glitchAnim->setKeyValueAt(0.7, severity * 0.6);
        glitchAnim->setEndValue(0.0);

        QPropertyAnimation* signalAnim = new QPropertyAnimation(this, "signalStrength");
        signalAnim->setDuration(300 + QRandomGenerator::global()->bounded(400));
        signalAnim->setStartValue(m_signalStrength);
        signalAnim->setKeyValueAt(0.2, m_signalStrength * 0.7);
        signalAnim->setEndValue(m_signalStrength);

        QPropertyAnimation* glowAnim = new QPropertyAnimation(this, "glowIntensity");
        glowAnim->setDuration(400);
        glowAnim->setStartValue(m_glowIntensity);
        glowAnim->setKeyValueAt(0.5, m_glowIntensity + 0.4);
        glowAnim->setEndValue(m_glowIntensity);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(glitchAnim);
        group->addAnimation(signalAnim);
        group->addAnimation(glowAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QSize sizeHint() const override {
        QSize baseSize = QOpenGLWidget::sizeHint();
        baseSize.setHeight(m_messageLabel->sizeHint().height() +
                         m_headerLabel->sizeHint().height() + 40);
        return baseSize;
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Inicjalizujemy shader program
        m_shaderProgram = new QOpenGLShaderProgram();

        // Vertex shader
        m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
            "attribute highp vec4 vertices;\n"
            "attribute mediump vec2 texCoord;\n"
            "varying mediump vec2 texc;\n"
            "void main(void) {\n"
            "    gl_Position = vertices;\n"
            "    texc = texCoord;\n"
            "}\n");

        // Fragment shader z efektami cyberpunkowymi (glitch, scanline, glow)
        m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
            "uniform sampler2D texture;\n"
            "uniform mediump float glitchIntensity;\n"
            "uniform mediump float glowIntensity;\n"
            "uniform mediump float scanlineOffset;\n"
            "varying mediump vec2 texc;\n"
            "void main(void) {\n"
            "    mediump vec2 uv = texc;\n"
            "    mediump float time = scanlineOffset * 10.0;\n"

            // Glitch effect - RGB shift
            "    mediump float rgbShift = glitchIntensity * 0.01;\n"
            "    if (glitchIntensity > 0.2) {\n"
            "        mediump float noise = fract(sin(dot(texc, vec2(12.9898, 78.233) * time)) * 43758.5453);\n"
            "        if (noise > 0.9) {\n"
            "            rgbShift = rgbShift * 2.0;\n"
            "        }\n"
            "    }\n"
            "    mediump vec4 r = texture2D(texture, uv + vec2(rgbShift, 0.0));\n"
            "    mediump vec4 g = texture2D(texture, uv);\n"
            "    mediump vec4 b = texture2D(texture, uv - vec2(rgbShift, 0.0));\n"
            "    mediump vec4 texColor = vec4(r.r, g.g, b.b, g.a);\n"

            // Digital scanning lines
            "    mediump float scanline = mod(texc.y * 30.0 + scanlineOffset * 5.0, 1.0) < 0.5 ? 1.0 : 0.95;\n"
            "    texColor.rgb *= scanline;\n"

            // Holographic grid effect
            "    mediump float grid = (mod(texc.x * 40.0, 1.0) < 0.06 || mod(texc.y * 40.0, 1.0) < 0.06) ? 1.0 + glowIntensity * 0.4 : 1.0;\n"
            "    texColor.rgb *= grid;\n"

            // Random blocks glitch effect
            "    if (glitchIntensity > 0.5) {\n"
            "        mediump float blockNoise = fract(sin(dot(floor(texc * 10.0) / 10.0, vec2(12.9898, 78.233) * time)) * 43758.5453);\n"
            "        if (blockNoise > 0.93) {\n"
            "            texColor.rgb = mix(texColor.rgb, vec3(1.0, 0.0, 0.7) * texColor.rgb, glitchIntensity * 0.5);\n"
            "        }\n"
            "    }\n"

            // Glow effect
            "    mediump float edge = max(0.0, 1.0 - 2.0 * distance(texc, vec2(0.5)));\n"
            "    mediump vec3 glow = mix(texColor.rgb, vec3(0.7, 0.2, 1.0), glowIntensity * 0.2 * edge);\n"
            "    texColor.rgb = mix(texColor.rgb, glow, glowIntensity * 0.5);\n"

            "    gl_FragColor = texColor;\n"
            "}\n");

        m_shaderProgram->bindAttributeLocation("vertices", 0);
        m_shaderProgram->bindAttributeLocation("texCoord", 1);
        m_shaderProgram->link();

        // Generujemy teksturę szumu
        generateNoiseTexture();

        m_initialized = true;
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        m_signalBar->setFixedWidth(w * m_signalStrength);
    }

    void paintGL() override {
        if (!m_initialized) return;

        // Czyszczenie bufora
        glClear(GL_COLOR_BUFFER_BIT);

        // Używamy bufora ekranu do renderowania zwykłych widgetów
        QPainter painter(this);
        painter.beginNativePainting();

        // Włączamy shader dla efektów cyberpunkowych
        m_shaderProgram->bind();
        m_shaderProgram->setUniformValue("glitchIntensity", static_cast<float>(m_glitchIntensity));
        m_shaderProgram->setUniformValue("glowIntensity", static_cast<float>(m_glowIntensity));
        m_shaderProgram->setUniformValue("scanlineOffset", static_cast<float>(m_scanlineOffset));

        // Bindujemy teksturę szumu
        glActiveTexture(GL_TEXTURE0);
        m_noiseTexture->bind();
        m_shaderProgram->setUniformValue("texture", 0);

        // Renderujemy prostokąt z efektem
        static const GLfloat vertices[] = {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f
        };

        static const GLfloat texCoords[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        m_noiseTexture->release();
        m_shaderProgram->release();

        painter.endNativePainting();

        // Dodatkowe rysowanie interfejsu wykorzystując zwykły QPainter
        painter.setRenderHint(QPainter::Antialiasing);

        // Wybieramy kolory w stylu cyberpunk
        QColor bgColor, borderColor, glowColor;
        QLinearGradient gradient(0, 0, width(), height());

        switch (m_type) {
            case Transmitted:
                // Neonowy zielono-turkusowy dla wysyłanych
                bgColor = QColor(10, 25, 25, 220);
                gradient.setColorAt(0, QColor(10, 30, 25, 220));
                gradient.setColorAt(1, QColor(5, 20, 25, 220));
                borderColor = QColor(0, 255, 200);
                glowColor = QColor(0, 255, 200, 70);
                break;
            case Received:
                // Różowo-fioletowy dla przychodzących
                bgColor = QColor(25, 10, 25, 220);
                gradient.setColorAt(0, QColor(30, 10, 30, 220));
                gradient.setColorAt(1, QColor(20, 5, 25, 220));
                borderColor = QColor(220, 50, 255);
                glowColor = QColor(220, 50, 255, 70);
                break;
            case System:
                // Żółto-pomarańczowy dla systemowych
                bgColor = QColor(25, 20, 10, 220);
                gradient.setColorAt(0, QColor(30, 25, 10, 220));
                gradient.setColorAt(1, QColor(25, 15, 5, 220));
                borderColor = QColor(255, 200, 50);
                glowColor = QColor(255, 200, 50, 70);
                break;
        }

        // Rysujemy tło z gradientem
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);

        // Geometryczna forma - ścięty heksagon zamiast zaokrąglonych rogów
        QPainterPath path;
        int margin = 4; // margines do ścięć

        path.moveTo(margin, 0);
        path.lineTo(width() - margin, 0);
        path.lineTo(width(), margin);
        path.lineTo(width(), height() - margin);
        path.lineTo(width() - margin, height());
        path.lineTo(margin, height());
        path.lineTo(0, height() - margin);
        path.lineTo(0, margin);
        path.closeSubpath();

        painter.drawPath(path);

        // Rysujemy poświatę neonu dla obramowania
        if (m_glowIntensity > 0.1) {
            QPen glowPen(glowColor);
            glowPen.setWidth(6);
            painter.setPen(glowPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }

        // Rysujemy neonowe obramowanie
        QPen borderPen(borderColor);
        borderPen.setWidth(1);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Dodatkowe linie geometryczne (opcjonalnie)
        painter.setPen(QPen(borderColor, 1, Qt::SolidLine));
        painter.drawLine(margin, 15, width() - margin, 15);
    }

private slots:
    void updateEffects() {
        // Okresowo dodajemy niewielkie losowe glitche
        if (QRandomGenerator::global()->bounded(100) < m_glitchChance) {
            qreal severity = (QRandomGenerator::global()->bounded(40) / 100.0) * qMax(0.2, 1.0 - (m_age * 0.1));
            simulateGlitch(severity);
        }

        // Subtelna pulsacja poświaty
        setGlowIntensity(m_glowIntensity + sin(QDateTime::currentMSecsSinceEpoch() * 0.001) * 0.01);

        // Odświeżamy widok tylko gdy jest to konieczne
        if (m_glitchIntensity > 0.01) {
            update();
        }
    }

    void updateScanline() {
        // Aktualizujemy offset dla efektu skanowania
        setScanlineOffset(fmod(m_scanlineOffset + 0.01, 10.0));
    }

    void incrementAge() {
        qreal newAge = m_age + 0.1; // Zwiększamy wiek o 0.1 co sekundę
        setAge(newAge);
    }

private:
    void processMessage(const QString& message) {
        // Rozdzielamy nagłówek i treść wiadomości
        int contentStart = message.indexOf("<span style=\"color:#ffffff;\">");

        if (contentStart > 0) {
            // Wyodrębnij nagłówek (timestamp i nadawca)
            QString header = message.left(contentStart);

            // Zmień styl nagłówka na bardziej "cyberpunkowy"
            header.replace("color:#60ff8a;", "color:#00ffcc; font-family: 'Orbitron', 'Electrolize', 'Consolas';");
            header.replace("color:#85c4ff;", "color:#cc55ff; font-family: 'Orbitron', 'Electrolize', 'Consolas';");

            m_headerLabel->setText(header);

            // Wyodrębnij treść
            QString content = message.mid(contentStart);
            content.replace("color:#ffffff;", "color:#e0e0ff; font-family: 'Orbitron', 'Electrolize', 'Consolas';");

            m_messageLabel->setText(content);
        } else {
            // Wiadomość systemowa lub specjalny format
            m_headerLabel->setText(QDateTime::currentDateTime().toString("[HH:mm:ss] SYSTEM"));
            m_messageLabel->setText(message);
        }
    }

    void updateSignalBarStyle() {
        QString barColor;
        switch (m_type) {
            case Transmitted:
                barColor = "#00ffcc";
                break;
            case Received:
                barColor = "#cc55ff";
                break;
            case System:
                barColor = "#ffcc33";
                break;
        }

        // Neonowy świecący pasek z gradientem
        m_signalBar->setStyleSheet(QString(
            "QFrame {"
            "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            "       stop:0 %1, stop:0.5 %2, stop:1 %1);"
            "   border: none;"
            "}")
            .arg(barColor)
            .arg(barColor)
        );
    }

    void generateNoiseTexture() {
        // Tworzymy teksturę cyfrowego szumu o wyższej rozdzielczości
        const int texSize = 512;
        QImage noiseImage(texSize, texSize, QImage::Format_RGBA8888);

        // Generujemy bardziej uporządkowany szum cyfrowy
        for (int y = 0; y < texSize; ++y) {
            for (int x = 0; x < texSize; ++x) {
                int noise = QRandomGenerator::global()->bounded(256);
                // Dodajemy więcej struktury (linie, siatki) do szumu
                if ((x % 8 == 0) || (y % 8 == 0)) {
                    noise = (noise + 128) % 256;
                }
                if ((x % 64 == 0) || (y % 64 == 0)) {
                    noise = 200 + QRandomGenerator::global()->bounded(56);
                }
                noiseImage.setPixelColor(x, y, QColor(noise, noise, noise, 255));
            }
        }

        // Tworzymy i konfigurujemy teksturę OpenGL
        m_noiseTexture = new QOpenGLTexture(noiseImage);
        m_noiseTexture->setMinificationFilter(QOpenGLTexture::Linear);
        m_noiseTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_noiseTexture->setWrapMode(QOpenGLTexture::Repeat);
    }

    MessageType m_type;
    QLabel* m_headerLabel;
    QLabel* m_messageLabel;
    QFrame* m_signalBar;
    QVBoxLayout* m_layout;
    QTimer* m_updateTimer;
    QTimer* m_scanlineTimer;
    QTimer* m_ageTimer;
    QDateTime m_creationTime;

    qreal m_glitchIntensity;
    qreal m_signalStrength;
    qreal m_glowIntensity;
    qreal m_age;
    qreal m_glitchChance;
    qreal m_scanlineOffset;

    // OpenGL
    bool m_initialized;
    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLTexture* m_noiseTexture;
};

#endif // RADIO_MESSAGE_H