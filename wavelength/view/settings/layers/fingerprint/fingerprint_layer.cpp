#include "fingerprint_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPainterPath>
#include <QPropertyAnimation>

FingerprintLayer::FingerprintLayer(QWidget *parent) 
    : SecurityLayer(parent), m_fingerprintTimer(nullptr)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("FINGERPRINT AUTHENTICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    m_fingerprintImage = new QLabel(this);
    m_fingerprintImage->setFixedSize(200, 200);
    m_fingerprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");
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
        "  background-color: #ff3333;"
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
    m_fingerprintTimer->setInterval(50);
    connect(m_fingerprintTimer, &QTimer::timeout, this, &FingerprintLayer::updateProgress);
}

FingerprintLayer::~FingerprintLayer() {
    if (m_fingerprintTimer) {
        m_fingerprintTimer->stop();
        delete m_fingerprintTimer;
        m_fingerprintTimer = nullptr;
    }
}

void FingerprintLayer::initialize() {
    reset();
    generateRandomFingerprint();
}

void FingerprintLayer::reset() {
    if (m_fingerprintTimer && m_fingerprintTimer->isActive()) {
        m_fingerprintTimer->stop();
    }
    m_fingerprintProgress->setValue(0);
}

void FingerprintLayer::updateProgress() {
    int value = m_fingerprintProgress->value() + 1;
    m_fingerprintProgress->setValue(value);
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

        // Małe opóźnienie przed animacją zanikania, aby pokazać zmianę kolorów
        QTimer::singleShot(500, this, [this]() {
            // Animacja zanikania
            QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_fingerprintImage);
            m_fingerprintImage->setGraphicsEffect(effect);

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

void FingerprintLayer::generateRandomFingerprint() {
    QImage fingerprint(200, 200, QImage::Format_ARGB32);
    fingerprint.fill(Qt::transparent);

    QPainter painter(&fingerprint);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(150, 150, 150, 100), 1));
    painter.setBrush(QBrush(QColor(80, 80, 80, 60)));

    QPainterPath path;
    path.addEllipse(QRectF(50, 30, 100, 140));

    QRandomGenerator* rng = QRandomGenerator::global();
    int numLines = rng->bounded(20, 40);

    for (int i = 0; i < numLines; i++) {
        QPainterPath linePath;
        int startX = rng->bounded(60, 140);
        int startY = rng->bounded(40, 160);
        linePath.moveTo(startX, startY);

        for (int j = 0; j < rng->bounded(3, 8); j++) {
            int controlX1 = startX + rng->bounded(-20, 20);
            int controlY1 = startY + rng->bounded(-15, 15);
            int controlX2 = startX + rng->bounded(-20, 20);
            int controlY2 = startY + rng->bounded(-15, 15);
            int endX = startX + rng->bounded(-30, 30);
            int endY = startY + rng->bounded(-20, 20);

            linePath.cubicTo(controlX1, controlY1, controlX2, controlY2, endX, endY);
            startX = endX;
            startY = endY;
        }

        path.addPath(linePath);
    }

    painter.drawPath(path);
    painter.end();

    m_fingerprintImage->setPixmap(QPixmap::fromImage(fingerprint));
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