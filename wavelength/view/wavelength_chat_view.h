#ifndef WAVELENGTH_CHAT_VIEW_H
#define WAVELENGTH_CHAT_VIEW_H

#include <qfileinfo.h>
#include <QWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPropertyAnimation>

#include "../messages/wavelength_stream_display.h"
#include "../session/wavelength_session_coordinator.h"
#include "../messages/wavelength_text_display.h"
#include "../ui/chat/chat_style.h"

// Cyberpunkowy przycisk dla interfejsu czatu - uproszczona wersja
class CyberChatButton : public QPushButton {
    Q_OBJECT

public:
    CyberChatButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent) {
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Paleta kolorów
        QColor bgColor(0, 40, 60); // ciemne tło
        QColor borderColor(0, 170, 255); // neonowy niebieski
        QColor textColor(0, 220, 255); // jasny neonowy tekst

        // Ścieżka przycisku ze ściętymi rogami
        QPainterPath path;
        int clipSize = 5;

        path.moveTo(clipSize, 0);
        path.lineTo(width() - clipSize, 0);
        path.lineTo(width(), clipSize);
        path.lineTo(width(), height() - clipSize);
        path.lineTo(width() - clipSize, height());
        path.lineTo(clipSize, height());
        path.lineTo(0, height() - clipSize);
        path.lineTo(0, clipSize);
        path.closeSubpath();

        // Tło przycisku
        if (isDown()) {
            bgColor = QColor(0, 30, 45);
        } else if (underMouse()) {
            bgColor = QColor(0, 50, 75);
            borderColor = QColor(0, 200, 255);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        // Obramowanie
        painter.setPen(QPen(borderColor, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Tekst przycisku
        painter.setPen(QPen(textColor, 1));
        painter.setFont(QFont("Consolas", 10, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, text());
    }
};

class WavelengthChatView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)

public:
    explicit WavelengthChatView(QWidget *parent = nullptr)
        : QWidget(parent), m_scanlineOpacity(0.15) {
        setObjectName("chatViewContainer");

        // Podstawowy layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        // Nagłówek z dodatkowym efektem
        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);

        headerLabel = new QLabel(this);
        headerLabel->setObjectName("headerLabel");
        headerLayout->addWidget(headerLabel);

        // Wskaźnik status połączenia
        m_statusIndicator = new QLabel("AKTYWNE POŁĄCZENIE", this);
        m_statusIndicator->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_statusIndicator->setStyleSheet(
            "color: #00ffaa;"
            "background-color: rgba(0, 40, 30, 150);"
            "border: 1px solid #00aa77;"
            "padding: 4px 8px;"
            "font-family: 'Consolas';"
            "font-size: 9pt;"
        );
        headerLayout->addWidget(m_statusIndicator);

        mainLayout->addLayout(headerLayout);

        // Obszar wiadomości
        messageArea = new WavelengthStreamDisplay(this);
        messageArea->setStyleSheet(ChatStyle::getScrollBarStyle());
        mainLayout->addWidget(messageArea, 1);

        // Panel informacji technicznych
        QWidget *infoPanel = new QWidget(this);
        QHBoxLayout *infoLayout = new QHBoxLayout(infoPanel);
        infoLayout->setContentsMargins(3, 3, 3, 3);
        infoLayout->setSpacing(10);

        // Techniczne etykiety w stylu cyberpunk
        QString infoStyle =
                "color: #00aaff;"
                "background-color: transparent;"
                "font-family: 'Consolas';"
                "font-size: 8pt;";

        QLabel *sessionLabel = new QLabel(QString("SID:%1").arg(QRandomGenerator::global()->bounded(1000, 9999)), this);
        sessionLabel->setStyleSheet(infoStyle);
        infoLayout->addWidget(sessionLabel);

        QLabel *bufferLabel = new QLabel("BUFFER:100%", this);
        bufferLabel->setStyleSheet(infoStyle);
        infoLayout->addWidget(bufferLabel);

        infoLayout->addStretch();

        QLabel *timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm:ss"), this);
        timeLabel->setStyleSheet(infoStyle);
        infoLayout->addWidget(timeLabel);

        // Timer dla aktualnego czasu
        QTimer *timeTimer = new QTimer(this);
        connect(timeTimer, &QTimer::timeout, [timeLabel]() {
            timeLabel->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
        });
        timeTimer->start(1000);

        mainLayout->addWidget(infoPanel);

        // Panel wprowadzania wiadomości
        QHBoxLayout *inputLayout = new QHBoxLayout();
        inputLayout->setSpacing(10);

        // Przycisk załączania plików - niestandardowy wygląd
        attachButton = new CyberChatButton("📎", this);
        attachButton->setToolTip("Attach file");
        attachButton->setFixedSize(36, 36);
        inputLayout->addWidget(attachButton);

        // Pole wprowadzania wiadomości
        inputField = new QLineEdit(this);
        inputField->setPlaceholderText("Wpisz wiadomość...");
        inputField->setStyleSheet(ChatStyle::getInputFieldStyle());
        inputLayout->addWidget(inputField, 1);

        // Przycisk wysyłania - niestandardowy wygląd
        sendButton = new CyberChatButton("Wyślij", this);
        sendButton->setMinimumWidth(80);
        sendButton->setFixedHeight(36);
        inputLayout->addWidget(sendButton);

        mainLayout->addLayout(inputLayout);

        // Przycisk przerwania połączenia
        abortButton = new CyberChatButton("Zakończ połączenie", this);
        abortButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #662222;"
            "  color: #ffaaaa;"
            "  border: 1px solid #993333;"
            "  margin-top: 5px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #883333;"
            "  border: 1px solid #bb4444;"
            "}"
        );
        mainLayout->addWidget(abortButton);

        // Połączenia sygnałów
        connect(inputField, &QLineEdit::returnPressed, this, &WavelengthChatView::sendMessage);
        connect(sendButton, &QPushButton::clicked, this, &WavelengthChatView::sendMessage);
        connect(attachButton, &QPushButton::clicked, this, &WavelengthChatView::attachFile);
        connect(abortButton, &QPushButton::clicked, this, &WavelengthChatView::abortWavelength);

        WavelengthMessageService *messageService = WavelengthMessageService::getInstance();
        connect(messageService, &WavelengthMessageService::progressMessageUpdated,
                this, &WavelengthChatView::updateProgressMessage);
        connect(messageService, &WavelengthMessageService::removeProgressMessage,
                this, &WavelengthChatView::removeProgressMessage);

        // Timer dla efektów wizualnych
        QTimer *glitchTimer = new QTimer(this);
        connect(glitchTimer, &QTimer::timeout, this, &WavelengthChatView::triggerVisualEffect);
        glitchTimer->start(5000 + QRandomGenerator::global()->bounded(5000));

        setVisible(false);
    }

    double scanlineOpacity() const { return m_scanlineOpacity; }

    void setScanlineOpacity(double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    void attachFile() {
        QString filePath = QFileDialog::getOpenFileName(this,
                                                        "Select File to Attach",
                                                        QString(),
                                                        "Media Files (*.jpg *.jpeg *.png *.gif *.mp3 *.mp4 *.wav);;All Files (*)");

        if (filePath.isEmpty()) {
            return;
        }

        // Pobieramy nazwę pliku
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();

        // Generujemy identyfikator dla komunikatu
        QString progressMsgId = "file_progress_" + QString::number(QDateTime::currentMSecsSinceEpoch());

        // Wyświetlamy początkowy komunikat
        QString processingMsg = QString("<span style=\"color:#888888;\">Sending file: %1...</span>")
                .arg(fileName);

        messageArea->addMessage(processingMsg, progressMsgId, StreamMessage::MessageType::Transmitted);

        // Uruchamiamy asynchroniczny proces przetwarzania pliku
        WavelengthMessageService *service = WavelengthMessageService::getInstance();
        bool started = service->sendFileMessage(filePath, progressMsgId);

        if (!started) {
            messageArea->addMessage(progressMsgId,
                                    "<span style=\"color:#ff5555;\">Failed to start file processing.</span>",
                                    StreamMessage::MessageType::Transmitted);
        }
    }

    void setWavelength(double frequency, const QString &name = QString()) {
        currentFrequency = frequency;

        QString title;
        if (name.isEmpty()) {
            title = QString("Wavelength: %1 Hz").arg(frequency);
        } else {
            title = QString("%1 (%2 Hz)").arg(name).arg(frequency);
        }
        headerLabel->setText(title);

        messageArea->clear();

        QString welcomeMsg = QString("<span style=\"color:#ffcc00;\">Połączono z wavelength %1 Hz o %2</span>")
                .arg(frequency)
                .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
        messageArea->addMessage(welcomeMsg, "system", StreamMessage::MessageType::System);

        // Efekt wizualny przy połączeniu
        triggerConnectionEffect();

        setVisible(true);
        inputField->setFocus();
        inputField->clear();
    }

    void onMessageReceived(double frequency, const QString &message) {
        if (frequency != currentFrequency) {
            return;
        }

        // W przypadku bardzo dużych wiadomości, zapobiegaj zawieszeniom UI
        QTimer::singleShot(0, this, [this, message]() {
            messageArea->addMessage(message, "received", StreamMessage::MessageType::Received);
            // Mały efekt wizualny przy otrzymaniu wiadomości
            triggerActivityEffect();
        });
    }

    void onMessageSent(double frequency, const QString &message) {
        if (frequency != currentFrequency) {
            return;
        }

        messageArea->addMessage(message, "sent", StreamMessage::MessageType::Transmitted);
    }

    void onWavelengthClosed(double frequency) {
        if (frequency != currentFrequency) {
            return;
        }

        m_statusIndicator->setText("POŁĄCZENIE ZAMKNIĘTE");
        m_statusIndicator->setStyleSheet(
            "color: #ff5555;"
            "background-color: rgba(40, 0, 0, 150);"
            "border: 1px solid #aa3333;"
            "padding: 4px 8px;"
            "font-family: 'Consolas';"
            "font-size: 9pt;"
        );

        QString closeMsg = QString("<span style=\"color:#ff5555;\">Wavelength zostało zamknięte przez hosta.</span>");
        messageArea->addMessage(closeMsg, "system", StreamMessage::MessageType::System);

        QTimer::singleShot(2000, this, [this]() {
            clear();
            emit wavelengthAborted();
        });
    }

    void clear() {
        currentFrequency = -1;
        messageArea->clear();
        headerLabel->clear();
        inputField->clear();
        setVisible(false);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Rysowanie ramki w stylu AR/cyberpunk
        QColor borderColor(0, 170, 255);
        painter.setPen(QPen(borderColor, 1));

        // Zewnętrzna technologiczna ramka ze ściętymi rogami
        QPainterPath frame;
        int clipSize = 15;

        // Górna krawędź
        frame.moveTo(clipSize, 0);
        frame.lineTo(width() - clipSize, 0);

        // Prawy górny róg
        frame.lineTo(width(), clipSize);

        // Prawa krawędź
        frame.lineTo(width(), height() - clipSize);

        // Prawy dolny róg
        frame.lineTo(width() - clipSize, height());

        // Dolna krawędź
        frame.lineTo(clipSize, height());

        // Lewy dolny róg
        frame.lineTo(0, height() - clipSize);

        // Lewa krawędź
        frame.lineTo(0, clipSize);

        // Lewy górny róg
        frame.lineTo(clipSize, 0);

        painter.drawPath(frame);

        // Znaczniki AR w rogach
        painter.setPen(QPen(borderColor, 1, Qt::SolidLine));
        int markerSize = 8;

        // Lewy górny
        painter.drawLine(clipSize, 5, clipSize + markerSize, 5);
        painter.drawLine(clipSize, 5, clipSize, 5 + markerSize);

        // Prawy górny
        painter.drawLine(width() - clipSize - markerSize, 5, width() - clipSize, 5);
        painter.drawLine(width() - clipSize, 5, width() - clipSize, 5 + markerSize);

        // Prawy dolny
        painter.drawLine(width() - clipSize - markerSize, height() - 5, width() - clipSize, height() - 5);
        painter.drawLine(width() - clipSize, height() - 5, width() - clipSize, height() - 5 - markerSize);

        // Lewy dolny
        painter.drawLine(clipSize, height() - 5, clipSize + markerSize, height() - 5);
        painter.drawLine(clipSize, height() - 5, clipSize, height() - 5 - markerSize);

        // Efekt scanlines (jeśli aktywny)
        if (m_scanlineOpacity > 0.01) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 40 * m_scanlineOpacity));

            for (int y = 0; y < height(); y += 3) {
                painter.drawRect(0, y, width(), 1);
            }
        }
    }

