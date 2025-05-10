#include "communication_stream.h"

#include <QElapsedTimer>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTimer>

#include "../../app/wavelength_config.h"
#include "../../app/managers/translation_manager.h"

CommunicationStream::CommunicationStream(QWidget *parent): QOpenGLWidget(parent),
                                                           base_wave_amplitude_(0.05),
                                                           amplitude_scale_(0.25),
                                                           wave_amplitude_(base_wave_amplitude_),
                                                           target_wave_amplitude_(base_wave_amplitude_),
                                                           wave_frequency_(2.0),
                                                           wave_speed_(1.0),
                                                           glitch_intensity_(0.0), wave_thickness_(0.008),
                                                           state_(kIdle), current_message_index_(-1),
                                                           initialized_(false), time_offset_(0.0),
                                                           shader_program_(nullptr),
                                                           vertex_buffer_(QOpenGLBuffer::VertexBuffer),
                                                           config_(WavelengthConfig::GetInstance()),
                                                           stream_color_(config_->GetStreamColor()) {
    setMinimumSize(600, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    TranslationManager *translator = TranslationManager::GetInstance();

    animation_timer_ = new QTimer(this);
    connect(animation_timer_, &QTimer::timeout, this, &CommunicationStream::UpdateAnimation);
    animation_timer_->setTimerType(Qt::PreciseTimer);
    animation_timer_->start(16); // ~60fps

    glitch_timer_ = new QTimer(this);
    connect(glitch_timer_, &QTimer::timeout, this, &CommunicationStream::TriggerRandomGlitch);
    glitch_timer_->start(3000 + QRandomGenerator::global()->bounded(2000));

    connect(config_, &WavelengthConfig::configChanged, this, &CommunicationStream::UpdateStreamColor);


    stream_name_label_ = new QLabel(translator->Translate("CommunicationStream.Title", "COMMUNICATION STREAM"), this);
    stream_name_label_->setStyleSheet(
        "QLabel {"
        "  color: #00ccff;"
        "  background-color: rgba(0, 15, 30, 180);"
        "  border: 1px solid #00aaff;"
        "  border-radius: 0px;"
        "  padding: 3px 10px;"
        "  font-family: 'Consolas', sans-serif;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
    );
    stream_name_label_->setAlignment(Qt::AlignCenter);
    stream_name_label_->adjustSize();
    stream_name_label_->move((width() - stream_name_label_->width()) / 2, 10);

    transmitting_user_label_ = new UserInfoLabel(this);
    transmitting_user_label_->setStyleSheet(
        "UserInfoLabel {"
        "  color: #dddddd;"
        "  background-color: rgba(0, 0, 0, 180);"
        "  border: 1px solid #555555;"
        "  border-radius: 0px;"
        "  padding: 2px 4px;"
        "  font-family: 'Consolas', sans-serif;"
        "  font-weight: bold;"
        "  font-size: 10px;"
        "}"
    );
    transmitting_user_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    transmitting_user_label_->hide();

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // MSAA
    format.setSwapInterval(1); // V-Sync
    setFormat(format);

    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

CommunicationStream::~CommunicationStream() {
    makeCurrent();
    vao_.destroy();
    vertex_buffer_.destroy();
    delete shader_program_;
    doneCurrent();
}

StreamMessage *CommunicationStream::AddMessageWithAttachment(const QString &content, const QString &sender,
                                                             const StreamMessage::MessageType type,
                                                             const QString &message_id) {
    StartReceivingAnimation();

    const auto message = new StreamMessage(content, sender, type, message_id, this);
    message->hide();

    message->AddAttachment(content);

    messages_.append(message);

    ConnectSignalsForMessage(message);

    if (current_message_index_ == -1) {
        QTimer::singleShot(1200, this, [this] {
            if (!messages_.isEmpty()) {
                ShowMessageAtIndex(messages_.size() - 1);
            }
        });
    } else {
        UpdateNavigationButtonsForCurrentMessage();
    }

    return message;
}

void CommunicationStream::SetWaveAmplitude(const qreal amplitude) {
    wave_amplitude_ = amplitude;
    update();
}

void CommunicationStream::SetWaveFrequency(const qreal frequency) {
    wave_frequency_ = frequency;
    update();
}

void CommunicationStream::SetWaveSpeed(const qreal speed) {
    wave_speed_ = speed;
    update();
}

void CommunicationStream::SetGlitchIntensity(const qreal intensity) {
    glitch_intensity_ = intensity;
    update();
}

void CommunicationStream::SetWaveThickness(const qreal thickness) {
    wave_thickness_ = thickness;
    update();
}

void CommunicationStream::SetStreamName(const QString &name) const {
    stream_name_label_->setText(name);
    stream_name_label_->adjustSize();
    stream_name_label_->move((width() - stream_name_label_->width()) / 2, 10);
}

StreamMessage *CommunicationStream::AddMessage(const QString &content, const QString &sender,
                                               const StreamMessage::MessageType type, const QString &message_id) {
    StartReceivingAnimation();

    const auto message = new StreamMessage(content, sender, type, message_id, this);
    message->hide();

    messages_.append(message);

    ConnectSignalsForMessage(message);

    if (current_message_index_ == -1) {
        QTimer::singleShot(1200, this, [this] {
            if (!messages_.isEmpty()) {
                ShowMessageAtIndex(messages_.size() - 1);
            }
        });
    } else {
        UpdateNavigationButtonsForCurrentMessage();
    }
    return message;
}

void CommunicationStream::ClearMessages() {
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        StreamMessage *current_message = messages_[current_message_index_];
        DisconnectSignalsForMessage(current_message);
        current_message->hide();
    }

    for (StreamMessage *message: qAsConst(messages_)) {
        DisconnectSignalsForMessage(message);
        message->deleteLater();
    }

    messages_.clear();
    current_message_index_ = -1;
    ReturnToIdleAnimation();
    update();
}

void CommunicationStream::SetTransmittingUser(const QString &user_id) const {
    QString display_text = user_id;
    if (user_id.length() > 15 && user_id.startsWith("client_")) {
        display_text = "CLIENT " + user_id.split('_').last();
    } else if (user_id.length() > 15 && user_id.startsWith("ws_")) {
        display_text = "HOST " + user_id.split('_').last();
    } else if (user_id == "You") {
        display_text = "YOU";
    }

    const auto [color] = GenerateUserVisuals(user_id);
    transmitting_user_label_->SetShapeColor(color);
    transmitting_user_label_->setText(display_text);
    transmitting_user_label_->adjustSize();
    transmitting_user_label_->move(width() - transmitting_user_label_->width() - 10, 10);
    transmitting_user_label_->show();
}

void CommunicationStream::ClearTransmittingUser() const {
    transmitting_user_label_->hide();
    transmitting_user_label_->setText("");
}

void CommunicationStream::SetAudioAmplitude(const qreal amplitude) {
    target_wave_amplitude_ = base_wave_amplitude_ + qBound(0.0, amplitude, 1.0) * amplitude_scale_;
}

void CommunicationStream::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    shader_program_ = new QOpenGLShaderProgram();

    const auto vertex_shader_source = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;

            uniform vec2 resolution;

            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

    const auto fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec2 resolution;
    uniform float time;
    uniform float amplitude;
    uniform float frequency;
    uniform float speed;
    uniform float glitchIntensity;
    uniform float waveThickness;
    uniform vec3 streamColor;

    float random(vec2 st) {
        return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
    }

    void main() {
        // standardized screen coordinates
        vec2 uv = gl_FragCoord.xy / resolution.xy;
        uv = uv * 2.0 - 1.0;

        // line parameters
        float lineThickness = waveThickness;
        float glow = 0.03;
        vec3 lineColor = streamColor;
        vec3 glowColor = streamColor * 0.6;

        // main wave
        float xFreq = frequency * 3.14159;
        float tOffset = time * speed;
        float wave = sin(uv.x * xFreq + tOffset) * amplitude;

        // shifting glitch
        if (glitchIntensity > 0.01) {
            float direction = sin(floor(time * 0.2) * 0.5) > 0.0 ? 1.0 : -1.0;

            float glitchPos = fract(time * 0.5) * 2.0 - 1.0;
            glitchPos = direction > 0.0 ? glitchPos : -glitchPos;

            float glitchWidth = 0.1 + glitchIntensity * 0.2;
            float distFromGlitch = abs(uv.x - glitchPos);
            float glitchFactor = smoothstep(glitchWidth, 0.0, distFromGlitch);

            wave += glitchFactor * glitchIntensity * 0.3 * sin(uv.x * 30.0 + time * 10.0);
        }

        float dist = abs(uv.y - wave);

        float line = smoothstep(lineThickness, 0.0, dist);
        float glowFactor = smoothstep(glow, lineThickness, dist);

        vec3 color = lineColor * line + glowColor * glowFactor * (0.3 + glitchIntensity);

        float gridIntensity = 0.1;
        vec2 gridCoord = uv * 20.0;
        if (mod(gridCoord.x, 1.0) < 0.05 || mod(gridCoord.y, 1.0) < 0.05) {
            color += vec3(0.0, 0.3, 0.4) * gridIntensity;
        }

        gridCoord = uv * 40.0;
        if (mod(gridCoord.x, 1.0) < 0.02 || mod(gridCoord.y, 1.0) < 0.02) {
            color += vec3(0.0, 0.1, 0.2) * gridIntensity * 0.5;
        }

        float alpha = line + glowFactor * 0.6 + 0.1;

        FragColor = vec4(color, alpha);
    }
)";

    shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source);
    shader_program_->link();

    vao_.create();
    vao_.bind();

    static constexpr GLfloat vertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f
    };

    vertex_buffer_.create();
    vertex_buffer_.bind();
    vertex_buffer_.allocate(vertices, sizeof(vertices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    vertex_buffer_.release();
    vao_.release();

    initialized_ = true;
}

void CommunicationStream::resizeGL(const int w, const int h) {
    glViewport(0, 0, w, h);
    stream_name_label_->move((width() - stream_name_label_->width()) / 2, 10);

    transmitting_user_label_->move(width() - transmitting_user_label_->width() - 10, 10);

    UpdateMessagePosition();
}

void CommunicationStream::paintGL() {
    if (!initialized_) return;

    glClear(GL_COLOR_BUFFER_BIT);

    shader_program_->bind();

    shader_program_->setUniformValue("resolution", QVector2D(width(), height()));
    shader_program_->setUniformValue("time", static_cast<float>(time_offset_));
    shader_program_->setUniformValue("amplitude", static_cast<float>(wave_amplitude_));
    shader_program_->setUniformValue("frequency", static_cast<float>(wave_frequency_));
    shader_program_->setUniformValue("speed", static_cast<float>(wave_speed_));
    shader_program_->setUniformValue("glitchIntensity", static_cast<float>(glitch_intensity_));
    shader_program_->setUniformValue("waveThickness", static_cast<float>(wave_thickness_));
    shader_program_->setUniformValue("streamColor",
                                     QVector3D(stream_color_.redF(), stream_color_.greenF(), stream_color_.blueF()));

    vao_.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    vao_.release();

    shader_program_->release();
}

void CommunicationStream::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Tab) {
        event->accept();
        return;
    }

    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        const StreamMessage *current_message = messages_[current_message_index_];

        switch (event->key()) {
            case Qt::Key_Right:
                if (current_message->GetNextButton()->isVisible()) {
                    ShowNextMessage();
                    event->accept();
                }
                return;

            case Qt::Key_Left:
                if (current_message->GetPrevButton()->isVisible()) {
                    ShowPreviousMessage();
                    event->accept();
                }
                return;

            case Qt::Key_Return:
                OnMessageRead();
                event->accept();
                return;

            default:
                break;
        }
    }
    QOpenGLWidget::keyPressEvent(event);
}

