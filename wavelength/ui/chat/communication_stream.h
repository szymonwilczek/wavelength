#ifndef COMMUNICATION_STREAM_H
#define COMMUNICATION_STREAM_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QSoundEffect>
#include <QQueue>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

// Klasa reprezentująca pojedynczą wiadomość w strumieniu
class StreamMessage : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    enum MessageType {
        Received,
        Transmitted,
        System
    };

    StreamMessage(const QString& content, const QString& sender, MessageType type, QWidget* parent = nullptr)
        : QWidget(parent), m_content(content), m_sender(sender), m_type(type),
          m_opacity(0.0), m_glowIntensity(0.8), m_isRead(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setMinimumWidth(400);
        setMaximumWidth(600);
        setMinimumHeight(120);

        // Efekt przeźroczystości
        QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);
        opacity->setOpacity(0.0);
        setGraphicsEffect(opacity);

        // Inicjalizacja przycisków nawigacyjnych
        m_nextButton = new QPushButton(">", this);
        m_nextButton->setFixedSize(40, 40);
        m_nextButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(0, 200, 255, 0.3);"
            "  color: #00ffff;"
            "  border: 1px solid #00ccff;"
            "  border-radius: 20px;"
            "  font-weight: bold;"
            "  font-size: 16px;"
            "}"
            "QPushButton:hover { background-color: rgba(0, 200, 255, 0.5); }"
            "QPushButton:pressed { background-color: rgba(0, 200, 255, 0.7); }");
        m_nextButton->hide();

        m_prevButton = new QPushButton("<", this);
        m_prevButton->setFixedSize(40, 40);
        m_prevButton->setStyleSheet(m_nextButton->styleSheet());
        m_prevButton->hide();

        m_markReadButton = new QPushButton("✓", this);
        m_markReadButton->setFixedSize(40, 40);
        m_markReadButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(0, 255, 150, 0.3);"
            "  color: #00ffaa;"
            "  border: 1px solid #00ffcc;"
            "  border-radius: 20px;"
            "  font-weight: bold;"
            "  font-size: 16px;"
            "}"
            "QPushButton:hover { background-color: rgba(0, 255, 150, 0.5); }"
            "QPushButton:pressed { background-color: rgba(0, 255, 150, 0.7); }");
        m_markReadButton->hide();

        connect(m_markReadButton, &QPushButton::clicked, this, &StreamMessage::markAsRead);

        // Timer dla subtelnych animacji
        m_animationTimer = new QTimer(this);
        connect(m_animationTimer, &QTimer::timeout, this, &StreamMessage::updateAnimation);
        m_animationTimer->start(50);

        // Automatyczne dopasowanie wysokości
        updateLayout();
    }

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity) {
        m_opacity = opacity;
        if (QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
            effect->setOpacity(opacity);
        }
    }

    qreal glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(qreal intensity) {
        m_glowIntensity = intensity;
        update();
    }

    bool isRead() const { return m_isRead; }

    void fadeIn() {
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "opacity");
        opacityAnim->setDuration(800);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        QPropertyAnimation* glowAnim = new QPropertyAnimation(this, "glowIntensity");
        glowAnim->setDuration(1500);
        glowAnim->setStartValue(1.0);
        glowAnim->setKeyValueAt(0.4, 0.7);
        glowAnim->setKeyValueAt(0.7, 0.9);
        glowAnim->setEndValue(0.5);
        glowAnim->setEasingCurve(QEasingCurve::InOutSine);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(opacityAnim);
        group->addAnimation(glowAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void fadeOut() {
        QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "opacity");
        opacityAnim->setDuration(500);
        opacityAnim->setStartValue(opacity());
        opacityAnim->setEndValue(0.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        connect(opacityAnim, &QPropertyAnimation::finished, this, [this]() {
            hide();
            emit hidden();
        });

        opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void showNavigationButtons(bool hasPrev, bool hasNext) {
        // Pozycjonowanie przycisków nawigacyjnych
        m_nextButton->setVisible(hasNext);
        m_prevButton->setVisible(hasPrev);
        m_markReadButton->setVisible(true);

        m_nextButton->move(width() - m_nextButton->width() - 10, height() / 2 - m_nextButton->height() / 2);
        m_prevButton->move(10, height() / 2 - m_prevButton->height() / 2);
        m_markReadButton->move(width() - m_markReadButton->width() - 10, 10);

        // Frontowanie przycisków
        m_nextButton->raise();
        m_prevButton->raise();
        m_markReadButton->raise();
    }

    QPushButton* nextButton() const { return m_nextButton; }
    QPushButton* prevButton() const { return m_prevButton; }

    QSize sizeHint() const override {
        return QSize(500, 150);
    }

signals:
    void messageRead();
    void hidden();

public slots:
    void markAsRead() {
        if (!m_isRead) {
            m_isRead = true;

            // Animacja zanikania poświaty i stopniowej przezroczystości
            QPropertyAnimation* glowAnim = new QPropertyAnimation(this, "glowIntensity");
            glowAnim->setDuration(1000);
            glowAnim->setStartValue(m_glowIntensity);
            glowAnim->setEndValue(0.2);
            glowAnim->setEasingCurve(QEasingCurve::OutQuad);

            QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "opacity");
            opacityAnim->setDuration(1000);
            opacityAnim->setStartValue(m_opacity);
            opacityAnim->setEndValue(0.7);
            opacityAnim->setEasingCurve(QEasingCurve::OutQuad);

            QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
            group->addAnimation(glowAnim);
            group->addAnimation(opacityAnim);
            group->start(QAbstractAnimation::DeleteWhenStopped);

            emit messageRead();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Wybieramy kolory w stylu cyberpunk zależnie od typu wiadomości
        QColor bgColor, borderColor, glowColor, textColor;

        switch (m_type) {
            case Transmitted:
                // Neonowy niebieski dla wysyłanych
                bgColor = QColor(0, 20, 40, 180);
                borderColor = QColor(0, 200, 255);
                glowColor = QColor(0, 150, 255, 80);
                textColor = QColor(0, 220, 255);
                break;
            case Received:
                // Różowo-fioletowy dla przychodzących
                bgColor = QColor(30, 0, 30, 180);
                borderColor = QColor(220, 50, 255);
                glowColor = QColor(180, 0, 255, 80);
                textColor = QColor(240, 150, 255);
                break;
            case System:
                // Żółto-pomarańczowy dla systemowych
                bgColor = QColor(40, 25, 0, 180);
                borderColor = QColor(255, 180, 0);
                glowColor = QColor(255, 150, 0, 80);
                textColor = QColor(255, 200, 0);
                break;
        }

        // Tworzymy ściętą formę geometryczną (cyberpunk style)
        QPainterPath path;
        int clipSize = 20; // rozmiar ścięcia rogu

        path.moveTo(clipSize, 0);
        path.lineTo(width() - clipSize, 0);
        path.lineTo(width(), clipSize);
        path.lineTo(width(), height() - clipSize);
        path.lineTo(width() - clipSize, height());
        path.lineTo(clipSize, height());
        path.lineTo(0, height() - clipSize);
        path.lineTo(0, clipSize);
        path.closeSubpath();

        // Tło z gradientem
        QLinearGradient bgGradient(0, 0, width(), height());
        bgGradient.setColorAt(0, bgColor.lighter(110));
        bgGradient.setColorAt(1, bgColor);

        painter.setPen(Qt::NoPen);
        painter.setBrush(bgGradient);
        painter.drawPath(path);

        // Poświata neonu
        if (m_glowIntensity > 0.1) {
            painter.setPen(QPen(glowColor, 6 + m_glowIntensity * 6));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }

        // Neonowe obramowanie
        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Linie dekoracyjne
        painter.setPen(QPen(borderColor.lighter(120), 1, Qt::SolidLine));
        painter.drawLine(clipSize, 30, width() - clipSize, 30);

        // Tekst nagłówka (nadawca)
        painter.setPen(QPen(textColor, 1));
        painter.setFont(QFont("Consolas", 10, QFont::Bold));
        painter.drawText(QRect(clipSize + 5, 5, width() - 2*clipSize - 10, 22),
                         Qt::AlignLeft | Qt::AlignVCenter, m_sender);

        // Tekst wiadomości
        painter.setPen(QPen(Qt::white, 1));
        painter.setFont(QFont("Segoe UI", 10));
        painter.drawText(QRect(clipSize + 5, 35, width() - 2*clipSize - 10, height() - 45),
                         Qt::AlignLeft | Qt::AlignTop, m_content);
    }

    void resizeEvent(QResizeEvent* event) override {
        updateLayout();
        QWidget::resizeEvent(event);
    }

private slots:
    void updateAnimation() {
        // Subtelna pulsacja poświaty
        qreal pulse = 0.05 * sin(QDateTime::currentMSecsSinceEpoch() * 0.002);
        setGlowIntensity(m_glowIntensity + pulse * (!m_isRead ? 1.0 : 0.3));
    }

private:
    void updateLayout() {
        // Pozycjonowanie przycisków nawigacyjnych
        if (m_nextButton->isVisible()) {
            m_nextButton->move(width() - m_nextButton->width() - 10, height() / 2 - m_nextButton->height() / 2);
        }
        if (m_prevButton->isVisible()) {
            m_prevButton->move(10, height() / 2 - m_prevButton->height() / 2);
        }
        if (m_markReadButton->isVisible()) {
            m_markReadButton->move(width() - m_markReadButton->width() - 10, 10);
        }
    }

    QString m_content;
    QString m_sender;
    MessageType m_type;
    qreal m_opacity;
    qreal m_glowIntensity;
    bool m_isRead;

    QPushButton* m_nextButton;
    QPushButton* m_prevButton;
    QPushButton* m_markReadButton;
    QTimer* m_animationTimer;
};

// Główny widget strumienia komunikacji - zamieniamy OpenGL na standardowy QWidget
class CommunicationStream : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude)
    Q_PROPERTY(qreal waveFrequency READ waveFrequency WRITE setWaveFrequency)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)

