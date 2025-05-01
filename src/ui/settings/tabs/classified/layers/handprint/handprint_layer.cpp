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
    : SecurityLayer(parent), handprint_timer_(nullptr), svg_renderer_(nullptr)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    const auto title = new QLabel("HANDPRINT AUTHENTICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    handprint_image_ = new QLabel(this);
    handprint_image_->setFixedSize(250, 250);
    handprint_image_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
    handprint_image_->setCursor(Qt::PointingHandCursor);
    handprint_image_->setAlignment(Qt::AlignCenter);
    handprint_image_->installEventFilter(this);

    const auto instructions = new QLabel("Press and hold on handprint to scan", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    handprint_progress_ = new QProgressBar(this);
    handprint_progress_->setRange(0, 100);
    handprint_progress_->setValue(0);
    handprint_progress_->setTextVisible(false);
    handprint_progress_->setFixedHeight(8);
    handprint_progress_->setStyleSheet(
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
    layout->addWidget(handprint_image_, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(handprint_progress_);
    layout->addStretch();

    handprint_timer_ = new QTimer(this);
    handprint_timer_->setInterval(30);
    connect(handprint_timer_, &QTimer::timeout, this, &HandprintLayer::UpdateProgress);

    handprint_files_ = QStringList()
        << ":/resources/security/handprint.svg";

    svg_renderer_ = new QSvgRenderer(this);
}

HandprintLayer::~HandprintLayer() {
    if (handprint_timer_) {
        handprint_timer_->stop();
        delete handprint_timer_;
        handprint_timer_ = nullptr;
    }

    if (svg_renderer_) {
        delete svg_renderer_;
        svg_renderer_ = nullptr;
    }
}

void HandprintLayer::Initialize() {
    Reset();
    LoadRandomHandprint();
    if (graphicsEffect()) {
        dynamic_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }
}

void HandprintLayer::Reset() {
    if (handprint_timer_ && handprint_timer_->isActive()) {
        handprint_timer_->stop();
    }
    handprint_progress_->setValue(0);

    handprint_image_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #3399ff; border-radius: 5px;");
    handprint_progress_->setStyleSheet(
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

    if (!base_handprint_.isNull()) {
        handprint_image_->setPixmap(QPixmap::fromImage(base_handprint_));
    }

    if (auto* effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void HandprintLayer::LoadRandomHandprint() {
    // Wybierz losowy plik odcisku dłoni
    if (handprint_files_.isEmpty()) {
        // Jeśli nie ma plików, użyj placeholder
        current_handprint_ = "";
        base_handprint_ = QImage(250, 250, QImage::Format_ARGB32);
        base_handprint_.fill(Qt::transparent);

        QPainter painter(&base_handprint_);
        painter.setPen(QPen(Qt::white));
        painter.drawText(base_handprint_.rect(), Qt::AlignCenter, "No handprint files");
        painter.end();

        handprint_image_->setPixmap(QPixmap::fromImage(base_handprint_));
        return;
    }

    const int index = QRandomGenerator::global()->bounded(handprint_files_.size());
    current_handprint_ = handprint_files_[index];

    if (!svg_renderer_->load(current_handprint_)) {
        // Jeśli nie można załadować SVG, wyświetl komunikat o błędzie
        base_handprint_ = QImage(250, 250, QImage::Format_ARGB32);
        base_handprint_.fill(Qt::transparent);

        QPainter painter(&base_handprint_);
        painter.setPen(QPen(Qt::red));
        painter.drawText(base_handprint_.rect(), Qt::AlignCenter, "Error loading SVG");
        painter.end();

        handprint_image_->setPixmap(QPixmap::fromImage(base_handprint_));
        return;
    }

    // Renderowanie SVG do obrazu bazowego w kolorze jasno-szarym
    base_handprint_ = QImage(250, 250, QImage::Format_ARGB32);
    base_handprint_.fill(Qt::transparent);

    QPainter painter(&base_handprint_);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie odcisku w kolorze jasno-szarym z przezroczystością
    constexpr QColor light_gray_color(180, 180, 180, 120); // Jasno-szary, częściowo przezroczysty

    // Ustawienie koloru dla SVG poprzez kompozycję
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.fillRect(base_handprint_.rect(), Qt::transparent);

    // Rysowanie SVG
    svg_renderer_->render(&painter, QRectF(25, 25, 200, 200));

    // Nałożenie koloru jasno-szarego na wygenerowany obraz
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(base_handprint_.rect(), light_gray_color);

    painter.end();

    handprint_image_->setPixmap(QPixmap::fromImage(base_handprint_));
}

void HandprintLayer::UpdateProgress() {
    int value = handprint_progress_->value() + 1;
    if (value > 100) value = 100;

    handprint_progress_->setValue(value);
    UpdateHandprintScan(value);

    if (value >= 100) {
        if (handprint_timer_->isActive()) {
            handprint_timer_->stop();
            ProcessHandprint(true);
        }
    }
}

void HandprintLayer::ProcessHandprint(const bool completed) {
    if (completed) {
        // Zmiana kolorów na zielony po pomyślnym przejściu
        handprint_image_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 5px;");

        handprint_progress_->setStyleSheet(
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
            QImage success_image(250, 250, QImage::Format_ARGB32);
            success_image.fill(Qt::transparent);

            QPainter painter(&success_image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Renderowanie SVG
            svg_renderer_->render(&painter, QRectF(25, 25, 200, 200));

            // Nałożenie koloru zielonego
            painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            painter.fillRect(success_image.rect(), QColor(50, 240, 50, 200));

            painter.end();

            handprint_image_->setPixmap(QPixmap::fromImage(success_image));
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

void HandprintLayer::UpdateHandprintScan(const int progress_value) const {
    if (!svg_renderer_->isValid()) {
        return; // Brak prawidłowego SVG do renderowania
    }

    // Tworzymy nowy obraz dla aktualnego stanu skanowania
    QImage scanning_handprint(250, 250, QImage::Format_ARGB32);
    scanning_handprint.fill(Qt::transparent);

    QPainter painter(&scanning_handprint);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Renderowanie SVG
    svg_renderer_->render(&painter, QRectF(25, 25, 200, 200));

    // Obliczenie wysokości niebieskiej części (wypełnionej)
    constexpr int total_height = 200;
    const int filled_height = static_cast<int>((progress_value / 100.0) * total_height);

    // Nałożenie koloru dla całego obrazu - najpierw szary
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(25, 25, 200, total_height, QColor(180, 180, 180, 120));

    // Następnie nałożenie niebieskiego koloru na zeskanowaną część
    if (filled_height > 0) {
        painter.fillRect(25, 25, 200, filled_height, QColor(51, 153, 255, 200)); // Jasny niebieski
    }

    painter.end();

    handprint_image_->setPixmap(QPixmap::fromImage(scanning_handprint));
}

bool HandprintLayer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == handprint_image_) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                handprint_timer_->start();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                handprint_timer_->stop();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}