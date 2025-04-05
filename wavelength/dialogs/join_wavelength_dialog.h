#ifndef JOIN_WAVELENGTH_DIALOG_H
#define JOIN_WAVELENGTH_DIALOG_H

#include <QApplication>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include <QPainter>
#include <QElapsedTimer>
#include <QPropertyAnimation>
#include <QPainterPath>
#include <QGraphicsOpacityEffect>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QRandomGenerator>
#include <QTimer>

#include "wavelength_dialog.h"
#include "../session/wavelength_session_coordinator.h"
#include "../ui/dialogs/animated_dialog.h"

class JoinWavelengthDialog : public AnimatedDialog {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
    Q_PROPERTY(double digitalizationProgress READ digitalizationProgress WRITE setDigitalizationProgress)
    Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)
    Q_PROPERTY(double glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)

public:
    explicit JoinWavelengthDialog(QWidget *parent = nullptr)
        : AnimatedDialog(parent, AnimatedDialog::DigitalMaterialization),
          m_shadowSize(10), m_scanlineOpacity(0.08)
    {
        // Dodaj flagi optymalizacyjne
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_StaticContents, true);
        setAttribute(Qt::WA_ContentsPropagated, false);

        // Preferuj akcelerację graficzną
        // QSurfaceFormat format;
        // format.setRenderableType(QSurfaceFormat::OpenGL);
        // QSurfaceFormat::setDefaultFormat(format);

        m_glitchLines = QList<int>();
        m_digitalizationProgress = 0.0;
        m_animationStarted = false;

        setWindowTitle("JOIN_WAVELENGTH::CONNECT");
        setModal(true);
        setFixedSize(450, 350);
        setAnimationDuration(400);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(12);

        // Nagłówek z tytułem
        QLabel *titleLabel = new QLabel("JOIN WAVELENGTH", this);
        titleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        titleLabel->setContentsMargins(0, 0, 0, 3);
        mainLayout->addWidget(titleLabel);

        // Panel informacyjny z ID sesji
        QWidget *infoPanel = new QWidget(this);
        QHBoxLayout *infoPanelLayout = new QHBoxLayout(infoPanel);
        infoPanelLayout->setContentsMargins(0, 0, 0, 0);
        infoPanelLayout->setSpacing(5);

        QString sessionId = QString("%1-%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10000, 99999));
        QLabel *sessionLabel = new QLabel(QString("SESSION_ID: %1").arg(sessionId), this);
        sessionLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        QLabel *timeLabel = new QLabel(QString("TS: %1").arg(timestamp), this);
        timeLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

        infoPanelLayout->addWidget(sessionLabel);
        infoPanelLayout->addStretch();
        infoPanelLayout->addWidget(timeLabel);
        mainLayout->addWidget(infoPanel);

        // Panel instrukcji
        QLabel *infoLabel = new QLabel("Enter the wavelength frequency (130Hz - 180MHz)", this);
        infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
        infoLabel->setAlignment(Qt::AlignLeft);
        infoLabel->setWordWrap(true);
        mainLayout->addWidget(infoLabel);

        // Formularz
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setSpacing(12);
        formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        formLayout->setContentsMargins(0, 15, 0, 15);

        // Etykieta częstotliwości
        QLabel* frequencyTitleLabel = new QLabel("FREQUENCY:", this);
        frequencyTitleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

        // Kontener dla pola częstotliwości i selecta jednostki
        QWidget* freqContainer = new QWidget(this);
        QHBoxLayout* freqLayout = new QHBoxLayout(freqContainer);
        freqLayout->setContentsMargins(0, 0, 0, 0);
        freqLayout->setSpacing(5);

        // Pole wprowadzania częstotliwości
        frequencyEdit = new CyberLineEdit(this);
        QDoubleValidator* validator = new QDoubleValidator(130, 180000000.0, 1, this);
        QLocale locale(QLocale::English);
        locale.setNumberOptions(QLocale::RejectGroupSeparator);
        validator->setLocale(locale);
        frequencyEdit->setValidator(validator);
        frequencyEdit->setPlaceholderText("ENTER FREQUENCY");
        freqLayout->addWidget(frequencyEdit, 3);

        // ComboBox z jednostkami
        frequencyUnitCombo = new QComboBox(this);
        frequencyUnitCombo->addItem("Hz");
        frequencyUnitCombo->addItem("kHz");
        frequencyUnitCombo->addItem("MHz");
        frequencyUnitCombo->setFixedHeight(30);
        frequencyUnitCombo->setStyleSheet(
            "QComboBox {"
            "  background-color: #001822;"
            "  color: #00ccff;"
            "  border: 1px solid #00a0cc;"
            "  border-radius: 3px;"
            "  padding: 5px;"
            "  font-family: Consolas;"
            "}"
            "QComboBox::drop-down {"
            "  subcontrol-origin: padding;"
            "  subcontrol-position: top right;"
            "  width: 20px;"
            "  border-left: 1px solid #00a0cc;"
            "}"
        );
        freqLayout->addWidget(frequencyUnitCombo, 1);

        formLayout->addRow(frequencyTitleLabel, freqContainer);

        // Etykieta informacji o separatorze dziesiętnym
        QLabel *decimalHintLabel = new QLabel("Use dot (.) as decimal separator (e.g. 98.7)", this);
        decimalHintLabel->setStyleSheet("color: #008888; background-color: transparent; font-family: Consolas; font-size: 8pt;");
        formLayout->addRow("", decimalHintLabel);

        // Etykieta hasła
        QLabel* passwordLabel = new QLabel("PASSWORD:", this);
        passwordLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

        // Pole hasła
        passwordEdit = new CyberLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setPlaceholderText("WAVELENGTH PASSWORD (IF REQUIRED)");
        formLayout->addRow(passwordLabel, passwordEdit);

        mainLayout->addLayout(formLayout);

        // Etykieta statusu
        statusLabel = new QLabel(this);
        statusLabel->setStyleSheet("color: #ff3355; background-color: transparent; font-family: Consolas; font-size: 9pt;");
        statusLabel->hide();
        mainLayout->addWidget(statusLabel);

        // Panel przycisków
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(15);

        joinButton = new CyberButton("JOIN WAVELENGTH", this, true);
        joinButton->setFixedHeight(35);

        cancelButton = new CyberButton("CANCEL", this, false);
        cancelButton->setFixedHeight(35);

        buttonLayout->addWidget(joinButton, 2);
        buttonLayout->addWidget(cancelButton, 1);
        mainLayout->addLayout(buttonLayout);

        // Połączenia sygnałów i slotów
        connect(frequencyEdit, &QLineEdit::textChanged, this, &JoinWavelengthDialog::validateInput);
        connect(passwordEdit, &QLineEdit::textChanged, this, &JoinWavelengthDialog::validateInput);
        connect(joinButton, &QPushButton::clicked, this, &JoinWavelengthDialog::tryJoin);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        WavelengthJoiner* joiner = WavelengthJoiner::getInstance();
        connect(joiner, &WavelengthJoiner::wavelengthJoined, this, &JoinWavelengthDialog::accept);
        connect(joiner, &WavelengthJoiner::authenticationFailed, this, &JoinWavelengthDialog::onAuthFailed);
        connect(joiner, &WavelengthJoiner::connectionError, this, &JoinWavelengthDialog::onConnectionError);

        validateInput();

        // Inicjuj timer odświeżania
        m_refreshTimer = new QTimer(this);
        m_refreshTimer->setInterval(16);
        connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
    if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0) {
        int scanLineY = height() * m_digitalizationProgress;
        int clipSize = 20;

        if (scanLineY != m_lastScanlineY) {
            // Odśwież tylko obszar wokół aktualnej i poprzedniej pozycji linii skanującej
            int minY = qMin(scanLineY, m_lastScanlineY) - 15;
            int maxY = qMax(scanLineY, m_lastScanlineY) + 15;

            // Ograniczenie zakresu do widocznego obszaru
            minY = qMax(0, minY);
            maxY = qMin(height(), maxY);

            update(0, minY, width(), maxY - minY);
        }
    }
    else if (m_glitchIntensity > 0.01) {
        for (int i = 0; i < m_glitchLines.size(); i++) {
            int y = m_glitchLines[i];
            update(0, y-5, width(), 10);
        }
    }
    else {
        m_refreshTimer->setInterval(33); // Mniej intensywne odświeżanie gdy nie ma animacji
    }
});
    }

    ~JoinWavelengthDialog() {
        if (m_refreshTimer) {
            m_refreshTimer->stop();
            delete m_refreshTimer;
            m_refreshTimer = nullptr;
        }
    }

    // Akcesory do animacji
    double digitalizationProgress() const { return m_digitalizationProgress; }
    void setDigitalizationProgress(double progress) {
        if (!m_animationStarted && progress > 0.01)
            m_animationStarted = true;
        m_digitalizationProgress = progress;
        update();
    }

    double cornerGlowProgress() const { return m_cornerGlowProgress; }
    void setCornerGlowProgress(double progress) {
        m_cornerGlowProgress = progress;
        update();
    }

    double glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(double intensity) {
        m_glitchIntensity = intensity;
        update();
        if (intensity > 0.4) regenerateGlitchLines();
    }

    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    void regenerateGlitchLines() {
        static QElapsedTimer lastGlitchUpdate;
        if (!lastGlitchUpdate.isValid() || lastGlitchUpdate.elapsed() > 100) {
            m_glitchLines.clear();
            int glitchCount = 3 + static_cast<int>(glitchIntensity() * 7);
            for (int i = 0; i < glitchCount; i++) {
                m_glitchLines.append(QRandomGenerator::global()->bounded(height()));
            }
            lastGlitchUpdate.restart();
        }
    }

    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Główne tło dialogu z gradientem
        QLinearGradient bgGradient(0, 0, 0, height());
        bgGradient.setColorAt(0, QColor(10, 21, 32));
        bgGradient.setColorAt(1, QColor(7, 18, 24));

        // Ścieżka dialogu ze ściętymi rogami
        QPainterPath dialogPath;
        int clipSize = 20; // rozmiar ścięcia

        // Tworzenie ścieżki dialogu
        dialogPath.moveTo(clipSize, 0);
        dialogPath.lineTo(width() - clipSize, 0);
        dialogPath.lineTo(width(), clipSize);
        dialogPath.lineTo(width(), height() - clipSize);
        dialogPath.lineTo(width() - clipSize, height());
        dialogPath.lineTo(clipSize, height());
        dialogPath.lineTo(0, height() - clipSize);
        dialogPath.lineTo(0, clipSize);
        dialogPath.closeSubpath();

        painter.setPen(Qt::NoPen);
        painter.setBrush(bgGradient);
        painter.drawPath(dialogPath);

        // Obramowanie dialogu
        QColor borderColor(0, 195, 255, 150);
        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(dialogPath);

        // Efekt digitalizacji
        if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0 && m_animationStarted) {
            // Inicjalizacja buforów
            initRenderBuffers();

            int scanLineY = static_cast<int>(height() * m_digitalizationProgress);
            int clipSize = 20;

            if (scanLineY != m_lastScanlineY) {
                painter.setClipping(false);
                QPainter::CompositionMode previousMode = painter.compositionMode();
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

                // Rysuj wszystkie linie poziome od góry do aktualnej pozycji
                painter.drawPixmap(0, 0, m_scanLinesBuffer, 0, 0, width(), scanLineY);

                // Oblicz szerokość linii skanującej na danej wysokości
                int startX = 0;
                int endX = width();

                // Górna część dialogu
                if (scanLineY < clipSize) {
                    float ratio = (float)scanLineY / clipSize;
                    startX = clipSize * (1.0f - ratio);
                    endX = width() - clipSize * (1.0f - ratio);
                }
                // Dolna część dialogu
                else if (scanLineY > height() - clipSize) {
                    float ratio = (float)(height() - scanLineY) / clipSize;
                    startX = clipSize * (1.0f - ratio);
                    endX = width() - clipSize * (1.0f - ratio);
                }

                // Rysuj linię skanującą z uwzględnieniem szerokości w danym punkcie Y
                int scanWidth = endX - startX;
                if (scanWidth > 0) {
                    painter.drawPixmap(startX, scanLineY - 10, scanWidth, 20,
                                      m_scanlineBuffer,
                                      (width() - scanWidth) / 2, 0, scanWidth, 20);
                }

                painter.setCompositionMode(previousMode);
                m_lastScanlineY = scanLineY;
            }
        }

        // Efekt glitch
        if (m_glitchIntensity > 0.01) {
            painter.setPen(QPen(QColor(255, 50, 120, 150 * m_glitchIntensity), 1));

            for (int i = 0; i < m_glitchLines.size(); i++) {
                int y = m_glitchLines[i];
                int offset = QRandomGenerator::global()->bounded(5, 15) * m_glitchIntensity;
                int width = QRandomGenerator::global()->bounded(30, 80) * m_glitchIntensity;

                if (QRandomGenerator::global()->bounded(2) == 0) {
                    painter.drawLine(0, y, width, y + offset);
                } else {
                    painter.drawLine(this->width() - width, y, this->width(), y + offset);
                }
            }
        }

        // Podświetlanie rogów
        if (m_cornerGlowProgress > 0.0) {
            QColor cornerColor(0, 220, 255, 150);
            painter.setPen(QPen(cornerColor, 2));

            double step = 1.0 / 4;

            if (m_cornerGlowProgress >= step * 0) {
                painter.drawLine(0, clipSize * 1.5, 0, clipSize);
                painter.drawLine(0, clipSize, clipSize, 0);
                painter.drawLine(clipSize, 0, clipSize * 1.5, 0);
            }

            if (m_cornerGlowProgress >= step * 1) {
                painter.drawLine(width() - clipSize * 1.5, 0, width() - clipSize, 0);
                painter.drawLine(width() - clipSize, 0, width(), clipSize);
                painter.drawLine(width(), clipSize, width(), clipSize * 1.5);
            }

            if (m_cornerGlowProgress >= step * 2) {
                painter.drawLine(width(), height() - clipSize * 1.5, width(), height() - clipSize);
                painter.drawLine(width(), height() - clipSize, width() - clipSize, height());
                painter.drawLine(width() - clipSize, height(), width() - clipSize * 1.5, height());
            }

            if (m_cornerGlowProgress >= step * 3) {
                painter.drawLine(clipSize * 1.5, height(), clipSize, height());
                painter.drawLine(clipSize, height(), 0, height() - clipSize);
                painter.drawLine(0, height() - clipSize, 0, height() - clipSize * 1.5);
            }
        }

        // Efekt scanline
        if (m_scanlineOpacity > 0.01) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 60 * m_scanlineOpacity));

            for (int y = 0; y < height(); y += 3) {
                painter.drawRect(0, y, width(), 1);
            }
        }
    }

    double getFrequency() const {
        double value = frequencyEdit->text().toDouble();

        // Przelicz wartość na Hz w zależności od wybranej jednostki
        if (frequencyUnitCombo->currentText() == "kHz") {
            value *= 1000.0;
        } else if (frequencyUnitCombo->currentText() == "MHz") {
            value *= 1000000.0;
        }

        return value;
    }

    QString getPassword() const {
        return passwordEdit->text();
    }

    QTimer* getRefreshTimer() const { return m_refreshTimer; }
    void startRefreshTimer() {
        if (m_refreshTimer) {
            m_refreshTimer->start();
        }
    }
    void stopRefreshTimer() {
        if (m_refreshTimer) {
            m_refreshTimer->stop();
        }
    }
    void setRefreshTimerInterval(int interval) {
        if (m_refreshTimer) {
            m_refreshTimer->setInterval(interval);
        }
    }

