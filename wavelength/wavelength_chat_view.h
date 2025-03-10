#ifndef WAVELENGTH_CHAT_VIEW_H
#define WAVELENGTH_CHAT_VIEW_H

#include <QWidget>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include "wavelength_manager.h"
#include <QScrollBar>

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
        mainLayout->addWidget(messageArea, 1);

        QHBoxLayout *inputLayout = new QHBoxLayout();

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
        connect(abortButton, &QPushButton::clicked, this, &WavelengthChatView::abortWavelength);

        WavelengthManager* manager = WavelengthManager::getInstance();
        connect(manager, &WavelengthManager::messageReceived, this, &WavelengthChatView::onMessageReceived);
        connect(manager, &WavelengthManager::wavelengthClosed, this, &WavelengthChatView::onWavelengthClosed);
    }

    void setWavelength(int frequency, const QString& name = QString()) {
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

        WavelengthManager* manager = WavelengthManager::getInstance();
        manager->sendMessage(message);

    }

    void abortWavelength() {
        if (currentFrequency == -1) {
            return;
        }

        qDebug() << "===== ABORT SEQUENCE START (UI) =====";
        qDebug() << "Aborting wavelength" << currentFrequency;

        WavelengthManager* manager = WavelengthManager::getInstance();

        bool isHost = false;
        if (manager->getWavelengthInfo(currentFrequency, &isHost)) {
            qDebug() << "Current user is host:" << isHost;

            if (isHost) {
                qDebug() << "Calling closeWavelength() as host";
                manager->closeWavelength(currentFrequency);
            } else {
                qDebug() << "Calling leaveWavelength() as client";
                manager->leaveWavelength();
            }
        } else {
            qDebug() << "Could not determine role, calling leaveWavelength()";
            manager->leaveWavelength();
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

    void onMessageReceived(int frequency, const QString& message) {
        if (frequency != currentFrequency) {
            return;
        }

        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        QString formattedMessage;

        if (message.startsWith("<span")) {
            formattedMessage = message;
        } else {
            formattedMessage = QString("<span style=\"color:#e0e0e0;\">[%1] %2</span>")
                .arg(timestamp, message);
        }

        messageArea->append(formattedMessage);

        messageArea->verticalScrollBar()->setValue(messageArea->verticalScrollBar()->maximum());
    }

    void onWavelengthClosed(int frequency) {
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

signals:
    void wavelengthAborted();

private:
    QLabel *headerLabel;
    QTextEdit *messageArea;
    QLineEdit *inputField;
    QPushButton *sendButton;
    QPushButton *abortButton;
    int currentFrequency = -1;
    void addMessage(const QString& message) {
        qDebug() << "Adding message to chat:" << message;
        messageArea->append(message);
    }
};

#endif // WAVELENGTH_CHAT_VIEW_H