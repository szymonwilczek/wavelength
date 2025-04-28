#include "retina_scan_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

RetinaScanLayer::RetinaScanLayer(QWidget *parent)
    : SecurityLayer(parent), m_scanTimer(nullptr), m_completeTimer(nullptr), m_scanLine(0)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    const auto title = new QLabel("RETINA SCAN VERIFICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    // Widget dla obrazu oka
    const auto eyeContainer = new QWidget(this);
    eyeContainer->setFixedSize(220, 220);
    eyeContainer->setStyleSheet("background-color: transparent;");

    const auto eyeLayout = new QVBoxLayout(eyeContainer);
    eyeLayout->setContentsMargins(0, 0, 0, 0);
    eyeLayout->setSpacing(0);
    eyeLayout->setAlignment(Qt::AlignCenter);

    // Pojedynczy kontener zamiast warstw
    const auto scannerContainer = new QWidget(eyeContainer);
    scannerContainer->setFixedSize(200, 200);
    scannerContainer->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #33ccff; border-radius: 100px;");

    // Pojedyncza etykieta dla połączonego obrazu oka i skanera
    m_eyeImage = new QLabel(scannerContainer);
    m_eyeImage->setFixedSize(200, 200);
    m_eyeImage->setAlignment(Qt::AlignCenter);

    const auto scannerLayout = new QVBoxLayout(scannerContainer);
    scannerLayout->setContentsMargins(0, 0, 0, 0);
    scannerLayout->addWidget(m_eyeImage);

    eyeLayout->addWidget(scannerContainer);

    const auto instructions = new QLabel("Please look directly at the scanner\nDo not move during scan process", this);
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
    "  background-color: #33ccff;"
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
    m_scanTimer->setInterval(16);
    connect(m_scanTimer, &QTimer::timeout, this, &RetinaScanLayer::updateScan);

    m_completeTimer = new QTimer(this);
    m_completeTimer->setSingleShot(true);
    m_completeTimer->setInterval(5000); // 5 sekund na skanowanie
    connect(m_completeTimer, &QTimer::timeout, this, &RetinaScanLayer::finishScan);

    // Przechowujemy bazowy obraz oka (bez linii skanującej)
    m_baseEyeImage = QImage(200, 200, QImage::Format_ARGB32);
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

    if (graphicsEffect()) {
         static_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }

    QTimer::singleShot(500, this, &RetinaScanLayer::startScanAnimation); // Krótsze opóźnienie dla płynności
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

    if (QWidget* eyeParent = m_eyeImage->parentWidget()) {
        eyeParent->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #33ccff; border-radius: 100px;"); // Niebiesko-zielony border
    }
    // Pasek postępu
    m_scanProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #333333;" // Szary border
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #33ccff;" // Niebiesko-zielony chunk
        "  border-radius: 3px;"
        "}"
    );

    // Resetujemy bazowy obraz oka i czyścimy pixmapę
    m_baseEyeImage = QImage(200, 200, QImage::Format_ARGB32);
    m_baseEyeImage.fill(Qt::transparent);
    m_eyeImage->clear(); // Wyczyść aktualną pixmapę

    // --- KLUCZOWA ZMIANA: Przywróć przezroczystość ---
    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void RetinaScanLayer::updateScan() {
    // Aktualizacja paska postępu
    const int value = m_scanProgress->value() + 1;
    m_scanProgress->setValue(value);

    // Aktualizacja linii skanowania - jeden pełny przebieg
    m_scanLine += 1;

    // Zatrzymaj animację po jednym przebiegu
    if (m_scanLine >= 200) {
        m_scanTimer->stop();
        finishScan();
        return;
    }

    // Rysowanie oka z linią skanującą
    QImage combinedImage = m_baseEyeImage.copy();
    QPainter painter(&combinedImage);
    painter.setRenderHint(QPainter::Antialiasing);

    // Linia skanująca - na całą szerokość kwadratu (z marginesem 1px)
    const QPen scanPen(QColor(51, 204, 255, 180), 2);
    painter.setPen(scanPen);
    painter.drawLine(-10, m_scanLine, 219, m_scanLine);

    // Efekt świecenia linii - również na całą szerokość
    constexpr QColor glowColor(51, 204, 255, 50);
    QPen glowPen(glowColor, 1);

    for (int i = 1; i <= 5; i++) {
        painter.setPen(QPen(glowColor, 1));
        painter.drawLine(-10, m_scanLine - i, 219, m_scanLine - i);
        painter.drawLine(-10, m_scanLine + i, 219, m_scanLine + i);
    }

    painter.end();
    m_eyeImage->setPixmap(QPixmap::fromImage(combinedImage));
}

void RetinaScanLayer::finishScan() {
    // Zmiana koloru na zielony po pomyślnym skanowaniu
    if (m_eyeImage->parentWidget()) {
        m_eyeImage->parentWidget()->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 100px;");
    }

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

    // Pokazujemy finalny obraz bez linii skanującej
    const QImage finalImage = m_baseEyeImage.copy();
    m_eyeImage->setPixmap(QPixmap::fromImage(finalImage));

    // Animacja zanikania po krótkim pokazaniu sukcesu
    QTimer::singleShot(800, this, [this]() {
        const auto effect = new QGraphicsOpacityEffect(this);
        this->setGraphicsEffect(effect);

        const auto animation = new QPropertyAnimation(effect, "opacity");
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

void RetinaScanLayer::startScanAnimation() const {
    m_scanTimer->start();
    m_completeTimer->start();
}

void RetinaScanLayer::generateEyeImage() {
    // Tworzenie obrazu oka i zapisanie go w m_baseEyeImage
    m_baseEyeImage = QImage(200, 200, QImage::Format_ARGB32);
    m_baseEyeImage.fill(Qt::transparent);

    QPainter painter(&m_baseEyeImage);
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

    switch (rng->bounded(5)) {
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

    // Ustawienie bazowego obrazu oka w widgecie
    m_eyeImage->setPixmap(QPixmap::fromImage(m_baseEyeImage));
}