private slots:
    void validateInput() {
        statusLabel->hide();
        joinButton->setEnabled(!frequencyEdit->text().isEmpty());
    }

    void tryJoin() {
        static bool isJoining = false;

        if (isJoining) {
            return;
        }

        isJoining = true;
        statusLabel->hide();

        double frequency = getFrequency();
        QString password = passwordEdit->text();

        if (frequency < 130 || frequency > 180000000.0) {
            statusLabel->setText("FREQUENCY MUST BE BETWEEN 130Hz AND 180MHz");
            statusLabel->show();
            isJoining = false;
            return;
        }

        // Animacja scanline podczas łączenia
        QPropertyAnimation* scanAnim = new QPropertyAnimation(this, "scanlineOpacity");
        scanAnim->setDuration(800);
        scanAnim->setStartValue(0.1);
        scanAnim->setEndValue(0.4);
        scanAnim->setKeyValueAt(0.5, 0.6);
        scanAnim->start(QAbstractAnimation::DeleteWhenStopped);

        // Animacja glitch podczas łączenia
        QPropertyAnimation* glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        glitchAnim->setDuration(800);
        glitchAnim->setStartValue(0.0);
        glitchAnim->setEndValue(0.2);
        glitchAnim->setKeyValueAt(0.3, 0.5);
        glitchAnim->start(QAbstractAnimation::DeleteWhenStopped);

        WavelengthJoiner* joiner = WavelengthJoiner::getInstance();

        QApplication::setOverrideCursor(Qt::WaitCursor);
        bool success = joiner->joinWavelength(frequency, password);
        QApplication::restoreOverrideCursor();

        if (!success) {
            statusLabel->setText("CONNECTION FAILED: WAVELENGTH UNAVAILABLE");
            statusLabel->show();

            // Animacja błędu (więcej glitchy)
            QPropertyAnimation* errorGlitchAnim = new QPropertyAnimation(this, "glitchIntensity");
            errorGlitchAnim->setDuration(1000);
            errorGlitchAnim->setStartValue(0.5);
            errorGlitchAnim->setEndValue(0.0);
            errorGlitchAnim->setKeyValueAt(0.2, 0.8);
            errorGlitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }

        isJoining = false;
    }

    void onAuthFailed(int frequency) {
        statusLabel->setText("AUTHENTICATION FAILED: INCORRECT PASSWORD");
        statusLabel->show();

        // Animacja błędu autentykacji
        QPropertyAnimation* errorGlitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        errorGlitchAnim->setDuration(1000);
        errorGlitchAnim->setStartValue(0.5);
        errorGlitchAnim->setEndValue(0.0);
        errorGlitchAnim->setKeyValueAt(0.3, 0.7);
        errorGlitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void onConnectionError(const QString& errorMessage) {
        statusLabel->setText("CONNECTION ERROR: " + errorMessage.toUpper());
        statusLabel->show();

        // Animacja błędu połączenia
        QPropertyAnimation* errorGlitchAnim = new QPropertyAnimation(this, "glitchIntensity");
        errorGlitchAnim->setDuration(1000);
        errorGlitchAnim->setStartValue(0.5);
        errorGlitchAnim->setEndValue(0.0);
        errorGlitchAnim->setKeyValueAt(0.2, 0.9);
        errorGlitchAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    void initRenderBuffers() {
    if (!m_buffersInitialized || height() != m_previousHeight) {
        int clipSize = 20; // rozmiar wycięcia rogów dialogu

        // Bufor głównej linii skanującej - nadal na pełnej szerokości
        m_scanlineBuffer = QPixmap(width(), 20);
        m_scanlineBuffer.fill(Qt::transparent);

        QPainter scanPainter(&m_scanlineBuffer);
        scanPainter.setRenderHint(QPainter::Antialiasing, false);

        // Główna linia skanująca jako gradient
        QLinearGradient scanGradient(0, 0, 0, 20);
        scanGradient.setColorAt(0, QColor(0, 200, 255, 0));
        scanGradient.setColorAt(0.5, QColor(0, 220, 255, 180));
        scanGradient.setColorAt(1, QColor(0, 200, 255, 0));

        // Rysujemy na pełnej szerokości - ale uwzględniając kształt dialogu
        scanPainter.setPen(Qt::NoPen);
        scanPainter.setBrush(scanGradient);

        // Ścieżka dialogu ze ściętymi rogami dla linii skanującej (mniejszej wysokości)
        QPainterPath scanlinePath;
        scanlinePath.moveTo(clipSize, 0);
        scanlinePath.lineTo(width() - clipSize, 0);
        scanlinePath.lineTo(width(), 0);
        scanlinePath.lineTo(width(), 20);
        scanlinePath.lineTo(0, 20);
        scanlinePath.lineTo(0, 0);
        scanlinePath.closeSubpath();

        scanPainter.drawPath(scanlinePath);

        // Bufor dla linii poziomych - tworzymy nowy dla każdej wysokości
        m_scanLinesBuffer = QPixmap(width(), height());
        m_scanLinesBuffer.fill(Qt::transparent);

        QPainter linesPainter(&m_scanLinesBuffer);
        linesPainter.setRenderHint(QPainter::Antialiasing, true);
        linesPainter.setPen(QPen(QColor(0, 180, 255, 40)));

        // Rysujemy linie poziome z uwzględnieniem kształtu dialogu
        for (int y = 0; y < height(); y += 6) {
            // Obliczamy szerokość w danym punkcie Y
            int startX = 0;
            int endX = width();

            // Górny obszar ścięć
            if (y < clipSize) {
                // Liniowa zmiana szerokości przy górnych ścięciach
                float ratio = (float)y / clipSize;
                startX = clipSize * (1.0f - ratio);
                endX = width() - clipSize * (1.0f - ratio);
            }
            // Dolny obszar ścięć
            else if (y > height() - clipSize) {
                // Liniowa zmiana szerokości przy dolnych ścięciach
                float ratio = (float)(height() - y) / clipSize;
                startX = clipSize * (1.0f - ratio);
                endX = width() - clipSize * (1.0f - ratio);
            }

            // Rysuj linię tylko jeśli jest widoczna
            if (startX < endX) {
                linesPainter.drawLine(startX, y, endX, y);
            }
        }

        m_buffersInitialized = true;
        m_previousHeight = height();
    }
}

    bool m_animationStarted = false;
    CyberLineEdit *frequencyEdit;
    QComboBox *frequencyUnitCombo;
    CyberLineEdit *passwordEdit;
    QLabel *statusLabel;
    CyberButton *joinButton;
    CyberButton *cancelButton;
    QTimer *m_refreshTimer;

    const int m_shadowSize;
    double m_scanlineOpacity;
    double m_digitalizationProgress = 0.0;
    double m_cornerGlowProgress = 0.0;
    double m_glitchIntensity = 0.0;
    QList<int> m_glitchLines;
    QPixmap m_scanlineBuffer;
    QPixmap m_scanLinesBuffer;
    bool m_buffersInitialized = false;
    int m_lastScanlineY = -1;
    int m_previousHeight = 0;
};

#endif // JOIN_WAVELENGTH_DIALOG_H