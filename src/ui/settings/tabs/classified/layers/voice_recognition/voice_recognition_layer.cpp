#include "voice_recognition_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <QTimer>

#include "../../../../../../app/managers/translation_manager.h"

VoiceRecognitionLayer::VoiceRecognitionLayer(QWidget *parent)
    : SecurityLayer(parent),
      progress_timer_(nullptr),
      recognition_timer_(nullptr),
      audio_process_timer_(nullptr),
      audio_input_(nullptr),
      audio_device_(nullptr),
      is_recording_(false),
      noise_threshold_(0.05f),
      is_speaking_(false),
      silence_counter_(0),
      current_audio_level_(0.0f) {
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    translator_ = TranslationManager::GetInstance();

    const auto title = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.VoiceRecognition.Title",
                               "VOICE RECOGNITION VERIFICATION"), this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    audio_visualizer_label_ = new QLabel(this);
    audio_visualizer_label_->setFixedSize(400, 200);
    audio_visualizer_label_->setStyleSheet(
        "background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");
    audio_visualizer_label_->setAlignment(Qt::AlignCenter);

    visualizer_data_.resize(100);
    visualizer_data_.fill(0);

    const auto instructions = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.VoiceRecognition.Info",
                               "Please speak into the microphone\nVoice pattern verification in progress..."),
        this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    recognition_progress_ = new QProgressBar(this);
    recognition_progress_->setRange(0, 100);
    recognition_progress_->setValue(0);
    recognition_progress_->setTextVisible(false);
    recognition_progress_->setFixedHeight(8);
    recognition_progress_->setFixedWidth(400);
    recognition_progress_->setStyleSheet(
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
    layout->addWidget(audio_visualizer_label_, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(recognition_progress_, 0, Qt::AlignCenter);
    layout->addStretch();

    progress_timer_ = new QTimer(this);
    progress_timer_->setInterval(100);
    connect(progress_timer_, &QTimer::timeout, this, &VoiceRecognitionLayer::UpdateProgress);

    recognition_timer_ = new QTimer(this);
    recognition_timer_->setSingleShot(true);
    recognition_timer_->setInterval(8000);
    connect(recognition_timer_, &QTimer::timeout, this, &VoiceRecognitionLayer::FinishRecognition);

    audio_process_timer_ = new QTimer(this);
    audio_process_timer_->setInterval(50);
    connect(audio_process_timer_, &QTimer::timeout, this, &VoiceRecognitionLayer::ProcessAudioInput);
}

VoiceRecognitionLayer::~VoiceRecognitionLayer() {
    StopRecording();

    if (progress_timer_) {
        progress_timer_->stop();
        delete progress_timer_;
        progress_timer_ = nullptr;
    }

    if (recognition_timer_) {
        recognition_timer_->stop();
        delete recognition_timer_;
        recognition_timer_ = nullptr;
    }

    if (audio_process_timer_) {
        audio_process_timer_->stop();
        delete audio_process_timer_;
        audio_process_timer_ = nullptr;
    }
}

void VoiceRecognitionLayer::Initialize() {
    Reset();

    if (graphicsEffect()) {
        dynamic_cast<QGraphicsOpacityEffect *>(graphicsEffect())->setOpacity(1.0);
    }

    QTimer::singleShot(500, this, &VoiceRecognitionLayer::StartRecording);
}

void VoiceRecognitionLayer::Reset() {
    StopRecording();

    if (progress_timer_ && progress_timer_->isActive()) {
        progress_timer_->stop();
    }
    if (recognition_timer_ && recognition_timer_->isActive()) {
        recognition_timer_->stop();
    }

    recognition_progress_->setValue(0);
    audio_buffer_.clear();
    visualizer_data_.fill(0);
    is_speaking_ = false;
    silence_counter_ = 0;
    current_audio_level_ = 0.0f;

    audio_visualizer_label_->setStyleSheet(
        "background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");
    recognition_progress_->setStyleSheet(
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

    UpdateAudioVisualizer(QByteArray());

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect *>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void VoiceRecognitionLayer::StartRecording() {
    if (is_recording_)
        return;

    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    const QAudioDeviceInfo input_device = QAudioDeviceInfo::defaultInputDevice();
    if (!input_device.isFormatSupported(format)) {
        format = input_device.nearestFormat(format);
    }

    audio_input_ = new QAudioInput(input_device, format, this);
    audio_device_ = audio_input_->start();

    if (audio_device_) {
        is_recording_ = true;
        progress_timer_->start();
        recognition_timer_->start();
        audio_process_timer_->start();
    }
}

void VoiceRecognitionLayer::StopRecording() {
    if (!is_recording_)
        return;

    if (audio_input_) {
        audio_input_->stop();
        delete audio_input_;
        audio_input_ = nullptr;
        audio_device_ = nullptr;
    }

    if (audio_process_timer_ && audio_process_timer_->isActive()) {
        audio_process_timer_->stop();
    }

    is_recording_ = false;
}

bool VoiceRecognitionLayer::IsSpeaking(const float audio_level) const {
    return audio_level > noise_threshold_;
}

void VoiceRecognitionLayer::ProcessAudioInput() {
    if (!audio_device_ || !is_recording_)
        return;

    QByteArray data;
    const qint64 len = audio_input_->bytesReady();

    if (len > 0) {
        data.resize(len);
        audio_device_->read(data.data(), len);

        float current_level = 0.0f;
        if (data.size() > 0) {
            const auto samples = reinterpret_cast<const qint16 *>(data.constData());
            const int num_of_samples = data.size() / sizeof(qint16);

            float sum = 0;
            for (int i = 0; i < num_of_samples; i++) {
                sum += qAbs(samples[i]) / 32768.0f;
            }

            if (num_of_samples > 0) {
                current_level = sum / num_of_samples;
            }
        }

        if (is_speaking_) {
            is_speaking_ = current_level > noise_threshold_ * 0.8f;
        } else {
            is_speaking_ = current_level > noise_threshold_ * 1.2f;
        }

        if (is_speaking_) {
            silence_counter_ = 0;
            audio_buffer_.append(data);
        } else {
            silence_counter_ += audio_process_timer_->interval();
        }

        current_audio_level_ = current_level;
        UpdateAudioVisualizer(data);
    }
}

void VoiceRecognitionLayer::UpdateAudioVisualizer(const QByteArray &data) {
    if (data.size() > 0) {
        auto samples = reinterpret_cast<const qint16 *>(data.constData());
        int num_of_samples = data.size() / sizeof(qint16);

        for (int i = 0; i < visualizer_data_.size() - 1; i++) {
            visualizer_data_[i] = visualizer_data_[i + 1];
        }

        float sum = 0;
        for (int i = 0; i < num_of_samples; i++) {
            sum += qAbs(samples[i]) / 32768.0f;
        }

        if (num_of_samples > 0) {
            visualizer_data_[visualizer_data_.size() - 1] = sum / num_of_samples;
        }
    }

    QImage visualizer(400, 200, QImage::Format_ARGB32);
    visualizer.fill(Qt::transparent);

    QPainter painter(&visualizer);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 25, 40, 220));
    painter.drawRect(0, 0, visualizer.width(), visualizer.height());

    int threshold_y = visualizer.height() / 2 - noise_threshold_ * 80.0f;
    int threshold_y2 = visualizer.height() / 2 + noise_threshold_ * 80.0f;
    painter.setPen(QPen(QColor(200, 200, 200, 100), 1, Qt::DashLine));
    painter.drawLine(0, threshold_y, visualizer.width(), threshold_y);
    painter.drawLine(0, threshold_y2, visualizer.width(), threshold_y2);

    QPainterPath path;
    int center_y = visualizer.height() / 2;
    int bar_width = visualizer.width() / visualizer_data_.size();

    QColor wave_color = is_speaking_ ? QColor(51, 153, 255, 200) : QColor(180, 180, 180, 150);
    QColor fill_color = is_speaking_ ? QColor(51, 153, 255, 50) : QColor(100, 100, 100, 30);

    painter.setPen(QPen(wave_color, 2));
    painter.setBrush(QBrush(fill_color));

    path.moveTo(0, center_y);
    for (int i = 0; i < visualizer_data_.size(); i++) {
        float amplitude = visualizer_data_[i] * 80.0f;
        path.lineTo(i * bar_width, center_y - amplitude);
    }

    for (int i = visualizer_data_.size() - 1; i >= 0; i--) {
        float amplitude = visualizer_data_[i] * 80.0f;
        path.lineTo(i * bar_width, center_y + amplitude);
    }

    path.closeSubpath();
    painter.drawPath(path);

    painter.setPen(QPen(QColor(100, 100, 100, 50), 1, Qt::DotLine));
    for (int i = 0; i < 4; i++) {
        int y = visualizer.height() * (i + 1) / 5;
        painter.drawLine(0, y, visualizer.width(), y);
    }

    for (int i = 0; i < 5; i++) {
        int x = visualizer.width() * i / 5;
        painter.drawLine(x, 0, x, visualizer.height());
    }

    QString speaking_status = is_speaking_
                                  ? translator_->Translate("ClassifiedSettingsWidget.VoiceRecognition.TalkingDetected",
                                                           "Talking detected")
                                  : translator_->Translate("ClassifiedSettingsWidget.VoiceRecognition.SilenceDetected",
                                                           "Silence");
    painter.setPen(QPen(is_speaking_ ? Qt::green : Qt::red));
    painter.drawText(10, 20, speaking_status);

    painter.end();

    audio_visualizer_label_->setPixmap(QPixmap::fromImage(visualizer));
}

void VoiceRecognitionLayer::UpdateProgress() {
    const int current_value = recognition_progress_->value();

    if (is_speaking_) {
        int new_value = current_value + 2;
        if (new_value > 100) new_value = 100;
        recognition_progress_->setValue(new_value);

        if (new_value >= 100) {
            if (progress_timer_->isActive()) {
                progress_timer_->stop();
            }
            FinishRecognition();
        }
    }
}

void VoiceRecognitionLayer::FinishRecognition() {
    StopRecording();
    progress_timer_->stop();

    audio_visualizer_label_->setStyleSheet(
        "background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 5px;");

    recognition_progress_->setStyleSheet(
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
    recognition_progress_->setValue(100);

    QImage final_visualizer(400, 200, QImage::Format_ARGB32);
    final_visualizer.fill(Qt::transparent);

    QPainter painter(&final_visualizer);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 25, 40, 220));
    painter.drawRect(0, 0, final_visualizer.width(), final_visualizer.height());

    QPainterPath path;
    const int center_y = final_visualizer.height() / 2;
    const int bar_width = final_visualizer.width() / visualizer_data_.size();

    painter.setPen(QPen(QColor(50, 200, 50, 200), 2));
    painter.setBrush(QBrush(QColor(50, 200, 50, 50)));

    path.moveTo(0, center_y);
    for (int i = 0; i < visualizer_data_.size(); i++) {
        const float amplitude = visualizer_data_[i] * 80.0f;
        path.lineTo(i * bar_width, center_y - amplitude);
    }

    for (int i = visualizer_data_.size() - 1; i >= 0; i--) {
        const float amplitude = visualizer_data_[i] * 80.0f;
        path.lineTo(i * bar_width, center_y + amplitude);
    }

    path.closeSubpath();
    painter.drawPath(path);

    painter.setPen(QPen(Qt::green));
    painter.setFont(QFont("Consolas", 12));
    painter.drawText(final_visualizer.rect(), Qt::AlignCenter,
                     translator_->Translate("ClassifiedSettingsWidget.VoiceRecognition.VoicePatternVerified",
                                            "Voice pattern verified"));

    painter.end();

    audio_visualizer_label_->setPixmap(QPixmap::fromImage(final_visualizer));

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
