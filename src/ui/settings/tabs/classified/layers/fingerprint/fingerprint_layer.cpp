#include "fingerprint_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDir>
#include <QCoreApplication>

FingerprintLayer::FingerprintLayer(QWidget *parent)
    : SecurityLayer(parent), m_fingerprintTimer(nullptr), m_svgRenderer(nullptr)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("FINGERPRINT AUTHENTICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    m_fingerprintImage = new QLabel(this);
    m_fingerprintImage->setFixedSize(200, 200);
    m_fingerprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
    m_fingerprintImage->setCursor(Qt::PointingHandCursor);
    m_fingerprintImage->setAlignment(Qt::AlignCenter);
    m_fingerprintImage->installEventFilter(this);

    auto *instructions = new QLabel("Press and hold on fingerprint to scan", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    m_fingerprintProgress = new QProgressBar(this);
    m_fingerprintProgress->setRange(0, 100);
    m_fingerprintProgress->setValue(0);
    m_fingerprintProgress->setTextVisible(false);
    m_fingerprintProgress->setFixedHeight(8);
    m_fingerprintProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #333333;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #3399ff;" // Jasny niebieski
        "  border-radius: 3px;"
        "}"
    );

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(m_fingerprintImage, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(m_fingerprintProgress);
    layout->addStretch();

    m_fingerprintTimer = new QTimer(this);
    m_fingerprintTimer->setInterval(30); // Szybki postęp
    connect(m_fingerprintTimer, &QTimer::timeout, this, &FingerprintLayer::updateProgress);

    // Inicjalizacja listy dostępnych plików daktylogramów
    m_fingerprintFiles = QStringList()
        << ":/resources/security/fingerprint.svg";

    // Jeśli powyższe ścieżki nie działają, możesz użyć tej pojedynczej
    // m_fingerprintFiles = QStringList() << ":/assets/security/fingerprint.svg";

    m_svgRenderer = new QSvgRenderer(this);
}

FingerprintLayer::~FingerprintLayer() {
    if (m_fingerprintTimer) {
        m_fingerprintTimer->stop();
        delete m_fingerprintTimer;
        m_fingerprintTimer = nullptr;
    }

    if (m_svgRenderer) {
        delete m_svgRenderer;
        m_svgRenderer = nullptr;
    }
}

void FingerprintLayer::initialize() {
    reset();
    loadRandomFingerprint();
    if (graphicsEffect()) {
        dynamic_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }
}

void FingerprintLayer::reset() {
    if (m_fingerprintTimer && m_fingerprintTimer->isActive()) {
        m_fingerprintTimer->stop();
    }
    m_fingerprintProgress->setValue(0);

    m_fingerprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
    m_fingerprintProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #333333;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #3399ff;" // Jasny niebieski
        "  border-radius: 3px;"
        "}"
    );

    if (!m_baseFingerprint.isNull()) {
         m_fingerprintImage->setPixmap(QPixmap::fromImage(m_baseFingerprint));
    }
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect());
    if (effect) {
        effect->setOpacity(1.0);
    }
}

