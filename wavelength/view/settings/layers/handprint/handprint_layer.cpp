#include "handprint_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPainterPath>
#include <QPropertyAnimation>

HandprintLayer::HandprintLayer(QWidget *parent) 
    : SecurityLayer(parent), m_handprintTimer(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("HANDPRINT AUTHENTICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    m_handprintImage = new QLabel(this);
    m_handprintImage->setFixedSize(250, 250);
    m_handprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");
    m_handprintImage->setCursor(Qt::PointingHandCursor);
    m_handprintImage->setAlignment(Qt::AlignCenter);
    m_handprintImage->installEventFilter(this);

    QLabel *instructions = new QLabel("Press and hold on handprint to scan", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    m_handprintProgress = new QProgressBar(this);
    m_handprintProgress->setRange(0, 100);
    m_handprintProgress->setValue(0);
    m_handprintProgress->setTextVisible(false);
    m_handprintProgress->setFixedHeight(8);
    m_handprintProgress->setStyleSheet(
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
    layout->addWidget(m_handprintImage, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(m_handprintProgress);
    layout->addStretch();

    m_handprintTimer = new QTimer(this);
    m_handprintTimer->setInterval(50);
    connect(m_handprintTimer, &QTimer::timeout, this, &HandprintLayer::updateProgress);
}

HandprintLayer::~HandprintLayer() {
    if (m_handprintTimer) {
        m_handprintTimer->stop();
        delete m_handprintTimer;
        m_handprintTimer = nullptr;
    }
}

void HandprintLayer::initialize() {
    reset();
    generateRandomHandprint();
}

void HandprintLayer::reset() {
    if (m_handprintTimer && m_handprintTimer->isActive()) {
        m_handprintTimer->stop();
    }
    m_handprintProgress->setValue(0);
}

void HandprintLayer::updateProgress() {
    int value = m_handprintProgress->value() + 1;
    m_handprintProgress->setValue(value);
    if (value >= 100) {
        m_handprintTimer->stop();
        processHandprint(true);
    }
}

void HandprintLayer::processHandprint(bool completed) {
    if (completed) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        m_handprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 5px;");

        m_handprintProgress->setStyleSheet(
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
            QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_handprintImage);
            m_handprintImage->setGraphicsEffect(effect);

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

void HandprintLayer::generateRandomHandprint() {
    QImage handprint(250, 250, QImage::Format_ARGB32);
    handprint.fill(Qt::transparent);

    QPainter painter(&handprint);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(150, 150, 150, 100), 1));
    painter.setBrush(QBrush(QColor(80, 80, 80, 60)));

    // Główny kształt dłoni
    QPainterPath palmPath;
    palmPath.addEllipse(QRectF(75, 100, 100, 120));
    
    // Kciuk
    QPainterPath thumbPath;
    thumbPath.addEllipse(QRectF(50, 100, 40, 70));
    
    QRandomGenerator* rng = QRandomGenerator::global();

    // Pozostałe palce
    for (int i = 0; i < 4; i++) {
        QPainterPath fingerPath;
        int baseX = 95 + i * 25;
        int length = 80 + rng->bounded(-10, 10);
        int width = 20 + rng->bounded(-5, 5);
        
        fingerPath.addEllipse(QRectF(baseX, 50, width, length));
        palmPath.addPath(fingerPath);
    }

    palmPath.addPath(thumbPath);

    // Dodajemy losowe linie wewnątrz odcisku dłoni
    int numLines = rng->bounded(30, 60);

    for (int i = 0; i < numLines; i++) {
        QPainterPath linePath;
        int startX = rng->bounded(60, 190);
        int startY = rng->bounded(70, 190);
        linePath.moveTo(startX, startY);

        for (int j = 0; j < rng->bounded(2, 6); j++) {
            int controlX1 = startX + rng->bounded(-25, 25);
            int controlY1 = startY + rng->bounded(-20, 20);
            int controlX2 = startX + rng->bounded(-25, 25);
            int controlY2 = startY + rng->bounded(-20, 20);
            int endX = startX + rng->bounded(-35, 35);
            int endY = startY + rng->bounded(-25, 25);

            linePath.cubicTo(controlX1, controlY1, controlX2, controlY2, endX, endY);
            startX = endX;
            startY = endY;
        }

        palmPath.addPath(linePath);
    }

    painter.drawPath(palmPath);
    painter.end();

    m_handprintImage->setPixmap(QPixmap::fromImage(handprint));
}

bool HandprintLayer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_handprintImage) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_handprintTimer->start();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_handprintTimer->stop();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}