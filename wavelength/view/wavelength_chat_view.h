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

#include "../session/wavelength_session_coordinator.h"
#include "../messages/wavelength_text_display.h"

class WavelengthChatView : public QWidget {
    Q_OBJECT

public:
    explicit WavelengthChatView(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("chatViewContainer");

        setStyleSheet(ChatStyle::getChatStyleSheet());

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        headerLabel = new QLabel(this);
        headerLabel->setObjectName("headerLabel");
        mainLayout->addWidget(headerLabel);

        // Zastępujemy QTextEdit naszą nową klasą
        messageArea = new WavelengthTextDisplay(this);
        mainLayout->addWidget(messageArea, 1);

        QHBoxLayout *inputLayout = new QHBoxLayout();

        attachButton = new QPushButton(this);
        attachButton->setToolTip("Attach file");
        attachButton->setStyleSheet(ChatStyle::getAttachButtonStyle());
        attachButton->setFixedSize(36, 36);
        inputLayout->addWidget(attachButton);

        inputField = new QLineEdit(this);
        inputField->setPlaceholderText("Enter message...");
        inputField->setStyleSheet(ChatStyle::getInputFieldStyle());
        inputLayout->addWidget(inputField, 1);

        sendButton = new QPushButton("Send", this);
        sendButton->setStyleSheet(ChatStyle::getSendButtonStyle());
        inputLayout->addWidget(sendButton);

        mainLayout->addLayout(inputLayout);

        abortButton = new QPushButton("Abort Wavelength", this);
        abortButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #b54a4a;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 5px;"
            "  padding: 8px 16px;"
            "  margin-top: 10px;"
            "}"
            "QPushButton:hover { background-color: #c55a5a; }"
            "QPushButton:pressed { background-color: #a53a3a; }"
            "QPushButton:disabled { background-color: #662c2c; color: #aaaaaa; }"
        );
        mainLayout->addWidget(abortButton);

        setVisible(false);

        connect(inputField, &QLineEdit::returnPressed, this, &WavelengthChatView::sendMessage);
        connect(sendButton, &QPushButton::clicked, this, &WavelengthChatView::sendMessage);
        connect(attachButton, &QPushButton::clicked, this, &WavelengthChatView::attachFile);
        connect(abortButton, &QPushButton::clicked, this, &WavelengthChatView::abortWavelength);

        WavelengthMessageService* messageService = WavelengthMessageService::getInstance();
        connect(messageService, &WavelengthMessageService::progressMessageUpdated,
                this, &WavelengthChatView::updateProgressMessage);
        connect(messageService, &WavelengthMessageService::removeProgressMessage,
                this, &WavelengthChatView::removeProgressMessage);
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
        messageArea->appendMessage(processingMsg, progressMsgId);

        // Uruchamiamy asynchroniczny proces przetwarzania pliku
        WavelengthMessageService* service = WavelengthMessageService::getInstance();
        bool started = service->sendFileMessage(filePath, progressMsgId);

        if (!started) {
            messageArea->replaceMessage(progressMsgId,
                "<span style=\"color:#ff5555;\">Failed to start file processing.</span>");
        }
    }

    void setWavelength(double frequency, const QString& name = QString()) {
        currentFrequency = frequency;

        QString title;
        if (name.isEmpty()) {
            title = QString("Wavelength: %1 Hz").arg(frequency);
        } else {
            title = QString("%1 (%2 Hz)").arg(name).arg(frequency);
        }
        headerLabel->setText(title);

        messageArea->clear();

        QString welcomeMsg = QString("<span style=\"color:#888888;\">Connected to wavelength %1 Hz at %2</span>")
            .arg(frequency)
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
        messageArea->appendMessage(welcomeMsg);

        setVisible(true);

        inputField->setFocus();
        inputField->clear();
    }

    void onMessageReceived(double frequency, const QString& message) {
        qDebug() << "onMessageReceived called - frequency:" << frequency
                 << "Current frequency:" << currentFrequency;

        if (frequency != currentFrequency) {
            qDebug() << "Ignoring message for different frequency";
            return;
        }

        qDebug() << "Received message for display, HTML length:" << message.length();
        qDebug() << "Message preview:" << message.left(100);

        // W przypadku bardzo dużych wiadomości (z załącznikami), zapobiegaj zawieszeniom UI
        QTimer::singleShot(0, this, [this, message]() {
            messageArea->appendMessage(message);
            qDebug() << "Message added to display";
        });
    }

    void onMessageSent(double frequency, const QString& message) {
        if (frequency != currentFrequency) {
            return;
        }

        messageArea->appendMessage(message);
    }

    void onWavelengthClosed(double frequency) {
        if (frequency != currentFrequency) {
            return;
        }

        QString closeMsg = QString("<span style=\"color:#ff5555;\">Wavelength has been closed by the host.</span>");
        messageArea->appendMessage(closeMsg);

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

private slots:

    void updateProgressMessage(const QString& messageId, const QString& message) {
        messageArea->replaceMessage(messageId, message);
    }

    void removeProgressMessage(const QString& messageId) {
        messageArea->removeMessage(messageId);
    }

    void sendMessage() {
        QString message = inputField->text().trimmed();
        if (message.isEmpty()) {
            return;
        }

        inputField->clear();

        WavelengthMessageService* manager = WavelengthMessageService::getInstance();
        manager->sendMessage(message);

    }

    void abortWavelength() {
        if (currentFrequency == -1) {
            return;
        }

        qDebug() << "===== ABORT SEQUENCE START (UI) =====";
        qDebug() << "Aborting wavelength" << currentFrequency;

        WavelengthSessionCoordinator* coordinator = WavelengthSessionCoordinator::getInstance();

        bool isHost = false;
        if (coordinator->getWavelengthInfo(currentFrequency, &isHost).isHost) {
            qDebug() << "Current user is host:" << isHost;

            if (isHost) {
                qDebug() << "Calling closeWavelength() as host";
                coordinator->closeWavelength(currentFrequency);
            } else {
                qDebug() << "Calling leaveWavelength() as client";
                coordinator->leaveWavelength();
            }
        } else {
            qDebug() << "Could not determine role, calling leaveWavelength()";
            coordinator->leaveWavelength();
        }

        qDebug() << "Manager operation completed, clearing UI";
        clear();

        qDebug() << "Clear completed, delaying wavelengthAborted signal emission";

        QTimer::singleShot(250, this, [this]() {
            qDebug() << "Emitting delayed wavelengthAborted signal";
            emit wavelengthAborted();
            qDebug() << "===== ABORT SEQUENCE COMPLETE =====";
        });
    }

signals:
    void wavelengthAborted();

private:
    QLabel *headerLabel;
    WavelengthTextDisplay *messageArea;  // Zmieniony typ
    QLineEdit *inputField;
    QPushButton *attachButton;
    QList<QFile> attachedFiles;
    QPushButton *sendButton;
    QPushButton *abortButton;
    double currentFrequency = -1.0;
};

#endif // WAVELENGTH_CHAT_VIEW_H