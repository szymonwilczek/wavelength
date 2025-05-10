#include "retina_scan_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>

#include "../../../../../../app/managers/translation_manager.h"

RetinaScanLayer::RetinaScanLayer(QWidget *parent)
    : SecurityLayer(parent), scan_timer_(nullptr), complete_timer_(nullptr), scanline_(0) {
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    const TranslationManager *translator = TranslationManager::GetInstance();

    const auto title = new QLabel(
        translator->Translate("ClassifiedSettingsWidget.RetinaScan.Title", "RETINA SCAN VERIFICATION"), this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    const auto eye_container = new QWidget(this);
    eye_container->setFixedSize(220, 220);
    eye_container->setStyleSheet("background-color: transparent;");

    const auto eye_layout = new QVBoxLayout(eye_container);
    eye_layout->setContentsMargins(0, 0, 0, 0);
    eye_layout->setSpacing(0);
    eye_layout->setAlignment(Qt::AlignCenter);

    const auto scanner_container = new QWidget(eye_container);
    scanner_container->setFixedSize(200, 200);
    scanner_container->setStyleSheet(
        "background-color: rgba(10, 25, 40, 220); border: 1px solid #33ccff; border-radius: 100px;");

    eye_image_ = new QLabel(scanner_container);
    eye_image_->setFixedSize(200, 200);
    eye_image_->setAlignment(Qt::AlignCenter);

    const auto scanner_layout = new QVBoxLayout(scanner_container);
    scanner_layout->setContentsMargins(0, 0, 0, 0);
    scanner_layout->addWidget(eye_image_);

    eye_layout->addWidget(scanner_container);

    const auto instructions = new QLabel(translator->Translate("ClassifiedSettingsWidget.RetinaScan.Info",
                                                               "Please look directly at the scanner\nDo not move during scan process"),
                                         this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    scan_progress_ = new QProgressBar(this);
    scan_progress_->setRange(0, 100);
    scan_progress_->setValue(0);
    scan_progress_->setTextVisible(false);
    scan_progress_->setFixedHeight(8);
    scan_progress_->setFixedWidth(200);
    scan_progress_->setStyleSheet(
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
    layout->addWidget(eye_container, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(scan_progress_, 0, Qt::AlignCenter);
    layout->addStretch();

    scan_timer_ = new QTimer(this);
    scan_timer_->setInterval(16);
    connect(scan_timer_, &QTimer::timeout, this, &RetinaScanLayer::UpdateScan);

    complete_timer_ = new QTimer(this);
    complete_timer_->setSingleShot(true);
    complete_timer_->setInterval(5000);
    connect(complete_timer_, &QTimer::timeout, this, &RetinaScanLayer::FinishScan);

    base_eye_image_ = QImage(200, 200, QImage::Format_ARGB32);
}

RetinaScanLayer::~RetinaScanLayer() {
    if (scan_timer_) {
        scan_timer_->stop();
        delete scan_timer_;
        scan_timer_ = nullptr;
    }

    if (complete_timer_) {
        complete_timer_->stop();
        delete complete_timer_;
        complete_timer_ = nullptr;
    }
}

void RetinaScanLayer::Initialize() {
    Reset();
    GenerateEyeImage();

    if (graphicsEffect()) {
        static_cast<QGraphicsOpacityEffect *>(graphicsEffect())->setOpacity(1.0);
    }

    QTimer::singleShot(500, this, &RetinaScanLayer::StartScanAnimation);
}

void RetinaScanLayer::Reset() {
    if (scan_timer_ && scan_timer_->isActive()) {
        scan_timer_->stop();
    }
    if (complete_timer_ && complete_timer_->isActive()) {
        complete_timer_->stop();
    }

    scanline_ = 0;
    scan_progress_->setValue(0);

    if (QWidget *eyeParent = eye_image_->parentWidget()) {
        eyeParent->setStyleSheet(
            "background-color: rgba(10, 25, 40, 220); border: 1px solid #33ccff; border-radius: 100px;");
    }

    scan_progress_->setStyleSheet(
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

    base_eye_image_ = QImage(200, 200, QImage::Format_ARGB32);
    base_eye_image_.fill(Qt::transparent);
    eye_image_->clear();

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect *>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void RetinaScanLayer::UpdateScan() {
    const int value = scan_progress_->value() + 1;
    scan_progress_->setValue(value);
    scanline_ += 1;

    if (scanline_ >= 200) {
        scan_timer_->stop();
        FinishScan();
        return;
    }

    QImage combined_image = base_eye_image_.copy();
    QPainter painter(&combined_image);
    painter.setRenderHint(QPainter::Antialiasing);

    const QPen scan_pen(QColor(51, 204, 255, 180), 2);
    painter.setPen(scan_pen);
    painter.drawLine(-10, scanline_, 219, scanline_);

    constexpr QColor glow_color(51, 204, 255, 50);
    QPen glowPen(glow_color, 1);

    for (int i = 1; i <= 5; i++) {
        painter.setPen(QPen(glow_color, 1));
        painter.drawLine(-10, scanline_ - i, 219, scanline_ - i);
        painter.drawLine(-10, scanline_ + i, 219, scanline_ + i);
    }

    painter.end();
    eye_image_->setPixmap(QPixmap::fromImage(combined_image));
}

void RetinaScanLayer::FinishScan() {
    if (eye_image_->parentWidget()) {
        eye_image_->parentWidget()->setStyleSheet(
            "background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 100px;");
    }

    scan_progress_->setStyleSheet(
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

    const QImage final_image = base_eye_image_.copy();
    eye_image_->setPixmap(QPixmap::fromImage(final_image));

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

void RetinaScanLayer::StartScanAnimation() const {
    scan_timer_->start();
    complete_timer_->start();
}

void RetinaScanLayer::GenerateEyeImage() {
    base_eye_image_ = QImage(200, 200, QImage::Format_ARGB32);
    base_eye_image_.fill(Qt::transparent);

    QPainter painter(&base_eye_image_);
    painter.setRenderHint(QPainter::Antialiasing);

    // background of the eye
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(240, 240, 240));
    painter.drawEllipse(10, 10, 180, 180);

    // iris
    QRadialGradient iris_gradient(QPointF(100, 100), 60);
    QRandomGenerator *rng = QRandomGenerator::global();
    QColor irisColor;

    switch (rng->bounded(5)) {
        case 0: irisColor = QColor(60, 120, 180);
            break;
        case 1: irisColor = QColor(80, 140, 70);
            break;
        case 2: irisColor = QColor(110, 75, 50);
            break;
        case 3: irisColor = QColor(100, 80, 140);
            break;
        case 4: irisColor = QColor(80, 100, 110);
            break;
        default:
            break;
    }

    iris_gradient.setColorAt(0, irisColor.lighter(120));
    iris_gradient.setColorAt(0.8, irisColor);
    iris_gradient.setColorAt(1.0, irisColor.darker(150));

    painter.setBrush(iris_gradient);
    painter.drawEllipse(50, 50, 100, 100);

    // pupil
    painter.setBrush(Qt::black);
    painter.drawEllipse(75, 75, 50, 50);

    painter.setPen(QPen(irisColor.darker(200), 1));

    for (int i = 0; i < 20; i++) {
        int start_angle = rng->bounded(360);
        int span_angle = rng->bounded(20, 80);
        int radius = rng->bounded(35, 50);

        painter.drawArc(100 - radius, 100 - radius,
                        radius * 2, radius * 2,
                        start_angle * 16, span_angle * 16);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(75, 65, 15, 10);
    painter.drawEllipse(110, 90, 8, 6);

    painter.end();

    eye_image_->setPixmap(QPixmap::fromImage(base_eye_image_));
}