void CommunicationStream::UpdateAnimation() {
    time_offset_ += 0.02 * wave_speed_;
    wave_amplitude_ = wave_amplitude_ * 0.9 + target_wave_amplitude_ * 0.1;
    if (glitch_intensity_ > 0.0) {
        glitch_intensity_ = qMax(0.0, glitch_intensity_ - 0.005);
    }

    const QRegion update_region(0, 0, width(), height());
    update(update_region);
}

void CommunicationStream::TriggerRandomGlitch() {
    if (state_ == kIdle) {
        const float intensity = 0.3 + QRandomGenerator::global()->bounded(40) / 100.0;

        StartGlitchAnimation(intensity);
    }

    glitch_timer_->setInterval(1000 + QRandomGenerator::global()->bounded(2000));
}

void CommunicationStream::StartReceivingAnimation() {
    state_ = kReceiving;

    const auto amplitude_animation = new QPropertyAnimation(this, "waveAmplitude");
    amplitude_animation->setDuration(1000);
    amplitude_animation->setStartValue(wave_amplitude_);
    amplitude_animation->setEndValue(0.15);
    amplitude_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto frequency_animation = new QPropertyAnimation(this, "waveFrequency");
    frequency_animation->setDuration(1000);
    frequency_animation->setStartValue(wave_frequency_);
    frequency_animation->setEndValue(6.0);
    frequency_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto speed_animation = new QPropertyAnimation(this, "waveSpeed");
    speed_animation->setDuration(1000);
    speed_animation->setStartValue(wave_speed_);
    speed_animation->setEndValue(2.5);
    speed_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto thickness_animation = new QPropertyAnimation(this, "waveThickness");
    thickness_animation->setDuration(1000);
    thickness_animation->setStartValue(wave_thickness_);
    thickness_animation->setEndValue(0.012);
    thickness_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    glitch_animation->setDuration(1000);
    glitch_animation->setStartValue(0.0);
    glitch_animation->setKeyValueAt(0.3, 0.6);
    glitch_animation->setKeyValueAt(0.6, 0.3);
    glitch_animation->setEndValue(0.2);
    glitch_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(amplitude_animation);
    group->addAnimation(frequency_animation);
    group->addAnimation(speed_animation);
    group->addAnimation(thickness_animation);
    group->addAnimation(glitch_animation);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    connect(group, &QParallelAnimationGroup::finished, this, [this] {
        target_wave_amplitude_ = 0.15;
        state_ = kDisplaying;
    });
}