private slots:
    void updateProgressMessage(const QString &messageId, const QString &message) {
        messageArea->addMessage(message, messageId, StreamMessage::MessageType::Transmitted);
    }

    void removeProgressMessage(const QString &messageId) {
        messageArea->addMessage("<span style=\"color:#ff5555);\">File transfer completed.</span>",
                                messageId, StreamMessage::MessageType::Transmitted);
    }

    void sendMessage() {
        QString message = inputField->text().trimmed();
        if (message.isEmpty()) {
            return;
        }

        inputField->clear();

        WavelengthMessageService *manager = WavelengthMessageService::getInstance();
        manager->sendMessage(message);
    }

    void abortWavelength() {
        if (currentFrequency == -1) {
            return;
        }

        m_statusIndicator->setText("ROZŁĄCZANIE...");
        m_statusIndicator->setStyleSheet(
            "color: #ffaa55;"
            "background-color: rgba(40, 20, 0, 150);"
            "border: 1px solid #aa7733;"
            "padding: 4px 8px;"
            "font-family: 'Consolas';"
            "font-size: 9pt;"
        );

        WavelengthSessionCoordinator *coordinator = WavelengthSessionCoordinator::getInstance();

        bool isHost = false;
        if (coordinator->getWavelengthInfo(currentFrequency, &isHost).isHost) {
            if (isHost) {
                coordinator->closeWavelength(currentFrequency);
            } else {
                coordinator->leaveWavelength();
            }
        } else {
            coordinator->leaveWavelength();
        }

        clear();

        QTimer::singleShot(250, this, [this]() {
            emit wavelengthAborted();
        });
    }

    void triggerVisualEffect() {
        // Losowy efekt scanlines co jakiś czas
        QPropertyAnimation *anim = new QPropertyAnimation(this, "scanlineOpacity");
        anim->setDuration(800);
        anim->setStartValue(m_scanlineOpacity);
        anim->setKeyValueAt(0.4, 0.3 + QRandomGenerator::global()->bounded(10) / 100.0);
        anim->setEndValue(0.15);
        anim->setEasingCurve(QEasingCurve::OutQuad);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }

    void triggerConnectionEffect() {
        // Efekt przy połączeniu
        QPropertyAnimation *anim = new QPropertyAnimation(this, "scanlineOpacity");
        anim->setDuration(1200);
        anim->setStartValue(0.6);
        anim->setEndValue(0.15);
        anim->setEasingCurve(QEasingCurve::OutQuad);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }

    void triggerActivityEffect() {
        // Subtelny efekt przy aktywności
        QPropertyAnimation *anim = new QPropertyAnimation(this, "scanlineOpacity");
        anim->setDuration(300);
        anim->setStartValue(m_scanlineOpacity);
        anim->setKeyValueAt(0.3, 0.25);
        anim->setEndValue(0.15);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }

signals:
    void wavelengthAborted();

private:
    QLabel *headerLabel;
    QLabel *m_statusIndicator;
    WavelengthStreamDisplay *messageArea;
    QLineEdit *inputField;
    QPushButton *attachButton;
    QPushButton *sendButton;
    QPushButton *abortButton;
    double currentFrequency = -1.0;
    double m_scanlineOpacity;
};

#endif // WAVELENGTH_CHAT_VIEW_H
