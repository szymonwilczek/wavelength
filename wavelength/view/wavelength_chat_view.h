#ifndef WAVELENGTH_CHAT_VIEW_H
#define WAVELENGTH_CHAT_VIEW_H

#include <qfileinfo.h>
#include <QWidget>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QFileDialog>

#include "../session/wavelength_session_coordinator.h"

class WavelengthChatView : public QWidget {
    Q_OBJECT

public:
    explicit WavelengthChatView(QWidget *parent = nullptr) : QWidget(parent) {
        setStyleSheet("background-color: #2d2d2d; color: #e0e0e0;");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        headerLabel = new QLabel(this);
        headerLabel->setStyleSheet("font-size: 16pt; color: #a0a0a0; margin-bottom: 10px;");
        mainLayout->addWidget(headerLabel);

        messageArea = new QTextEdit(this);
        messageArea->setReadOnly(true);
        messageArea->setStyleSheet(
            "QTextEdit {"
            "  background-color: #232323;"
            "  border: 1px solid #3a3a3a;"
            "  border-radius: 5px;"
            "  padding: 10px;"
            "  color: #e0e0e0;"
            "  font-size: 11pt;"
            "}"
        );
        messageArea->setAcceptRichText(true);
        messageArea->document()->setDefaultStyleSheet(
         "img { max-width: 300px; max-height: 200px; } "
         "audio, video { max-width: 300px; } "
         "span.attachment-name { color: #aaaaaa; font-size: 9pt; }"
     );
        mainLayout->addWidget(messageArea, 1);

        messageArea->document()->setMaximumBlockCount(1000);

        messageArea->setTextInteractionFlags(Qt::TextBrowserInteraction);

        QHBoxLayout *inputLayout = new QHBoxLayout();

        attachButton = new QPushButton(this);
        attachButton->setToolTip("Attach file");
        attachButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #333333;"
            "  border: 1px solid #444444;"
            "  border-radius: 5px;"
            "  padding: 8px;"
            "}"
            "QPushButton:hover { background-color: #444444; }"
            "QPushButton:pressed { background-color: #2a2a2a; }"
        );
        attachButton->setFixedSize(36, 36);
        inputLayout->addWidget(attachButton);

        inputField = new QLineEdit(this);
        inputField->setPlaceholderText("Enter message...");
        inputField->setStyleSheet(
            "QLineEdit {"
            "  background-color: #333333;"
            "  border: 1px solid #444444;"
            "  border-radius: 5px;"
            "  padding: 8px;"
            "  color: #e0e0e0;"
            "  font-size: 11pt;"
            "}"
        );
        inputLayout->addWidget(inputField, 1);

        sendButton = new QPushButton("Send", this);
        sendButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #4a6db5;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 5px;"
            "  padding: 8px 16px;"
            "}"
            "QPushButton:hover { background-color: #5a7dc5; }"
            "QPushButton:pressed { background-color: #3a5da5; }"
            "QPushButton:disabled { background-color: #2c3e66; color: #aaaaaa; }"
        );
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
    }

    void attachFile() {
        QString filePath = QFileDialog::getOpenFileName(this,
            "Select File to Attach",
            QString(),
            "Media Files (*.jpg *.jpeg *.png *.gif *.mp3 *.mp4 *.wav);;All Files (*)");

        if (filePath.isEmpty()) {
            return;
        }

        QFileInfo fileInfo(filePath);
        // if (fileInfo.size() > 10 * 1024 * 1024) { // Ograniczamy do 10MB
        //     QMessageBox::warning(this, "File too large",
        //         "The selected file exceeds the maximum size limit (10MB).");
        //     return;
        // }

        // Dodaj informację o wysyłaniu pliku
        QString processingMsg = QString("<span style=\"color:#888888;\">Sending file: %1...</span>")
            .arg(fileInfo.fileName());
        messageArea->append(processingMsg);

        // Opóźnij wysyłanie, aby dać czas na wyświetlenie komunikatu
        QTimer::singleShot(100, this, [this, filePath]() {
            WavelengthMessageService* manager = WavelengthMessageService::getInstance();
            if(manager->sendFileMessage(filePath)) {
                qDebug() << "File sent successfully";
            } else {
                messageArea->append("<span style=\"color:#ff5555;\">Failed to send file.</span>");
            }
        });
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
        messageArea->append(welcomeMsg);

        setVisible(true);

        inputField->setFocus();
        inputField->clear();
    }

    void onMessageReceived(double frequency, const QString& message) {
        if (frequency != currentFrequency) {
            return;
        }

        qDebug() << "Received message for display, HTML length:" << message.length();

        // W przypadku bardzo dużych wiadomości (z załącznikami), zapobiegaj zawieszeniom UI
        QTimer::singleShot(0, this, [this, message]() {
            messageArea->append(message);
            messageArea->verticalScrollBar()->setValue(messageArea->verticalScrollBar()->maximum());
        });
    }

    void onMessageSent(double frequency, const QString& message) {
        if (frequency != currentFrequency) {
            return;
        }


        messageArea->append(message);

        messageArea->verticalScrollBar()->setValue(messageArea->verticalScrollBar()->maximum());
    }

    void onWavelengthClosed(double frequency) {
        if (frequency != currentFrequency) {
            return;
        }

        QString closeMsg = QString("<span style=\"color:#ff5555;\">Wavelength has been closed by the host.</span>");
        messageArea->append(closeMsg);

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
    QTextEdit *messageArea;
    QLineEdit *inputField;
    QPushButton *attachButton;
    QList<QFile> attachedFiles;
    QPushButton *sendButton;
    QPushButton *abortButton;
    double currentFrequency = -1.0;
};

#endif // WAVELENGTH_CHAT_VIEW_H