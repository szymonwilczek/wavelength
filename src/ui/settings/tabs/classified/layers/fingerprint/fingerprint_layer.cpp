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
    : SecurityLayer(parent), fingerprint_timer_(nullptr), svg_renderer_(nullptr)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel("FINGERPRINT AUTHENTICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    fingerprint_image_ = new QLabel(this);
    fingerprint_image_->setFixedSize(200, 200);
    fingerprint_image_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
    fingerprint_image_->setCursor(Qt::PointingHandCursor);
    fingerprint_image_->setAlignment(Qt::AlignCenter);
    fingerprint_image_->installEventFilter(this);

    auto *instructions = new QLabel("Press and hold on fingerprint to scan", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    fingerprint_progress_ = new QProgressBar(this);
    fingerprint_progress_->setRange(0, 100);
    fingerprint_progress_->setValue(0);
    fingerprint_progress_->setTextVisible(false);
    fingerprint_progress_->setFixedHeight(8);
    fingerprint_progress_->setStyleSheet(
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
    layout->addWidget(fingerprint_image_, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(fingerprint_progress_);
    layout->addStretch();

    fingerprint_timer_ = new QTimer(this);
    fingerprint_timer_->setInterval(30); // Szybki postęp
    connect(fingerprint_timer_, &QTimer::timeout, this, &FingerprintLayer::UpdateProgress);

    // Inicjalizacja listy dostępnych plików daktylogramów
    fingerprint_files_ = QStringList()
        << ":/resources/security/fingerprint.svg";

    // Jeśli powyższe ścieżki nie działają, możesz użyć tej pojedynczej
    // m_fingerprintFiles = QStringList() << ":/assets/security/fingerprint.svg";

    svg_renderer_ = new QSvgRenderer(this);
}

FingerprintLayer::~FingerprintLayer() {
    if (fingerprint_timer_) {
        fingerprint_timer_->stop();
        delete fingerprint_timer_;
        fingerprint_timer_ = nullptr;
    }

    if (svg_renderer_) {
        delete svg_renderer_;
        svg_renderer_ = nullptr;
    }
}

void FingerprintLayer::Initialize() {
    Reset();
    LoadRandomFingerprint();
    if (graphicsEffect()) {
        dynamic_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }
}

void FingerprintLayer::Reset() {
    if (fingerprint_timer_ && fingerprint_timer_->isActive()) {
        fingerprint_timer_->stop();
    }
    fingerprint_progress_->setValue(0);

    fingerprint_image_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
    fingerprint_progress_->setStyleSheet(
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

    if (!base_fingerprint_.isNull()) {
         fingerprint_image_->setPixmap(QPixmap::fromImage(base_fingerprint_));
    }
    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void FingerprintLayer::LoadRandomFingerprint() {
    if (fingerprint_files_.isEmpty()) {
        current_fingerprint_ = "";
        base_fingerprint_ = QImage(200, 200, QImage::Format_ARGB32);
        base_fingerprint_.fill(Qt::transparent);

        QPainter painter(&base_fingerprint_);
        painter.setPen(QPen(Qt::white));
        painter.drawText(base_fingerprint_.rect(), Qt::AlignCenter, "No fingerprint files");
        painter.end();

        fingerprint_image_->setPixmap(QPixmap::fromImage(base_fingerprint_));
        return;
    }

    const int index = QRandomGenerator::global()->bounded(fingerprint_files_.size());
    current_fingerprint_ = fingerprint_files_[index];

    if (!svg_renderer_->load(current_fingerprint_)) {
        // Jeśli nie można załadować SVG, wyświetl komunikat o błędzie
        base_fingerprint_ = QImage(200, 200, QImage::Format_ARGB32);
        base_fingerprint_.fill(Qt::transparent);

        QPainter painter(&base_fingerprint_);
        painter.setPen(QPen(Qt::red));
        painter.drawText(base_fingerprint_.rect(), Qt::AlignCenter, "Error loading SVG");
        painter.end();

        fingerprint_image_->setPixmap(QPixmap::fromImage(base_fingerprint_));
        return;
    }

    // Renderowanie SVG do obrazu bazowego w kolorze jasno-szarym
    base_fingerprint_ = QImage(200, 200, QImage::Format_ARGB32);
    base_fingerprint_.fill(Qt::transparent);

    QPainter painter(&base_fingerprint_);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie odcisku w kolorze jasno-szarym z przezroczystością
    constexpr QColor light_gray_color(180, 180, 180, 120); // Jasno-szary, częściowo przezroczysty

    // Ustawienie koloru dla SVG poprzez kompozycję
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillRect(base_fingerprint_.rect(), Qt::transparent);

    // Rysowanie SVG
    svg_renderer_->render(&painter, QRectF(20, 20, 160, 160));

    // Nałożenie koloru jasno-szarego na wygenerowany obraz
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(base_fingerprint_.rect(), light_gray_color);

    painter.end();

    fingerprint_image_->setPixmap(QPixmap::fromImage(base_fingerprint_));
}

void FingerprintLayer::UpdateProgress() {
    int value = fingerprint_progress_->value() + 1; // Szybki postęp (wzrost o 3)
    if (value > 100) value = 100;

    fingerprint_progress_->setValue(value);

    // Aktualizacja wyglądu odcisku palca podczas skanowania
    UpdateFingerprintScan(value);

    if (value >= 100) {
        fingerprint_timer_->stop();
        ProcessFingerprint(true);
    }
}

void FingerprintLayer::ProcessFingerprint(const bool completed) {
    if (completed) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        fingerprint_image_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 5px;");

        fingerprint_progress_->setStyleSheet(
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
        if (svg_renderer_->isValid()) {
            QImage success_image(200, 200, QImage::Format_ARGB32);
            success_image.fill(Qt::transparent);

            QPainter painter(&success_image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Renderowanie SVG
            svg_renderer_->render(&painter, QRectF(20, 20, 160, 160));

            // Nałożenie koloru zielonego
            painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            painter.fillRect(success_image.rect(), QColor(50, 240, 50, 200));

            painter.end();

            fingerprint_image_->setPixmap(QPixmap::fromImage(success_image));
        }

        // Małe opóźnienie przed animacją zanikania, aby pokazać zmianę kolorów
        QTimer::singleShot(500, this, [this]() {
            // Animacja zanikania
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
}

void FingerprintLayer::UpdateFingerprintScan(const int progress_value) const {
    if (!svg_renderer_->isValid()) {
        return; // Brak prawidłowego SVG do renderowania
    }

    // Tworzymy nowy obraz dla aktualnego stanu skanowania
    QImage scanning_fingerprint(200, 200, QImage::Format_ARGB32);
    scanning_fingerprint.fill(Qt::transparent);

    QPainter painter(&scanning_fingerprint);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie SVG
    svg_renderer_->render(&painter, QRectF(20, 20, 160, 160));

    // Obliczenie wysokości niebieskiej części (wypełnionej)
    constexpr int total_height = 160;
    const int filled_height = static_cast<int>((progress_value / 100.0) * total_height);

    // Nałożenie koloru dla całego obrazu - najpierw szary
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(20, 20, 160, total_height, QColor(180, 180, 180, 120));

    // Następnie nałożenie niebieskiego koloru na zeskanowaną część
    if (filled_height > 0) {
        painter.fillRect(20, 20, 160, filled_height, QColor(51, 153, 255, 200)); // Jasny niebieski
    }

    painter.end();

    fingerprint_image_->setPixmap(QPixmap::fromImage(scanning_fingerprint));
}

bool FingerprintLayer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == fingerprint_image_) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                fingerprint_timer_->start();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                fingerprint_timer_->stop();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}