public:
    enum StreamState {
        Idle,        // Prosta linia z okazjonalnymi zakłóceniami
        Receiving,   // Aktywna animacja podczas otrzymywania wiadomości
        Displaying   // Wyświetlanie wiadomości
    };

    explicit CommunicationStream(QWidget* parent = nullptr)
        : QWidget(parent),
          m_waveAmplitude(0.05), m_waveFrequency(5.0), m_waveSpeed(1.0),
          m_glitchIntensity(0.0), m_state(Idle), m_currentMessageIndex(-1),
          m_timeOffset(0.0)
    {
        setMinimumSize(600, 200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // Timer animacji
        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, &CommunicationStream::updateAnimation);
        m_animTimer->start(16); // ~60fps

        // Timer losowych zakłóceń w trybie Idle
        m_glitchTimer = new QTimer(this);
        connect(m_glitchTimer, &QTimer::timeout, this, &CommunicationStream::triggerRandomGlitch);
        m_glitchTimer->start(5000 + QRandomGenerator::global()->bounded(5000));

        // Etykieta nazwy strumienia
        m_streamNameLabel = new QLabel("COMMUNICATION STREAM", this);
        m_streamNameLabel->setStyleSheet(
            "QLabel {"
            "  color: #00ccff;"
            "  background-color: rgba(0, 15, 30, 180);"
            "  border: 1px solid #00aaff;"
            "  border-radius: 0px;"
            "  padding: 3px 10px;"
            "  font-family: 'Consolas', sans-serif;"
            "  font-weight: bold;"
            "  font-size: 12px;"
            "}"
        );
        m_streamNameLabel->setAlignment(Qt::AlignCenter);
        m_streamNameLabel->adjustSize();
        m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);
    }

    // Settery i gettery dla animacji fali
    qreal waveAmplitude() const { return m_waveAmplitude; }
    void setWaveAmplitude(qreal amplitude) {
        m_waveAmplitude = amplitude;
        update();
    }

    qreal waveFrequency() const { return m_waveFrequency; }
    void setWaveFrequency(qreal frequency) {
        m_waveFrequency = frequency;
        update();
    }

    qreal waveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(qreal speed) {
        m_waveSpeed = speed;
        update();
    }

    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity) {
        m_glitchIntensity = intensity;
        update();
    }

    void setStreamName(const QString& name) {
        m_streamNameLabel->setText(name);
        m_streamNameLabel->adjustSize();
        m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);
    }

    void addMessage(const QString& content, const QString& sender, StreamMessage::MessageType type) {
        // Najpierw rozpoczynamy animację odbioru
        startReceivingAnimation();

        // Tworzymy nową wiadomość, ale nie wyświetlamy jej od razu
        StreamMessage* message = new StreamMessage(content, sender, type, this);
        message->hide();
        m_messages.enqueue(message);

        // Jeśli nie ma wyświetlanej wiadomości, pokaż tę po zakończeniu animacji odbioru
        if (m_currentMessageIndex == -1) {
            QTimer::singleShot(1200, this, &CommunicationStream::showNextMessage);
        }

        // Podłączamy sygnały do nawigacji między wiadomościami
        connect(message, &StreamMessage::messageRead, this, &CommunicationStream::onMessageRead);
        connect(message->nextButton(), &QPushButton::clicked, this, &CommunicationStream::showNextMessage);
        connect(message->prevButton(), &QPushButton::clicked, this, &CommunicationStream::showPreviousMessage);
    }

    void clearMessages() {
        // Ukrywamy i usuwamy wszystkie wiadomości
        while (!m_messages.isEmpty()) {
            StreamMessage* message = m_messages.dequeue();
            message->deleteLater();
        }

        m_currentMessageIndex = -1;
        returnToIdleAnimation();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Tło - ciemne z subtelnymi liniami siatki
        painter.fillRect(rect(), QColor(10, 12, 18));

        // Rysujemy linie siatki w tle
        painter.setPen(QPen(QColor(30, 40, 70, 40), 1, Qt::SolidLine));
        int gridSize = 20;
        for (int x = 0; x < width(); x += gridSize) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0; y < height(); y += gridSize) {
            painter.drawLine(0, y, width(), y);
        }

        // Główna linia sygnału (fala)
        int centerY = height() / 2;
        int margin = 20;

        QPainterPath wavePath;
        QPainterPath glowPath;

        // Generujemy ścieżkę fali
        wavePath.moveTo(margin, centerY);

        for (int x = margin; x < width() - margin; x++) {
            float phase = (x - margin) / (float)(width() - 2 * margin);
            float time = m_timeOffset;

            // Obliczamy wysokość fali
            float waveY = centerY;

            // Dodajemy falę sinusoidalną o zmiennej amplitudzie
            waveY += sin(phase * m_waveFrequency * 10.0 + time * m_waveSpeed) * m_waveAmplitude * 100;

            // Dodajemy zakłócenia, jeśli są aktywne
            if (m_glitchIntensity > 0.01) {
                // Dodajemy losowe zakłócenia w określonych miejscach
                float noise = QRandomGenerator::global()->bounded(100) / 100.0;
                if (noise > 0.7) {
                    waveY += sin(phase * m_waveFrequency * 30.0 + time * m_waveSpeed * 2.0) *
                              m_glitchIntensity * 50 * noise;
                }
            }

            wavePath.lineTo(x, waveY);
            glowPath.lineTo(x, waveY);
        }

        wavePath.lineTo(width() - margin, centerY);
        glowPath.lineTo(width() - margin, centerY);

        // Rysujemy poświatę fali
        QPen glowPen(QColor(0, 150, 255, 50), 8);
        painter.setPen(glowPen);
        painter.drawPath(glowPath);

        // Używamy bardziej intensywnej poświaty dla zakłóceń
        if (m_glitchIntensity > 0.1) {
            QPen strongGlowPen(QColor(50, 200, 255, 80), 12);
            painter.setPen(strongGlowPen);
            painter.drawPath(glowPath);
        }

        // Rysujemy główną linię
        QPen wavePen(QColor(0, 200, 255), 2);
        painter.setPen(wavePen);
        painter.drawPath(wavePath);

        // Dodajemy małe markery na linii (futurystyczne akcenty)
        painter.setPen(QPen(QColor(0, 230, 255), 4, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 5; i++) {
            int x = margin + (i + 1) * (width() - 2 * margin) / 6;
            int y = centerY + sin((x - margin) / (float)(width() - 2 * margin) *
                       m_waveFrequency * 10.0 + m_timeOffset * m_waveSpeed) * m_waveAmplitude * 100;
            painter.drawPoint(x, y);
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);

        // Aktualizujemy pozycję etykiety nazwy strumienia
        m_streamNameLabel->move((width() - m_streamNameLabel->width()) / 2, 10);

        // Aktualizujemy pozycję aktualnie wyświetlanej wiadomości
        updateMessagePosition();
    }

    void keyPressEvent(QKeyEvent* event) override {
        // Obsługa nawigacji klawiaturą
        if (event->key() == Qt::Key_Right) {
            showNextMessage();
        } else if (event->key() == Qt::Key_Left) {
            showPreviousMessage();
        } else if (event->key() == Qt::Key_Space) {
            if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.count()) {
                m_messages[m_currentMessageIndex]->markAsRead();
            }
        } else {
            QWidget::keyPressEvent(event);
        }
    }

