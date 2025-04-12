#include "handprint_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDir>
#include <QCoreApplication>

HandprintLayer::HandprintLayer(QWidget *parent)
    : SecurityLayer(parent), m_handprintTimer(nullptr), m_svgRenderer(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("HANDPRINT AUTHENTICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    m_handprintImage = new QLabel(this);
    m_handprintImage->setFixedSize(250, 250);
    m_handprintImage->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
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
        "  background-color: #3399ff;"
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
    m_handprintTimer->setInterval(30);
    connect(m_handprintTimer, &QTimer::timeout, this, &HandprintLayer::updateProgress);

    m_handprintFiles = QStringList()
        << ":/assets/security_resources/handprint.svg";

    m_svgRenderer = new QSvgRenderer(this);
}

HandprintLayer::~HandprintLayer() {
    if (m_handprintTimer) {
        m_handprintTimer->stop();
        delete m_handprintTimer;
        m_handprintTimer = nullptr;
    }

    if (m_svgRenderer) {
        delete m_svgRenderer;
        m_svgRenderer = nullptr;
    }
}

void HandprintLayer::initialize() {
    reset();
    loadRandomHandprint();
}

void HandprintLayer::reset() {
    if (m_handprintTimer && m_handprintTimer->isActive()) {
        m_handprintTimer->stop();
    }
    m_handprintProgress->setValue(0);
}

void HandprintLayer::loadRandomHandprint() {
    // Wybierz losowy plik odcisku dłoni
    if (m_handprintFiles.isEmpty()) {
        // Jeśli nie ma plików, użyj placeholder
        m_currentHandprint = "";
        m_baseHandprint = QImage(250, 250, QImage::Format_ARGB32);
        m_baseHandprint.fill(Qt::transparent);

        QPainter painter(&m_baseHandprint);
        painter.setPen(QPen(Qt::white));
        painter.drawText(m_baseHandprint.rect(), Qt::AlignCenter, "No handprint files");
        painter.end();

        m_handprintImage->setPixmap(QPixmap::fromImage(m_baseHandprint));
        return;
    }

    int index = QRandomGenerator::global()->bounded(m_handprintFiles.size());
    m_currentHandprint = m_handprintFiles[index];

    if (!m_svgRenderer->load(m_currentHandprint)) {
        // Jeśli nie można załadować SVG, wyświetl komunikat o błędzie
        m_baseHandprint = QImage(250, 250, QImage::Format_ARGB32);
        m_baseHandprint.fill(Qt::transparent);

        QPainter painter(&m_baseHandprint);
        painter.setPen(QPen(Qt::red));
        painter.drawText(m_baseHandprint.rect(), Qt::AlignCenter, "Error loading SVG");
        painter.end();

        m_handprintImage->setPixmap(QPixmap::fromImage(m_baseHandprint));
        return;
    }

    // Renderowanie SVG do obrazu bazowego w kolorze jasno-szarym
    m_baseHandprint = QImage(250, 250, QImage::Format_ARGB32);
    m_baseHandprint.fill(Qt::transparent);

    QPainter painter(&m_baseHandprint);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie odcisku w kolorze jasno-szarym z przezroczystością
    QColor lightGrayColor(180, 180, 180, 120); // Jasno-szary, częściowo przezroczysty

    // Ustawienie koloru dla SVG poprzez kompozycję
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillRect(m_baseHandprint.rect(), Qt::transparent);

    // Rysowanie SVG
    m_svgRenderer->render(&painter, QRectF(25, 25, 200, 200));

    // Nałożenie koloru jasno-szarego na wygenerowany obraz
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(m_baseHandprint.rect(), lightGrayColor);

    painter.end();

    m_handprintImage->setPixmap(QPixmap::fromImage(m_baseHandprint));
}

void HandprintLayer::updateProgress() {
    int value = m_handprintProgress->value() + 1; // Wolniejszy postęp (wzrost o 1)
    if (value > 100) value = 100;

    m_handprintProgress->setValue(value);

    // Aktualizacja wyglądu odcisku dłoni podczas skanowania
    updateHandprintScan(value);

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

        // Pełne wypełnienie odcisku kolorem zielonym
        if (m_svgRenderer->isValid()) {
            QImage successImage(250, 250, QImage::Format_ARGB32);
            successImage.fill(Qt::transparent);

            QPainter painter(&successImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Renderowanie SVG
            m_svgRenderer->render(&painter, QRectF(25, 25, 200, 200));

            // Nałożenie koloru zielonego
            painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            painter.fillRect(successImage.rect(), QColor(50, 240, 50, 200));

            painter.end();

            m_handprintImage->setPixmap(QPixmap::fromImage(successImage));
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

void HandprintLayer::updateHandprintScan(int progressValue) {
    if (!m_svgRenderer->isValid()) {
        return; // Brak prawidłowego SVG do renderowania
    }

    // Tworzymy nowy obraz dla aktualnego stanu skanowania
    QImage scanningHandprint(250, 250, QImage::Format_ARGB32);
    scanningHandprint.fill(Qt::transparent);

    QPainter painter(&scanningHandprint);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie SVG
    m_svgRenderer->render(&painter, QRectF(25, 25, 200, 200));

    // Obliczenie wysokości niebieskiej części (wypełnionej)
    int totalHeight = 200;
    int filledHeight = static_cast<int>((progressValue / 100.0) * totalHeight);

    // Nałożenie koloru dla całego obrazu - najpierw szary
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(25, 25, 200, totalHeight, QColor(180, 180, 180, 120));

    // Następnie nałożenie niebieskiego koloru na zeskanowaną część
    if (filledHeight > 0) {
        painter.fillRect(25, 25, 200, filledHeight, QColor(51, 153, 255, 200)); // Jasny niebieski
    }

    painter.end();

    m_handprintImage->setPixmap(QPixmap::fromImage(scanningHandprint));
}

bool HandprintLayer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_handprintImage) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_handprintTimer->start();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                m_handprintTimer->stop();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}