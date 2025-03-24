#ifndef WAVELENGTH_RADIO_DISPLAY_H
#define WAVELENGTH_RADIO_DISPLAY_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QScrollBar>
#include <QSoundEffect>
#include <QRandomGenerator>
#include <QOpenGLWidget>
#include <QProgressBar>

#include "../ui/messages/radio_message.h"
#include "../ui/messages/system_radio_message.h"
#include "../files/attachment_placeholder.h"

class WavelengthRadioDisplay : public QWidget {
    Q_OBJECT

public:
    QMap<QString, QWidget*> m_messageWidgets;

    WavelengthRadioDisplay(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Obszar przewijania
        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setStyleSheet("QScrollArea { background-color: #121212; border: none; }"
                                   "QScrollBar:vertical { background: #222222; width: 12px; }"
                                   "QScrollBar::handle:vertical { background: #444444; min-height: 20px; border-radius: 3px; }"
                                   "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");

        // Panel sterowania radiem
        m_controlPanel = new QFrame(this);
        m_controlPanel->setFixedHeight(60);
        m_controlPanel->setStyleSheet(
            "QFrame { "
            "   background-color: #0a0a12; "
            "   border-top: 1px solid #2d2d56; "
            "   border-image: none; "
            "}"
        );

        QHBoxLayout* controlLayout = new QHBoxLayout(m_controlPanel);

        // Wskaźnik siły sygnału - futurystyczny design
        m_signalIndicator = new QProgressBar(m_controlPanel);
        m_signalIndicator->setRange(0, 100);
        m_signalIndicator->setValue(0);
        m_signalIndicator->setTextVisible(false);
        m_signalIndicator->setFixedWidth(150);
        m_signalIndicator->setFixedHeight(8);
        m_signalIndicator->setStyleSheet(
            "QProgressBar { "
            "   background: #151525; "
            "   border: 1px solid #2a2a40; "
            "   border-radius: 0px; "
            "}"
            "QProgressBar::chunk { "
            "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
            "       stop:0 #ff00aa, stop:0.4 #9900ff, stop:1 #00ccff); "
            "}"
        );
        controlLayout->addWidget(m_signalIndicator);

        // Etykieta częstotliwości - neonowy cyberpunkowy styl
        m_frequencyLabel = new QLabel("FREQ: ---", m_controlPanel);
        m_frequencyLabel->setStyleSheet(
            "color: #00ccff; "
            "font-family: 'Orbitron', 'Electrolize', 'Consolas'; "
            "font-size: 14px; "
            "font-weight: bold; "
            "padding: 2px 10px; "
            "border: 1px solid #336699; "
            "background-color: #101020;"
        );
        controlLayout->addWidget(m_frequencyLabel);

        controlLayout->addStretch(1);

        // Etykieta statusu - cyfrowy wyświetlacz
        m_statusLabel = new QLabel("STANDBY", m_controlPanel);
        m_statusLabel->setStyleSheet(
            "color: #ffcc00; "
            "font-family: 'Orbitron', 'Electrolize', 'Consolas'; "
            "font-size: 14px; "
            "font-weight: bold; "
            "padding: 2px 10px; "
            "border: 1px solid #665500; "
            "background-color: #101020;"
        );
        controlLayout->addWidget(m_statusLabel);

        // Widget zawartości dla wiadomości - czarne tło z subtelną siatką
        m_contentWidget = new QWidget();
        m_contentWidget->setObjectName("radioContentWidget");
        m_contentWidget->setStyleSheet(
            "#radioContentWidget { "
            "   background-color: #080812; "
            "   background-image: url(:/images/grid_pattern.png); "
            "   background-repeat: repeat; "
            "}"
        );

        // Używamy QOpenGLWidget jako widżetu zawartości dla akceleracji GPU
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSamples(4); // Włącz MSAA dla wygładzania krawędzi

        QOpenGLWidget* openGLWidget = new QOpenGLWidget(m_contentWidget);
        openGLWidget->setFormat(format);

        m_contentLayout = new QVBoxLayout(m_contentWidget);
        m_contentLayout->setAlignment(Qt::AlignTop);
        m_contentLayout->setSpacing(6);
        m_contentLayout->setContentsMargins(10, 10, 10, 10);

        m_scrollArea->setWidget(m_contentWidget);

        mainLayout->addWidget(m_scrollArea);
        mainLayout->addWidget(m_controlPanel);

        // Inicjalizacja efektów dźwiękowych
        initSoundEffects();

        // Timer symulujący losowe zakłócenia
        m_interferenceTimer = new QTimer(this);
        connect(m_interferenceTimer, &QTimer::timeout, this, &WavelengthRadioDisplay::simulateRandomInterference);
        m_interferenceTimer->start(5000 + QRandomGenerator::global()->bounded(5000));
    }