void CommunicationStream::ReturnToIdleAnimation() {
    state_ = kIdle;

    target_wave_amplitude_ = base_wave_amplitude_;

    const auto amplitude_animation = new QPropertyAnimation(this, "waveAmplitude");
    amplitude_animation->setDuration(1500);
    amplitude_animation->setStartValue(wave_amplitude_);
    amplitude_animation->setEndValue(0.01);
    amplitude_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto frequency_animation = new QPropertyAnimation(this, "waveFrequency");
    frequency_animation->setDuration(1500);
    frequency_animation->setStartValue(wave_frequency_);
    frequency_animation->setEndValue(2.0);
    frequency_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto speed_animation = new QPropertyAnimation(this, "waveSpeed");
    speed_animation->setDuration(1500);
    speed_animation->setStartValue(wave_speed_);
    speed_animation->setEndValue(1.0);
    speed_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto thickness_animation = new QPropertyAnimation(this, "waveThickness");
    thickness_animation->setDuration(1500);
    thickness_animation->setStartValue(wave_thickness_);
    thickness_animation->setEndValue(0.008);
    thickness_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto group = new QParallelAnimationGroup(this);
    group->addAnimation(amplitude_animation);
    group->addAnimation(frequency_animation);
    group->addAnimation(speed_animation);
    group->addAnimation(thickness_animation);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void CommunicationStream::StartGlitchAnimation(const qreal intensity) {
    const int duration = 600 + QRandomGenerator::global()->bounded(400);

    const auto glitch_animation = new QPropertyAnimation(this, "glitchIntensity");
    glitch_animation->setDuration(duration);
    glitch_animation->setStartValue(glitch_intensity_);
    glitch_animation->setKeyValueAt(0.1, intensity);
    glitch_animation->setKeyValueAt(0.5, intensity * 0.7);
    glitch_animation->setEndValue(0.0);
    glitch_animation->setEasingCurve(QEasingCurve::OutQuad);
    glitch_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void CommunicationStream::ShowMessageAtIndex(int index) {
    OptimizeForMessageTransition();

    if (index < 0 || index >= messages_.size()) {
        qWarning() << "[COMMUNICATION STREAM]::showMessageAtIndex - Invalid index:" << index << ", list size:"
                << messages_.size();
        if (!messages_.isEmpty()) {
            index = 0; // fallback
            qWarning() << "[COMMUNICATION STREAM]::showMessageAtIndex - Index has been set to 0.";
        } else {
            current_message_index_ = -1;
            ReturnToIdleAnimation();
            return;
        }
    }

    if (index == current_message_index_ && messages_[index]->isVisible()) {
        messages_[index]->setFocus();
        return;
    }

    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        if (current_message_index_ != index) {
            StreamMessage *previous_message_ptr = nullptr;
            previous_message_ptr = messages_[current_message_index_];
            DisconnectSignalsForMessage(previous_message_ptr);
            previous_message_ptr->hide();
        }
    }

    current_message_index_ = index;
    StreamMessage *message = messages_[current_message_index_];

    ConnectSignalsForMessage(message);

    state_ = kDisplaying;

    // 1. making sure the widget has a chance to calculate its size
    message->adjustSize();
    // 2. setting position and size
    UpdateMessagePosition();
    // 3. raise widget
    message->raise();
    // 4. show widget
    message->show();
    // 5. start animation
    message->FadeIn();
    // 6. update buttons
    UpdateNavigationButtonsForCurrentMessage();
    // 7. focus widget
    message->setFocus();

    update();
}

void CommunicationStream::ShowNextMessage() {
    if (current_message_index_ < messages_.size() - 1) {
        OptimizeForMessageTransition();
        ShowMessageAtIndex(current_message_index_ + 1);
    }
}

void CommunicationStream::ShowPreviousMessage() {
    if (current_message_index_ > 0) {
        OptimizeForMessageTransition();
        ShowMessageAtIndex(current_message_index_ - 1);
    }
}

void CommunicationStream::OnMessageRead() {
    if (current_message_index_ < 0 || current_message_index_ >= messages_.size()) {
        qWarning() << "[COMMUNICATION STREAM]::onMessageRead - No current message to start shutdown.";
        return;
    }

    is_clearing_all_messages_ = true;

    StreamMessage *message = messages_[current_message_index_];
    message->MarkAsRead();
}

void CommunicationStream::HandleMessageHidden() {
    const auto hidden_message = qobject_cast<StreamMessage *>(sender());
    if (!hidden_message) {
        qWarning() <<
                "[COMMUNICATION STREAM]::handleMessageHidden - A hidden() signal was received from an unknown object.";
        return;
    }

    const int hidden_message_index = messages_.indexOf(hidden_message);

    DisconnectSignalsForMessage(hidden_message);

    if (is_clearing_all_messages_) {
        if (hidden_message_index != -1) {
            messages_.removeAt(hidden_message_index);
        } else {
            qWarning() <<
                    "[COMMUNICATION STREAM]::handleMessageHidden [ClearAll] - Hidden message not found in the list!";
        }
        hidden_message->deleteLater();

        QList<StreamMessage *> messages_to_remove = messages_;
        messages_.clear();

        for (StreamMessage *message: messages_to_remove) {
            if (message) {
                DisconnectSignalsForMessage(message);
                message->hide();
                message->deleteLater();
            }
        }

        current_message_index_ = -1;
        is_clearing_all_messages_ = false;
        ReturnToIdleAnimation();
        update();
        return;
    }

    if (!hidden_message->GetMessageId().isEmpty()) {
        if (hidden_message_index != -1) {
            messages_.removeAt(hidden_message_index);
            if (hidden_message_index < current_message_index_) {
                current_message_index_--;
            } else if (hidden_message_index == current_message_index_) {
                current_message_index_ = -1;
            }
        }
        hidden_message->deleteLater();

        if (messages_.isEmpty()) {
            current_message_index_ = -1;
            ReturnToIdleAnimation();
        } else if (current_message_index_ == -1) {
            ReturnToIdleAnimation();
        }
    } else {
        if (hidden_message_index == current_message_index_) {
            if (hidden_message_index != -1) {
                messages_.removeAt(hidden_message_index);
            } else {
                qWarning() <<
                        "[COMMUNICATION STREAM]::handleMessageHidden [SingleClose] - Closed message not found in list!";
            }
            hidden_message->deleteLater();

            if (messages_.isEmpty()) {
                current_message_index_ = -1;
                ReturnToIdleAnimation();
            } else {
                int next_index_to_show = qMin(hidden_message_index, messages_.size() - 1);
                QTimer::singleShot(0, this, [this, next_index_to_show] {
                    if (next_index_to_show >= 0 && next_index_to_show < messages_.size()) {
                        ShowMessageAtIndex(next_index_to_show);
                    } else if (!messages_.isEmpty()) {
                        ShowMessageAtIndex(0);
                    } else {
                        current_message_index_ = -1;
                        ReturnToIdleAnimation();
                    }
                });
                current_message_index_ = -1;
            }
        }
    }
    update();
}

void CommunicationStream::UpdateStreamColor(const QString &key) {
    if (key == "stream_color" || key == "all") {
        stream_color_ = config_->GetStreamColor();
        if (isVisible()) {
            update();
        }
    }
}

UserVisuals CommunicationStream::GenerateUserVisuals(const QString &user_id) {
    UserVisuals visuals;
    const quint32 hash = qHash(user_id);

    const int hue = hash % 360; // shade 0-359
    const int saturation = 230 + (hash >> 8) % 26; // saturation 230-255
    const int lightness = 140 + (hash >> 16) % 31; // brightness 140-170

    visuals.color = QColor::fromHsl(hue, saturation, lightness);
    return visuals;
}

void CommunicationStream::ConnectSignalsForMessage(const StreamMessage *message) {
    if (!message) return;
    connect(message->GetNextButton(), &QPushButton::clicked, this, &CommunicationStream::ShowNextMessage,
            Qt::UniqueConnection);
    connect(message->GetPrevButton(), &QPushButton::clicked, this, &CommunicationStream::ShowPreviousMessage,
            Qt::UniqueConnection);
    connect(message->mark_read_button, &QPushButton::clicked, this, &CommunicationStream::OnMessageRead,
            Qt::UniqueConnection);
    connect(message, &StreamMessage::hidden, this, &CommunicationStream::HandleMessageHidden, Qt::UniqueConnection);
}

void CommunicationStream::DisconnectSignalsForMessage(const StreamMessage *message) const {
    if (!message) return;
    disconnect(message, nullptr, this, nullptr);
}

void CommunicationStream::UpdateNavigationButtonsForCurrentMessage() {
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        const StreamMessage *current_message = messages_[current_message_index_];
        const bool has_previous = current_message_index_ > 0;
        const bool has_next = current_message_index_ < messages_.size() - 1;
        current_message->ShowNavigationButtons(has_previous, has_next);
    }
}

void CommunicationStream::OptimizeForMessageTransition() const {
    static QElapsedTimer transition_timer;
    static bool in_transition = false;

    if (!in_transition) {
        in_transition = true;
        transition_timer.start();

        if (animation_timer_->interval() == 16) {
            animation_timer_->setInterval(33);
        }

        QTimer::singleShot(500, this, [this] {
            animation_timer_->setInterval(16);
            in_transition = false;
        });
    }
}

void CommunicationStream::UpdateMessagePosition() {
    if (current_message_index_ >= 0 && current_message_index_ < messages_.size()) {
        StreamMessage *message = messages_[current_message_index_];
        message->move((width() - message->width()) / 2, (height() - message->height()) / 2);
    }
}
