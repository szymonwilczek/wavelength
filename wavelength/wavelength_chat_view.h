#ifndef WAVELENGTH_CHAT_VIEW_H
#define WAVELENGTH_CHAT_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QDateTime>
#include <QDebug>
#include <QTimer>

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

        sendButton = new QPushButton("Send", this);
        sendButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #3e3e3e;"
            "  color: #e0e0e0;"
            "  border: 1px solid #4a4a4a;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "}"
            "QPushButton:hover { background-color: #4a4a4a; }"
            "QPushButton:pressed { background-color: #2e2e2e; }"
        );

        inputLayout->addWidget(inputField, 1);
        inputLayout->addWidget(sendButton);

        mainLayout->addLayout(inputLayout);

        abortButton = new QPushButton("Abort Wavelength", this);
        abortButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #3a3a3a;"
            "  color: #e0e0e0;"
            "  border: 1px solid #4a4a4a;"
            "  border-radius: 4px;"
            "  padding: 10px 20px;"
            "  margin-top: 10px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #555555; }"
            "QPushButton:pressed { background-color: #2e2e2e; }"
        );

        mainLayout->addWidget(abortButton, 0, Qt::AlignCenter);

        connect(sendButton, &QPushButton::clicked, this, &WavelengthChatView::sendMessage);
        connect(inputField, &QLineEdit::returnPressed, this, &WavelengthChatView::sendMessage);
        connect(abortButton, &QPushButton::clicked, this, &WavelengthChatView::abortWavelength);
    }

    void setWavelength(int frequency, const QString& name = QString()) {
        currentFrequency = frequency;
        QString displayName = name.isEmpty() ? QString::number(frequency) + " Hz" : name;
        headerLabel->setText("Wavelength: " + displayName);
        qDebug() << "Chat view set to frequency:" << frequency << (name.isEmpty() ? "" : "(" + name + ")");
    }

    void clear() {
        messageArea->clear();
        inputField->clear();
    }

private slots:
    void sendMessage() {
        QString message = inputField->text().trimmed();
        if (message.isEmpty()) {
            return;
        }

        qDebug() << "Sending message on wavelength" << currentFrequency << ":" << message;

        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        QString formattedMessage = QString("<b>[%1] You:</b> %2").arg(timestamp).arg(message);
        messageArea->append(formattedMessage);

        QTimer::singleShot(500, this, [this, message]() {
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
            QString formattedMessage = QString("<b>[%1] Echo:</b> %2").arg(timestamp).arg(message);
            messageArea->append(formattedMessage);
        });

        inputField->clear();
    }

    void abortWavelength() {
        qDebug() << "Aborting wavelength" << currentFrequency;
        clear();
        emit wavelengthAborted();
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
};

#endif // WAVELENGTH_CHAT_VIEW_H