void FingerprintLayer::loadRandomFingerprint() {
    if (m_fingerprintFiles.isEmpty()) {
        m_currentFingerprint = "";
        m_baseFingerprint = QImage(200, 200, QImage::Format_ARGB32);
        m_baseFingerprint.fill(Qt::transparent);

        QPainter painter(&m_baseFingerprint);
        painter.setPen(QPen(Qt::white));
        painter.drawText(m_baseFingerprint.rect(), Qt::AlignCenter, "No fingerprint files");
        painter.end();

        m_fingerprintImage->setPixmap(QPixmap::fromImage(m_baseFingerprint));
        return;
    }

    int index = QRandomGenerator::global()->bounded(m_fingerprintFiles.size());
    m_currentFingerprint = m_fingerprintFiles[index];

    if (!m_svgRenderer->load(m_currentFingerprint)) {
        // Jeśli nie można załadować SVG, wyświetl komunikat o błędzie
        m_baseFingerprint = QImage(200, 200, QImage::Format_ARGB32);
        m_baseFingerprint.fill(Qt::transparent);

        QPainter painter(&m_baseFingerprint);
        painter.setPen(QPen(Qt::red));
        painter.drawText(m_baseFingerprint.rect(), Qt::AlignCenter, "Error loading SVG");
        painter.end();

        m_fingerprintImage->setPixmap(QPixmap::fromImage(m_baseFingerprint));
        return;
    }

    // Renderowanie SVG do obrazu bazowego w kolorze jasno-szarym
    m_baseFingerprint = QImage(200, 200, QImage::Format_ARGB32);
    m_baseFingerprint.fill(Qt::transparent);

    QPainter painter(&m_baseFingerprint);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie odcisku w kolorze jasno-szarym z przezroczystością
    QColor lightGrayColor(180, 180, 180, 120); // Jasno-szary, częściowo przezroczysty

    // Ustawienie koloru dla SVG poprzez kompozycję
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillRect(m_baseFingerprint.rect(), Qt::transparent);

    // Rysowanie SVG
    m_svgRenderer->render(&painter, QRectF(20, 20, 160, 160));

    // Nałożenie koloru jasno-szarego na wygenerowany obraz
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(m_baseFingerprint.rect(), lightGrayColor);

    painter.end();

    m_fingerprintImage->setPixmap(QPixmap::fromImage(m_baseFingerprint));
}

void FingerprintLayer::updateProgress() {
    int value = m_fingerprintProgress->value() + 1; // Szybki postęp (wzrost o 3)
    if (value > 100) value = 100;

    m_fingerprintProgress->setValue(value);

    // Aktualizacja wyglądu odcisku palca podczas skanowania
    updateFingerprintScan(value);

    if (value >= 100) {
        m_fingerprintTimer->stop();
        processFingerprint(true);
    }
}

void FingerprintLayer::processFingerprint(bool completed) {
    if (completed) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        m_fingerprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 5px;");

        m_fingerprintProgress->setStyleSheet(
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

        // Pełne wypełnienie odcisku kolorem zielonym
        if (m_svgRenderer->isValid()) {
            QImage successImage(200, 200, QImage::Format_ARGB32);
            successImage.fill(Qt::transparent);

            QPainter painter(&successImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Renderowanie SVG
            m_svgRenderer->render(&painter, QRectF(20, 20, 160, 160));

            // Nałożenie koloru zielonego
            painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            painter.fillRect(successImage.rect(), QColor(50, 240, 50, 200));

            painter.end();

            m_fingerprintImage->setPixmap(QPixmap::fromImage(successImage));
        }

        // Małe opóźnienie przed animacją zanikania, aby pokazać zmianę kolorów
        QTimer::singleShot(500, this, [this]() {
            // Animacja zanikania
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
}

void FingerprintLayer::updateFingerprintScan(int progressValue) {
    if (!m_svgRenderer->isValid()) {
        return; // Brak prawidłowego SVG do renderowania
    }

    // Tworzymy nowy obraz dla aktualnego stanu skanowania
    QImage scanningFingerprint(200, 200, QImage::Format_ARGB32);
    scanningFingerprint.fill(Qt::transparent);

    QPainter painter(&scanningFingerprint);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie SVG
    m_svgRenderer->render(&painter, QRectF(20, 20, 160, 160));

    // Obliczenie wysokości niebieskiej części (wypełnionej)
    int totalHeight = 160;
    int filledHeight = static_cast<int>((progressValue / 100.0) * totalHeight);

    // Nałożenie koloru dla całego obrazu - najpierw szary
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(20, 20, 160, totalHeight, QColor(180, 180, 180, 120));

    // Następnie nałożenie niebieskiego koloru na zeskanowaną część
    if (filledHeight > 0) {
        painter.fillRect(20, 20, 160, filledHeight, QColor(51, 153, 255, 200)); // Jasny niebieski
    }

    painter.end();

    m_fingerprintImage->setPixmap(QPixmap::fromImage(scanningFingerprint));
}

bool FingerprintLayer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_fingerprintImage) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_fingerprintTimer->start();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_fingerprintTimer->stop();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}