    void setFrequency(double frequency) {
        m_frequencyLabel->setText(QString("FREQ: %1").arg(frequency));

        // Animujemy wskaźnik siły sygnału z efektem pulsacji
        QPropertyAnimation* anim = new QPropertyAnimation(m_signalIndicator, "value");
        anim->setDuration(1200);
        anim->setStartValue(0);
        anim->setKeyValueAt(0.2, 30);
        anim->setKeyValueAt(0.4, 60);
        anim->setKeyValueAt(0.7, 90);
        anim->setKeyValueAt(0.8, 75);
        anim->setEndValue(85);
        anim->setEasingCurve(QEasingCurve::OutBounce);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        // Ustawiamy status z efektem glitch
        m_statusLabel->setStyleSheet(
            "color: #00ff99; "
            "font-family: 'Orbitron', 'Electrolize', 'Consolas'; "
            "font-size: 14px; "
            "font-weight: bold; "
            "padding: 2px 10px; "
            "border: 1px solid #006633; "
            "background-color: #101020;"
        );

        // Efekt pojawienia się tekstu "CONNECTED"
        QSequentialAnimationGroup* textAnim = new QSequentialAnimationGroup(this);

        // Najpierw "CONN"
        QTimer::singleShot(200, this, [this]() {
            m_statusLabel->setText("CONN");
        });

        // Potem "CONNEC"
        QTimer::singleShot(400, this, [this]() {
            m_statusLabel->setText("CONNEC");
        });

        // Następnie "CONNECT"
        QTimer::singleShot(500, this, [this]() {
            m_statusLabel->setText("CONNECT");
        });

        // Na końcu "CONNECTED"
        QTimer::singleShot(600, this, [this]() {
            m_statusLabel->setText("CONNECTED");
        });

        // Odtwarzamy dźwięk połączenia
        m_connectSound->play();
    }

    void appendMessage(const QString& formattedMessage, const QString& messageId = QString()) {
        // Określamy typ wiadomości
        RadioMessage::MessageType messageType = RadioMessage::Received;
        bool isSystemMessage = false;

        if (formattedMessage.contains("<span style=\"color:#60ff8a;\">[You]:</span>")) {
            messageType = RadioMessage::Transmitted;

            // Dla wiadomości wychodzących odtwarzamy dźwięk nadawania
            m_sendBeepSound->play();

        } else if (formattedMessage.contains("<span style=\"color:#ffcc00;\">")) {
            isSystemMessage = true;

            // Dla wiadomości systemowych odtwarzamy inny dźwięk
            m_notificationSound->play();
        } else {
            // Dla wiadomości przychodzących odtwarzamy dźwięk odbioru
            m_receiveBeepSound->play();
        }

        QWidget* messageWidget = nullptr;

        // Obsługa wiadomości z załącznikami
        if (formattedMessage.contains("placeholder")) {
            if (formattedMessage.contains("video-placeholder")) {
                processMessageWithAttachment(formattedMessage, "video", messageId, messageType);
                return;
            } else if (formattedMessage.contains("audio-placeholder")) {
                processMessageWithAttachment(formattedMessage, "audio", messageId, messageType);
                return;
            } else if (formattedMessage.contains("gif-placeholder")) {
                processMessageWithAttachment(formattedMessage, "gif", messageId, messageType);
                return;
            } else if (formattedMessage.contains("image-placeholder")) {
                processMessageWithAttachment(formattedMessage, "image", messageId, messageType);
                return;
            }
        }

        // Obsługa wiadomości systemowej - wyświetlana jako tekst na środku
        if (isSystemMessage) {
            SystemRadioMessage* systemMsg = new SystemRadioMessage(formattedMessage, m_contentWidget);
            m_contentLayout->addWidget(systemMsg, 0, Qt::AlignHCenter);
            messageWidget = systemMsg;

            // Animacja pojawienia się
            QTimer::singleShot(30, systemMsg, &SystemRadioMessage::startEntryAnimation);
        } else {
            // Standardowa wiadomość tekstowa
            RadioMessage* radioMsg = new RadioMessage(formattedMessage, messageType, m_contentWidget);

            // Rozciągamy wiadomość na całą szerokość
            QHBoxLayout* wrapperLayout = new QHBoxLayout();
            wrapperLayout->setContentsMargins(0, 0, 0, 0);
            wrapperLayout->addWidget(radioMsg);

            QWidget* wrapper = new QWidget(m_contentWidget);
            wrapper->setLayout(wrapperLayout);

            m_contentLayout->addWidget(wrapper);
            messageWidget = wrapper;

            // Animacja pojawienia się
            QTimer::singleShot(30, radioMsg, &RadioMessage::startEntryAnimation);

            // Aktualizujemy wskaźnik siły sygnału
            updateSignalIndicator(messageType);
        }

        // Jeśli podano ID, zapisz referencję do widgetu
        if (!messageId.isEmpty()) {
            if (m_messageWidgets.contains(messageId)) {
                QWidget* oldMessage = m_messageWidgets[messageId];
                m_contentLayout->removeWidget(oldMessage);
                delete oldMessage;
            }
            m_messageWidgets[messageId] = messageWidget;
        }

        // Przewiń do najnowszej wiadomości
        QTimer::singleShot(100, this, &WavelengthRadioDisplay::scrollToBottom);
    }