private slots:
    void updateAnimation() {
        // Aktualizujemy czas animacji
        m_timeOffset += 0.02 * m_waveSpeed;

        // Stopniowo zmniejszamy intensywność zakłóceń
        if (m_glitchIntensity > 0.0) {
            m_glitchIntensity = qMax(0.0, m_glitchIntensity - 0.005);
        }

        update();
    }

    void triggerRandomGlitch() {
        // Wywołujemy losowe zakłócenia tylko w trybie bezczynności
        if (m_state == Idle) {
            startGlitchAnimation(0.1 + QRandomGenerator::global()->bounded(10) / 100.0);
        }

        // Ustawiamy kolejny losowy interwał
        m_glitchTimer->setInterval(4000 + QRandomGenerator::global()->bounded(8000));
    }

    void startReceivingAnimation() {
        // Zmieniamy stan
        m_state = Receiving;

        // Animujemy parametry fali
        QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
        ampAnim->setDuration(1000);
        ampAnim->setStartValue(m_waveAmplitude);
        ampAnim->setEndValue(0.15);
        ampAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
        freqAnim->setDuration(1000);
        freqAnim->setStartValue(m_waveFrequency);
        freqAnim->setEndValue(12.0);
        freqAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
        speedAnim->setDuration(1000);
        speedAnim->setStartValue(m_waveSpeed);
        speedAnim->setEndValue(3.0);
        speedAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(1000);
        glitchAnim->setStartValue(0.0);
        glitchAnim->setKeyValueAt(0.3, 0.4);
        glitchAnim->setKeyValueAt(0.6, 0.2);
        glitchAnim->setEndValue(0.1);
        glitchAnim->setEasingCurve(QEasingCurve::OutQuad);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(ampAnim);
        group->addAnimation(freqAnim);
        group->addAnimation(speedAnim);
        group->addAnimation(glitchAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);

        // Zmieniamy stan na wyświetlanie po zakończeniu animacji
        connect(group, &QParallelAnimationGroup::finished, this, [this]() {
            m_state = Displaying;
        });
    }

    void returnToIdleAnimation() {
        // Zmieniamy stan
        m_state = Idle;

        // Animujemy powrót do stanu bezczynności
        QPropertyAnimation* ampAnim = new QPropertyAnimation(this, "waveAmplitude");
        ampAnim->setDuration(1500);
        ampAnim->setStartValue(m_waveAmplitude);
        ampAnim->setEndValue(0.05);
        ampAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* freqAnim = new QPropertyAnimation(this, "waveFrequency");
        freqAnim->setDuration(1500);
        freqAnim->setStartValue(m_waveFrequency);
        freqAnim->setEndValue(5.0);
        freqAnim->setEasingCurve(QEasingCurve::OutQuad);

        QPropertyAnimation* speedAnim = new QPropertyAnimation(this, "waveSpeed");
        speedAnim->setDuration(1500);
        speedAnim->setStartValue(m_waveSpeed);
        speedAnim->setEndValue(1.0);
        speedAnim->setEasingCurve(QEasingCurve::OutQuad);

        QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
        group->addAnimation(ampAnim);
        group->addAnimation(freqAnim);
        group->addAnimation(speedAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startGlitchAnimation(qreal intensity) {
        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(800);
        glitchAnim->setStartValue(m_glitchIntensity);
        glitchAnim->setKeyValueAt(0.2, intensity);
        glitchAnim->setKeyValueAt(0.7, intensity * 0.3);
        glitchAnim->setEndValue(0.0);
        glitchAnim->setEasingCurve(QEasingCurve::OutQuad);
        glitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void showNextMessage() {
        if (m_messages.isEmpty()) return;

        // Ukrywamy aktualnie wyświetlaną wiadomość
        if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.count()) {
            m_messages[m_currentMessageIndex]->fadeOut();
        }

        // Przechodzimy do następnej wiadomości
        m_currentMessageIndex++;
        if (m_currentMessageIndex >= m_messages.count()) {
            m_currentMessageIndex = 0;
        }

        // Wyświetlamy nową wiadomość
        StreamMessage* message = m_messages[m_currentMessageIndex];
        message->show();
        updateMessagePosition();
        message->fadeIn();

        // Aktualizujemy przyciski nawigacyjne
        bool hasNext = m_messages.count() > 1;
        bool hasPrev = m_messages.count() > 1;
        message->showNavigationButtons(hasPrev, hasNext);
    }

    void showPreviousMessage() {
        if (m_messages.isEmpty()) return;

        // Ukrywamy aktualnie wyświetlaną wiadomość
        if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.count()) {
            m_messages[m_currentMessageIndex]->fadeOut();
        }

        // Przechodzimy do poprzedniej wiadomości
        m_currentMessageIndex--;
        if (m_currentMessageIndex < 0) {
            m_currentMessageIndex = m_messages.count() - 1;
        }

        // Wyświetlamy nową wiadomość
        StreamMessage* message = m_messages[m_currentMessageIndex];
        message->show();
        updateMessagePosition();
        message->fadeIn();

        // Aktualizujemy przyciski nawigacyjne
        bool hasNext = m_messages.count() > 1;
        bool hasPrev = m_messages.count() > 1;
        message->showNavigationButtons(hasPrev, hasNext);
    }

    void onMessageRead() {
        // Jeżeli wszystkie wiadomości zostały przeczytane, powróć do stanu bezczynności
        bool allRead = true;
        for (const auto& message : m_messages) {
            if (!message->isRead()) {
                allRead = false;
                break;
            }
        }

        if (allRead) {
            // Oznacz wiadomości jako przeczytane po krótkim czasie
            QTimer::singleShot(2000, this, [this]() {
                // Ukrywamy aktualnie wyświetlaną wiadomość
                if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.count()) {
                    m_messages[m_currentMessageIndex]->fadeOut();
                }

                // Usuwamy wszystkie wiadomości z kolejki
                clearMessages();
            });
        }
    }

private:
    void updateMessagePosition() {
        if (m_currentMessageIndex >= 0 && m_currentMessageIndex < m_messages.count()) {
            StreamMessage* message = m_messages[m_currentMessageIndex];
            // Centrujemy wiadomość na fali (w środku ekranu)
            message->move((width() - message->width()) / 2, (height() - message->height()) / 2);
        }
    }

    // Parametry animacji fali
    qreal m_waveAmplitude;
    qreal m_waveFrequency;
    qreal m_waveSpeed;
    qreal m_glitchIntensity;

    StreamState m_state;
    QQueue<StreamMessage*> m_messages;
    int m_currentMessageIndex;

    QLabel* m_streamNameLabel;
    qreal m_timeOffset;

    QTimer* m_animTimer;
    QTimer* m_glitchTimer;
};

#endif // COMMUNICATION_STREAM_H