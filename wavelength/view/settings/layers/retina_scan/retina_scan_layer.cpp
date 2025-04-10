#include "retina_scan_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

RetinaScanLayer::RetinaScanLayer(QWidget *parent) 
    : SecurityLayer(parent), m_scanTimer(nullptr), m_completeTimer(nullptr), m_scanLine(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("RETINA SCAN VERIFICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    // Widget dla obrazu oka
    QWidget *eyeContainer = new QWidget(this);
    eyeContainer->setFixedSize(220, 220);
    eyeContainer->setStyleSheet("background-color: transparent;");

    QVBoxLayout *eyeLayout = new QVBoxLayout(eyeContainer);
    eyeLayout->setContentsMargins(0, 0, 0, 0);
    eyeLayout->setSpacing(0);

    m_eyeImage = new QLabel(eyeContainer);
    m_eyeImage->setFixedSize(200, 200);
    m_eyeImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 100px;");
    m_eyeImage->setAlignment(Qt::AlignCenter);
    eyeLayout->addWidget(m_eyeImage, 0, Qt::AlignCenter);

    // Nakładka skanera (czerwona linia skanująca)
    m_scannerOverlay = new QLabel(eyeContainer);
    m_scannerOverlay->setFixedSize(200, 200);
    m_scannerOverlay->setStyleSheet("background-color: transparent;");
    m_scannerOverlay->setAlignment(Qt::AlignCenter);
    m_scannerOverlay->lower(); // Na spód, pod obrazem oka

    QLabel *instructions = new QLabel("Please look directly at the scanner\nDo not move during scan process", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    m_scanProgress = new QProgressBar(this);
    m_scanProgress->setRange(0, 100);
    m_scanProgress->setValue(0);
    m_scanProgress->setTextVisible(false);
    m_scanProgress->setFixedHeight(8);
    m_scanProgress->setFixedWidth(200);
    m_scanProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #333333;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #ff3333;"
        "  border-radius: 3px;"
        "}"
    );

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(eyeContainer, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(m_scanProgress, 0, Qt::AlignCenter);
    layout->addStretch();

    m_scanTimer = new QTimer(this);
    m_scanTimer->setInterval(50);
    connect(m_scanTimer, &QTimer::timeout, this, &RetinaScanLayer::updateScan);

    m_completeTimer = new QTimer(this);
    m_completeTimer->setSingleShot(true);
    m_completeTimer->setInterval(5000); // 5 sekund na skanowanie
    connect(m_completeTimer, &QTimer::timeout, this, &RetinaScanLayer::finishScan);
}

RetinaScanLayer::~RetinaScanLayer() {
    if (m_scanTimer) {
        m_scanTimer->stop();
        delete m_scanTimer;
        m_scanTimer = nullptr;
    }
    
    if (m_completeTimer) {
        m_completeTimer->stop();
        delete m_completeTimer;
        m_completeTimer = nullptr;
    }
}

void RetinaScanLayer::initialize() {
    reset();
    generateEyeImage();
    QTimer::singleShot(1000, this, &RetinaScanLayer::startScanAnimation);
}

void RetinaScanLayer::reset() {
    if (m_scanTimer && m_scanTimer->isActive()) {
        m_scanTimer->stop();
    }
    
    if (m_completeTimer && m_completeTimer->isActive()) {
        m_completeTimer->stop();
    }
    
    m_scanLine = 0;
    m_scanProgress->setValue(0);
    m_scannerOverlay->clear();
}

void RetinaScanLayer::updateScan() {
    // Aktualizacja paska postępu
    int value = m_scanProgress->value() + 1;
    m_scanProgress->setValue(value);
    
    // Aktualizacja linii skanowania
    m_scanLine += 2;
    if (m_scanLine > 200) {
        m_scanLine = 0;
    }
    
    // Rysowanie linii skanowania
    QPixmap overlay(200, 200);
    overlay.fill(Qt::transparent);
    
    QPainter painter(&overlay);
    painter.setRenderHint(QPainter::Antialiasing);

    // Linia skanująca
    QPen scanPen(QColor(255, 0, 0, 180), 2);
    painter.setPen(scanPen);
    painter.drawLine(0, m_scanLine, 200, m_scanLine);
    
    // Efekt świecenia linii
    QColor glowColor(255, 0, 0, 80);
    QPen glowPen(glowColor, 1);
    painter.setPen(glowPen);
    
    for (int i = 1; i <= 5; i++) {
        painter.drawLine(0, m_scanLine - i, 200, m_scanLine - i);
        painter.drawLine(0, m_scanLine + i, 200, m_scanLine + i);
    }
    
    painter.end();
    
    m_scannerOverlay->setPixmap(overlay);
}

void RetinaScanLayer::finishScan() {
    m_scanTimer->stop();
    
    // Zmiana koloru na zielony po pomyślnym skanowaniu
    m_eyeImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 100px;");
    
    m_scanProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #33ff33;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #33ff33;"
        "  border-radius: 3px;"
        "}"
    );
    
    // Animacja zanikania po krótkim pokazaniu sukcesu
    QTimer::singleShot(800, this, [this]() {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
        this->setGraphicsEffect(effect);

        QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
        animation->setDuration(500);
        animation->setStartValue(1.0);
        animation->setEndValue(0.0);
        animation->setEasingCurve(QEasingCurve::OutQuad);

        connect(animation, &QPropertyAnimation::finished, this, [this]() {
            emit layerCompleted();
        });

        animation->start(QPropertyAnimation::DeleteWhenStopped);
    });
}

void RetinaScanLayer::startScanAnimation() {
    m_scanTimer->start();
    m_completeTimer->start();
}

void RetinaScanLayer::generateEyeImage() {
    QImage eye(200, 200, QImage::Format_ARGB32);
    eye.fill(Qt::transparent);
    
    QPainter painter(&eye);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Tło oka (białko)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(240, 240, 240));
    painter.drawEllipse(10, 10, 180, 180);
    
    // Tęczówka
    QRadialGradient irisGradient(QPointF(100, 100), 60);
    
    // Losowy kolor tęczówki
    QRandomGenerator* rng = QRandomGenerator::global();
    QColor irisColor;
    int colorChoice = rng->bounded(5);
    
    switch (colorChoice) {
        case 0: irisColor = QColor(60, 120, 180); break;  // Niebieski
        case 1: irisColor = QColor(80, 140, 70); break;   // Zielony
        case 2: irisColor = QColor(110, 75, 50); break;   // Brązowy
        case 3: irisColor = QColor(100, 80, 140); break;  // Fioletowy
        case 4: irisColor = QColor(80, 100, 110); break;  // Szary
    }
    
    irisGradient.setColorAt(0, irisColor.lighter(120));
    irisGradient.setColorAt(0.8, irisColor);
    irisGradient.setColorAt(1.0, irisColor.darker(150));
    
    painter.setBrush(irisGradient);
    painter.drawEllipse(50, 50, 100, 100);
    
    // Źrenica
    painter.setBrush(Qt::black);
    painter.drawEllipse(75, 75, 50, 50);
    
    // Dodanie losowych wzorów na tęczówce
    painter.setPen(QPen(irisColor.darker(200), 1));
    
    for (int i = 0; i < 20; i++) {
        int startAngle = rng->bounded(360);
        int spanAngle = rng->bounded(20, 80);
        int radius = rng->bounded(35, 50);
        
        painter.drawArc(100 - radius, 100 - radius, 
                         radius * 2, radius * 2, 
                         startAngle * 16, spanAngle * 16);
    }
    
    // Odblaski
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(75, 65, 15, 10);
    painter.drawEllipse(110, 90, 8, 6);
    
    painter.end();
    
    m_eyeImage->setPixmap(QPixmap::fromImage(eye));
}