    // Metoda do zastępowania wiadomości według ID
    void replaceMessage(const QString& messageId, const QString& newMessage) {
        if (messageId.isEmpty() || !m_messageWidgets.contains(messageId)) {
            // Jeśli nie istnieje, dodaj nową wiadomość
            appendMessage(newMessage);
            return;
        }

        QWidget* oldMessage = m_messageWidgets[messageId];

        // Usuwamy stary widget
        m_contentLayout->removeWidget(oldMessage);
        int index = m_contentLayout->indexOf(oldMessage);
        delete oldMessage;

        // Dodajemy nową wiadomość
        appendMessage(newMessage, messageId);

        // Przewijamy do najnowszej wiadomości
        QTimer::singleShot(100, this, &WavelengthRadioDisplay::scrollToBottom);
    }

    // Metoda do usuwania wiadomości według ID
    void removeMessage(const QString& messageId) {
        if (messageId.isEmpty() || !m_messageWidgets.contains(messageId)) {
            return;
        }

        QWidget* message = m_messageWidgets[messageId];
        m_contentLayout->removeWidget(message);
        m_messageWidgets.remove(messageId);
        delete message;
    }

    void clear() {
        QLayoutItem* item;
        while ((item = m_contentLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        m_messageWidgets.clear();

        // Resetujemy panel kontrolny
        m_frequencyLabel->setText("FREQ: ---");
        m_statusLabel->setText("STANDBY");
        m_statusLabel->setStyleSheet("color: #ffcc00; font-family: 'Digital-7', 'Courier New'; font-size: 14px;");
        m_signalIndicator->setValue(0);

        // Odtwarzamy dźwięk rozłączenia
        m_disconnectSound->play();
    }

public slots:
    void scrollToBottom() {
        // Poprawiona metoda przewijania do końca
        QScrollBar* vbar = m_scrollArea->verticalScrollBar();
        if (vbar) {
            vbar->setValue(vbar->maximum());
        }
    }

private slots:
    void simulateRandomInterference() {
        // Losowe zakłócenia dla wszystkich widgetów
        if (m_messageWidgets.isEmpty()) return;

        QList<QWidget*> messages = m_messageWidgets.values();
        int index = QRandomGenerator::global()->bounded(messages.size());
        QWidget* randomWidget = messages.at(index);

        // Sprawdzamy czy widget jest wrapperem dla RadioMessage
        RadioMessage* radioMsg = nullptr;

        // Spróbuj znaleźć RadioMessage w hierarchii widgetów
        if (randomWidget->layout()) {
            QLayoutItem* item = randomWidget->layout()->itemAt(0);
            if (item && item->widget()) {
                radioMsg = qobject_cast<RadioMessage*>(item->widget());
            }
        }

        if (!radioMsg) {
            radioMsg = randomWidget->findChild<RadioMessage*>();
        }

        if (radioMsg) {
            // Generuj losową siłę zakłóceń (0.1-0.5)
            qreal severity = 0.1 + QRandomGenerator::global()->bounded(40) / 100.0;

            // Symuluj zakłócenia
            radioMsg->simulateGlitch(severity);

            // Odtwarzaj dźwięk zakłóceń
            m_staticSound->play();

            // Mały spadek siły sygnału
            int currentSignal = m_signalIndicator->value();
            m_signalIndicator->setValue(qMax(currentSignal - 15, 30));

            // Odzyskiwanie siły sygnału po zakłóceniach
            QTimer::singleShot(1500, this, [this, currentSignal]() {
                QPropertyAnimation* anim = new QPropertyAnimation(m_signalIndicator, "value");
                anim->setDuration(1000);
                anim->setStartValue(m_signalIndicator->value());
                anim->setEndValue(currentSignal);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            });
        }

        // Zmień interwał losowo
        m_interferenceTimer->setInterval(5000 + QRandomGenerator::global()->bounded(10000));
    }

private:
    void processMessageWithAttachment(const QString& message, const QString& type,
                                      const QString& messageId, RadioMessage::MessageType messageType) {
        // Ekstrahujemy części wiadomości z placeholderem
        int placeholderStart = message.indexOf("<div class='" + type + "-placeholder'");
        if (placeholderStart >= 0) {
            // Przetwarzamy część przed placeholderem jako wiadomość radiową
            RadioMessage* headerMessage = new RadioMessage(
                message.left(placeholderStart),
                messageType,
                m_contentWidget
            );

            // Rozciągamy wiadomość na całą szerokość
            QHBoxLayout* headerLayout = new QHBoxLayout();
            headerLayout->setContentsMargins(0, 0, 0, 0);
            headerLayout->addWidget(headerMessage);

            QWidget* headerWrapper = new QWidget(m_contentWidget);
            headerWrapper->setLayout(headerLayout);

            m_contentLayout->addWidget(headerWrapper);

            // Zapisujemy referencję do headera jeśli podano ID
            if (!messageId.isEmpty()) {
                m_messageWidgets[messageId] = headerWrapper;
            }

            headerMessage->startEntryAnimation();

            // Dodajemy placeholder załącznika
            QString attachmentId = extractAttribute(message, "data-attachment-id");
            QString filename = extractAttribute(message, "data-filename");

            AttachmentPlaceholder* attachPlaceholder = new AttachmentPlaceholder(
                filename, type, this, false);
            attachPlaceholder->setAttachmentReference(attachmentId,
                extractAttribute(message, "data-mime-type"));

            // Stylizacja placeholdera
            attachPlaceholder->setStyleSheet("background-color: #1a1a1a; border: 1px solid #333333;");

            m_contentLayout->addWidget(attachPlaceholder);

            // Jeśli są dane po placeholderze, dodajemy je jako osobną wiadomość
            int placeholderEnd = message.indexOf("</div>", placeholderStart);
            if (placeholderEnd > 0 && placeholderEnd < message.length() - 6) {
                RadioMessage* footerMessage = new RadioMessage(
                    message.mid(placeholderEnd + 6),
                    messageType,
                    m_contentWidget
                );

                // Rozciągamy wiadomość na całą szerokość
                QHBoxLayout* footerLayout = new QHBoxLayout();
                footerLayout->setContentsMargins(0, 0, 0, 0);
                footerLayout->addWidget(footerMessage);

                QWidget* footerWrapper = new QWidget(m_contentWidget);
                footerWrapper->setLayout(footerLayout);

                m_contentLayout->addWidget(footerWrapper);
                footerMessage->startEntryAnimation();
            }

            // Odtwarzamy dźwięk załącznika
            m_attachmentSound->play();
        }
    }
    
    void updateSignalIndicator(RadioMessage::MessageType messageType) {
        int currentSignal = m_signalIndicator->value();
        
        // Dla wiadomości wychodzących silny sygnał, dla przychodzących może być słabszy
        if (messageType == RadioMessage::Transmitted) {
            // Pełna siła dla nadawania
            QPropertyAnimation* anim = new QPropertyAnimation(m_signalIndicator, "value");
            anim->setDuration(300);
            anim->setStartValue(currentSignal);
            anim->setEndValue(95);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            
            m_statusLabel->setText("TRANSMITTING");
            m_statusLabel->setStyleSheet("color: #ff3333; font-family: 'Digital-7', 'Courier New'; font-size: 14px;");
            
            // Po krótkim czasie wracamy do statusu "Connected"
            QTimer::singleShot(1000, this, [this]() {
                m_statusLabel->setText("CONNECTED");
                m_statusLabel->setStyleSheet("color: #33ff33; font-family: 'Digital-7', 'Courier New'; font-size: 14px;");
            });
        } 
        else if (messageType == RadioMessage::Received) {
            // Dla odbioru, możliwy lekki spadek sygnału przed odbiorem
            QPropertyAnimation* anim = new QPropertyAnimation(m_signalIndicator, "value");
            anim->setDuration(800);
            anim->setStartValue(qMax(currentSignal - 15, 40));
            anim->setKeyValueAt(0.3, qMax(currentSignal - 5, 60));
            anim->setEndValue(85);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
            
            m_statusLabel->setText("RECEIVING");
            m_statusLabel->setStyleSheet("color: #33ccff; font-family: 'Digital-7', 'Courier New'; font-size: 14px;");
            
            // Po krótkim czasie wracamy do statusu "Connected"
            QTimer::singleShot(1000, this, [this]() {
                m_statusLabel->setText("CONNECTED");
                m_statusLabel->setStyleSheet("color: #33ff33; font-family: 'Digital-7', 'Courier New'; font-size: 14px;");
            });
        }
    }
    
    // Inicjalizacja efektów dźwiękowych
    void initSoundEffects() {
        m_sendBeepSound = new QSoundEffect(this);
        m_sendBeepSound->setSource(QUrl("qrc:/sounds/radio_send.wav"));
        m_sendBeepSound->setVolume(0.5);
        
        m_receiveBeepSound = new QSoundEffect(this);
        m_receiveBeepSound->setSource(QUrl("qrc:/sounds/radio_receive.wav"));
        m_receiveBeepSound->setVolume(0.5);
        
        m_staticSound = new QSoundEffect(this);
        m_staticSound->setSource(QUrl("qrc:/sounds/radio_static.wav"));
        m_staticSound->setVolume(0.3);
        
        m_connectSound = new QSoundEffect(this);
        m_connectSound->setSource(QUrl("qrc:/sounds/radio_connect.wav"));
        m_connectSound->setVolume(0.7);
        
        m_disconnectSound = new QSoundEffect(this);
        m_disconnectSound->setSource(QUrl("qrc:/sounds/radio_disconnect.wav"));
        m_disconnectSound->setVolume(0.7);
        
        m_notificationSound = new QSoundEffect(this);
        m_notificationSound->setSource(QUrl("qrc:/sounds/radio_notification.wav"));
        m_notificationSound->setVolume(0.4);
        
        m_attachmentSound = new QSoundEffect(this);
        m_attachmentSound->setSource(QUrl("qrc:/sounds/radio_attachment.wav"));
        m_attachmentSound->setVolume(0.6);
    }
    
    // Metoda pomocnicza do wyciągania atrybutów z HTML
    QString extractAttribute(const QString& html, const QString& attribute) {
        int attrPos = html.indexOf(attribute + "='");
        if (attrPos >= 0) {
            attrPos += attribute.length() + 2; // przesunięcie za ='
            int endPos = html.indexOf("'", attrPos);
            if (endPos >= 0) {
                return html.mid(attrPos, endPos - attrPos);
            }
        }
        return QString();
    }

    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    
    QFrame* m_controlPanel;
    QLabel* m_frequencyLabel;
    QLabel* m_statusLabel;
    QProgressBar* m_signalIndicator;
    
    QTimer* m_interferenceTimer;
    
    // Efekty dźwiękowe
    QSoundEffect* m_sendBeepSound;
    QSoundEffect* m_receiveBeepSound;
    QSoundEffect* m_staticSound;
    QSoundEffect* m_connectSound;
    QSoundEffect* m_disconnectSound;
    QSoundEffect* m_notificationSound;
    QSoundEffect* m_attachmentSound;
};

#endif // WAVELENGTH_RADIO_DISPLAY_H