#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QApplication>
#include <QtConcurrent>
#include <QStyleOptionButton>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QSurfaceFormat>

#include "../session/wavelength_session_coordinator.h"
#include "../ui/dialogs/animated_dialog.h"
#include "../../../font_manager.h"

// Cyberpunkowy checkbox
class CyberCheckBox : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberCheckBox(const QString& text, QWidget* parent = nullptr)
        : QCheckBox(text, parent), m_glowIntensity(0.5) {
        setStyleSheet("QCheckBox { spacing: 8px; background-color: transparent; color: #00ccff; font-family: Consolas; font-size: 9pt; }");
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Nie rysujemy standardowego wyglądu
        QStyleOptionButton opt;
        opt.initFrom(this);

        // Kolory
        QColor bgColor(0, 30, 40);
        QColor borderColor(0, 200, 255);
        QColor checkColor(0, 220, 255);
        QColor textColor(0, 200, 255);

        // Rysowanie pola wyboru (kwadrat ze ściętymi rogami)
        const int checkboxSize = 16;
        const int x = 0;
        const int y = (height() - checkboxSize) / 2;

        // Ścieżka dla pola wyboru ze ściętymi rogami
        QPainterPath path;
        int clipSize = 3; // rozmiar ścięcia

        path.moveTo(x + clipSize, y);
        path.lineTo(x + checkboxSize - clipSize, y);
        path.lineTo(x + checkboxSize, y + clipSize);
        path.lineTo(x + checkboxSize, y + checkboxSize - clipSize);
        path.lineTo(x + checkboxSize - clipSize, y + checkboxSize);
        path.lineTo(x + clipSize, y + checkboxSize);
        path.lineTo(x, y + checkboxSize - clipSize);
        path.lineTo(x, y + clipSize);
        path.closeSubpath();

        // Tło pola
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        // Obramowanie
        painter.setPen(QPen(borderColor, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Efekt poświaty
        if (m_glowIntensity > 0.1) {
            QColor glowColor = borderColor;
            glowColor.setAlpha(80 * m_glowIntensity);

            painter.setPen(QPen(glowColor, 2.0));
            painter.drawPath(path);
        }

        // Rysowanie znacznika (jeśli zaznaczony)
        if (isChecked()) {
            // Znacznik w postaci "X" w stylu technologicznym
            painter.setPen(QPen(checkColor, 2.0));
            int margin = 3;
            painter.drawLine(x + margin, y + margin, x + checkboxSize - margin, y + checkboxSize - margin);
            painter.drawLine(x + checkboxSize - margin, y + margin, x + margin, y + checkboxSize - margin);
        }

        // Skalowanie tekstu
        QFont font = painter.font();
        font.setFamily("Blender Pro Book");
        font.setPointSize(9);
        painter.setFont(font);

        // Rysowanie tekstu
        QRect textRect(x + checkboxSize + 5, 0, width() - checkboxSize - 5, height());
        painter.setPen(textColor);
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
    }

    void enterEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.9);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QCheckBox::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QCheckBox::leaveEvent(event);
    }

private:
    double m_glowIntensity;
};

// Cyberpunkowe pole tekstowe
class CyberLineEdit : public QLineEdit {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberLineEdit(QWidget* parent = nullptr)
        : QLineEdit(parent), m_glowIntensity(0.0) {
        setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt;");
        setCursor(Qt::IBeamCursor);

        // Kolor tekstu
        QPalette pal = palette();
        pal.setColor(QPalette::Text, QColor(0, 220, 255));
        setPalette(pal);
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Kolory
        QColor bgColor(0, 24, 34);
        QColor borderColor(0, 180, 255);

        // Tworzymy ścieżkę ze ściętymi rogami
        QPainterPath path;
        int clipSize = 6; // rozmiar ścięcia
        int w = width();
        int h = height();

        path.moveTo(clipSize, 0);
        path.lineTo(w - clipSize, 0);
        path.lineTo(w, clipSize);
        path.lineTo(w, h - clipSize);
        path.lineTo(w - clipSize, h);
        path.lineTo(clipSize, h);
        path.lineTo(0, h - clipSize);
        path.lineTo(0, clipSize);
        path.closeSubpath();

        // Tło
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        // Obramowanie
        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Efekt świecenia przy fokusie
        if (hasFocus() || m_glowIntensity > 0.1) {
            double intensity = hasFocus() ? 1.0 : m_glowIntensity;

            QColor glowColor = borderColor;
            glowColor.setAlpha(80 * intensity);

            painter.setPen(QPen(glowColor, 2.0));
            painter.drawPath(path);
        }

        // Efekt technologicznych linii
        painter.setPen(QPen(QColor(0, 100, 150), 1, Qt::DotLine));
        painter.drawLine(clipSize * 2, h - 2, w - clipSize * 2, h - 2);

        // Dodatkowe oznaczenia techniczne
        if (hasFocus()) {
            int markSize = 3;
            QColor markColor(0, 200, 255);
            painter.setPen(markColor);

            // Lewy dolny
            painter.drawLine(2, h - markSize - 2, 2 + markSize, h - markSize - 2);
            painter.drawLine(2, h - markSize - 2, 2, h - 2);

            // Prawy dolny
            painter.drawLine(w - 2 - markSize, h - markSize - 2, w - 2, h - markSize - 2);
            painter.drawLine(w - 2, h - markSize - 2, w - 2, h - 2);
        }

        // Rysowanie tekstu
        QRect textRect = rect().adjusted(10, 0, -10, 0);

        // Parametry tekstu
        QString content = text();
        QString placeholder = placeholderText();

        if (content.isEmpty() && !hasFocus() && !placeholder.isEmpty()) {
            painter.setPen(QPen(QColor(0, 140, 180)));
            painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, placeholder);
        } else {
            painter.setPen(QPen(QColor(0, 220, 255)));

            if (echoMode() == QLineEdit::Password) {
                content = QString(content.length(), '•');
            }

            painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, content);
        }

        // Rysowanie kursora jeśli ma focus i jest widoczny
        if (hasFocus() && cursorPosition() >= 0) {
            int cursorX = 10; // Domyślna pozycja X

            // Oblicz szerokość tekstu przed kursorem
            if (!content.isEmpty()) {
                QString textBeforeCursor = content.left(cursorPosition());
                QFontMetrics fm(font());
                cursorX += fm.horizontalAdvance(echoMode() == QLineEdit::Password ?
                                             QString(textBeforeCursor.length(), '•') :
                                             textBeforeCursor);
            }

            // Mrugający kursor (zależnie od stanu parzystości sekund)
            if (QDateTime::currentMSecsSinceEpoch() % 1000 < 500) {
                painter.setPen(QPen(QColor(0, 220, 255), 1));
                painter.drawLine(QPoint(cursorX, 5), QPoint(cursorX, h - 5));
            }
        }
    }

    void focusInEvent(QFocusEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QLineEdit::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(300);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QLineEdit::focusOutEvent(event);
    }

    void enterEvent(QEvent* event) override {
        if (!hasFocus()) {
            QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
            anim->setDuration(200);
            anim->setStartValue(m_glowIntensity);
            anim->setEndValue(0.5);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
        QLineEdit::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (!hasFocus()) {
            QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
            anim->setDuration(200);
            anim->setStartValue(m_glowIntensity);
            anim->setEndValue(0.0);
            anim->start(QPropertyAnimation::DeleteWhenStopped);
        }
        QLineEdit::leaveEvent(event);
    }

private:
    double m_glowIntensity;
};

// Cyberpunkowy przycisk - alternatywna implementacja dla tej aplikacji
class CyberButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberButton(const QString& text, QWidget* parent = nullptr, bool isPrimary = true)
        : QPushButton(text, parent), m_glowIntensity(0.5), m_isPrimary(isPrimary) {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet("background-color: transparent; border: none; font-family: Consolas; font-size: 9pt; font-weight: bold;");
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Paleta kolorów zależna od typu przycisku
        QColor bgColor, borderColor, textColor, glowColor;

        if (m_isPrimary) {
            bgColor = QColor(0, 40, 60);
            borderColor = QColor(0, 200, 255);
            textColor = QColor(0, 220, 255);
            glowColor = QColor(0, 150, 255, 50);
        } else {
            bgColor = QColor(40, 23, 41);
            borderColor = QColor(207, 56, 110);
            textColor = QColor(230, 70, 120);
            glowColor = QColor(200, 50, 100, 50);
        }

        // Ścieżka przycisku ze ściętymi rogami
        QPainterPath path;
        int clipSize = 5; // rozmiar ścięcia

        path.moveTo(clipSize, 0);
        path.lineTo(width() - clipSize, 0);
        path.lineTo(width(), clipSize);
        path.lineTo(width(), height() - clipSize);
        path.lineTo(width() - clipSize, height());
        path.lineTo(clipSize, height());
        path.lineTo(0, height() - clipSize);
        path.lineTo(0, clipSize);
        path.closeSubpath();

        // Efekt poświaty
        if (m_glowIntensity > 0.2) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(glowColor);

            for (int i = 3; i > 0; i--) {
                double glowSize = i * 2.0 * m_glowIntensity;
                QPainterPath glowPath;
                glowPath.addRoundedRect(rect().adjusted(-glowSize, -glowSize, glowSize, glowSize), 4, 4);
                painter.setOpacity(0.15 * m_glowIntensity);
                painter.drawPath(glowPath);
            }
            painter.setOpacity(1.0);
        }

        // Tło przycisku
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        // Obramowanie
        painter.setPen(QPen(borderColor, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Ozdobne linie wewnętrzne
        painter.setPen(QPen(borderColor.darker(150), 1, Qt::DotLine));
        painter.drawLine(5, 5, width() - 5, 5);
        painter.drawLine(5, height() - 5, width() - 5, height() - 5);

        // Znaczniki w rogach
        int markerSize = 3;
        painter.setPen(QPen(borderColor, 1, Qt::SolidLine));

        // Lewy górny
        painter.drawLine(clipSize + 2, 3, clipSize + 2 + markerSize, 3);
        painter.drawLine(clipSize + 2, 3, clipSize + 2, 3 + markerSize);

        // Prawy górny
        painter.drawLine(width() - clipSize - 2 - markerSize, 3, width() - clipSize - 2, 3);
        painter.drawLine(width() - clipSize - 2, 3, width() - clipSize - 2, 3 + markerSize);

        // Prawy dolny
        painter.drawLine(width() - clipSize - 2 - markerSize, height() - 3, width() - clipSize - 2, height() - 3);
        painter.drawLine(width() - clipSize - 2, height() - 3, width() - clipSize - 2, height() - 3 - markerSize);

        // Lewy dolny
        painter.drawLine(clipSize + 2, height() - 3, clipSize + 2 + markerSize, height() - 3);
        painter.drawLine(clipSize + 2, height() - 3, clipSize + 2, height() - 3 - markerSize);

        // Tekst przycisku
        painter.setPen(QPen(textColor, 1));
        painter.setFont(font());

        // Efekt przesunięcia dla stanu wciśniętego
        if (isDown()) {
            painter.drawText(rect().adjusted(1, 1, 1, 1), Qt::AlignCenter, text());
        } else {
            painter.drawText(rect(), Qt::AlignCenter, text());
        }

        // Jeśli nieaktywny, dodajemy efekt przyciemnienia
        if (!isEnabled()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 120));
            painter.drawPath(path);

            // Linie "zakłóceń" dla efektu nieaktywności
            painter.setPen(QPen(QColor(50, 50, 50, 80), 1, Qt::DotLine));
            for (int y = 0; y < height(); y += 3) {
                painter.drawLine(0, y, width(), y);
            }
        }
    }

    void enterEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.9);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QPushButton::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        m_glowIntensity = 1.0;
        update();
        QPushButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        m_glowIntensity = 0.9;
        update();
        QPushButton::mouseReleaseEvent(event);
    }

private:
    double m_glowIntensity;
    bool m_isPrimary;
};

// Główna klasa dialogu Wavelength
class WavelengthDialog : public AnimatedDialog {
    Q_OBJECT
     Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
     Q_PROPERTY(double digitalizationProgress READ digitalizationProgress WRITE setDigitalizationProgress)
     Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)
     Q_PROPERTY(double glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)

public:
    explicit WavelengthDialog(QWidget *parent = nullptr)
    : AnimatedDialog(parent, AnimatedDialog::DigitalMaterialization),
      m_shadowSize(10), m_scanlineOpacity(0.08)
{

        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_NoSystemBackground, true);

        // Preferuj animacje przez GPU
        setAttribute(Qt::WA_TranslucentBackground);

        setAttribute(Qt::WA_StaticContents, true);  // Optymalizuje częściowe aktualizacje
        setAttribute(Qt::WA_ContentsPropagated, false);  // Zapobiega propagacji zawartości

        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QSurfaceFormat::setDefaultFormat(format);

        m_glitchLines = QList<int>();
    setWindowTitle("CREATE_WAVELENGTH::NEW_INSTANCE");
    setModal(true);
    setFixedSize(450, 315);

    setAnimationDuration(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    // Nagłówek z tytułem
    QLabel *titleLabel = new QLabel("GENERATE WAVELENGTH", this);
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

    // Panel instrukcji - poprawka dla responsywności
    QLabel *infoLabel = new QLabel("System automatically assigns the lowest available frequency", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    infoLabel->setAlignment(Qt::AlignLeft);
    infoLabel->setWordWrap(true); // Włącz zawijanie tekstu
    mainLayout->addWidget(infoLabel);

    // Uprościliśmy panel formularza - bez dodatkowego kontenera
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setContentsMargins(0, 10, 0, 10);

    // Etykiety w formularzu
    QLabel* frequencyTitleLabel = new QLabel("ASSIGNED FREQUENCY:", this);
    frequencyTitleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    // Pole wyświetlające częstotliwość
    frequencyLabel = new QLabel(this);
    frequencyLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 16pt;");
    formLayout->addRow(frequencyTitleLabel, frequencyLabel);

    // Wskaźnik ładowania przy wyszukiwaniu częstotliwości
    loadingIndicator = new QLabel("SEARCHING FOR AVAILABLE FREQUENCY...", this);
    loadingIndicator->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    formLayout->addRow("", loadingIndicator);

    // Checkbox zabezpieczenia hasłem
    passwordProtectedCheckbox = new CyberCheckBox("PASSWORD PROTECTED", this);
    // Aktualizacja stylów dla checkboxa
    formLayout->addRow("", passwordProtectedCheckbox);

    // Etykieta i pole hasła
    QLabel* passwordLabel = new QLabel("PASSWORD:", this);
    passwordLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    passwordEdit = new CyberLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setEnabled(false);
    passwordEdit->setPlaceholderText("ENTER WAVELENGTH PASSWORD");
    passwordEdit->setStyleSheet("font-family: Consolas; font-size: 9pt;");
    formLayout->addRow(passwordLabel, passwordEdit);

    mainLayout->addLayout(formLayout);

    // Etykieta statusu
    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet("color: #ff3355; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    statusLabel->hide(); // Ukryj na początku
    mainLayout->addWidget(statusLabel);

    // Panel przycisków
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);

    generateButton = new CyberButton("CREATE WAVELENGTH", this, true);
    generateButton->setFixedHeight(35);
    generateButton->setEnabled(false); // Disable until frequency is found

    cancelButton = new CyberButton("CANCEL", this, false);
    cancelButton->setFixedHeight(35);

    buttonLayout->addWidget(generateButton, 2);
    buttonLayout->addWidget(cancelButton, 1);
    mainLayout->addLayout(buttonLayout);

    // Połączenia sygnałów i slotów
        connect(passwordProtectedCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        passwordEdit->setEnabled(checked);

        // Zmiana wyglądu pola w zależności od stanu
        if (checked) {
            passwordEdit->setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt; color: #00ccff;");
        } else {
            passwordEdit->setStyleSheet("border: none; background-color: transparent; padding: 5px; font-family: Consolas; font-size: 9pt; color: #005577;");
        }

        // Ukrywamy etykietę statusu przy zmianie stanu checkboxa
        statusLabel->hide();
        validateInputs();
    });
    connect(passwordEdit, &QLineEdit::textChanged, this, &WavelengthDialog::validateInputs);
    connect(generateButton, &QPushButton::clicked, this, &WavelengthDialog::tryGenerate);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        frequencyWatcher = new QFutureWatcher<double>(this);
        connect(frequencyWatcher, &QFutureWatcher<double>::finished, this, &WavelengthDialog::onFrequencyFound);

        // Ustawiamy text w etykiecie częstotliwości na domyślną wartość
        frequencyLabel->setText("...");

        // Podłączamy sygnał zakończenia animacji do rozpoczęcia wyszukiwania częstotliwości
        connect(this, &AnimatedDialog::showAnimationFinished,
                this, &WavelengthDialog::startFrequencySearch);

        // Inicjuj timer odświeżania
        m_refreshTimer = new QTimer(this);
        m_refreshTimer->setInterval(16); // ~60fps dla płynności animacji
        connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
            // Odświeżaj tylko jeśli są aktywne animacje
            if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0) {
                int scanLineY = height() * m_digitalizationProgress;

                // Używaj m_lastScanlineY do określenia czy pozycja się zmieniła
                if (scanLineY != m_lastScanlineY) {
                    // Odświeżaj tylko obszar wokół poprzedniej i obecnej pozycji linii skanującej
                    int minY = qMin(scanLineY, m_lastScanlineY) - 15;
                    int maxY = qMax(scanLineY, m_lastScanlineY) + 15;
                    update(0, qMax(0, minY), width(), qMin(height(), maxY - minY + 30));
                }
            }
            else if (m_glitchIntensity > 0.01) {
                // Odświeżaj tylko obszary z glitchami
                for (int i = 0; i < m_glitchLines.size(); i++) {
                    int y = m_glitchLines[i];
                    update(0, y - 5, width(), 10);
                }
            }
            else {
                // Jeśli nie ma co animować, zmniejsz częstotliwość odświeżania
                m_refreshTimer->setInterval(33); // ~30fps gdy nie trzeba tak często odświeżać
            }
        });
}

    ~WavelengthDialog()  override {
        if (m_refreshTimer) {
            m_refreshTimer->stop();
            delete m_refreshTimer;
            m_refreshTimer = nullptr;
        }
    }

    double digitalizationProgress() const { return m_digitalizationProgress; }
    void setDigitalizationProgress(double progress) {
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

    void regenerateGlitchLines() {
        // Nie generuj linii glitch zbyt często - najwyżej co 100ms
        static QElapsedTimer lastGlitchUpdate;
        if (!lastGlitchUpdate.isValid() || lastGlitchUpdate.elapsed() > 100) {
            m_glitchLines.clear();
            int glitchCount = 3 + static_cast<int>(glitchIntensity() * 7); // Mniej linii dla wydajności
            for (int i = 0; i < glitchCount; i++) {
                m_glitchLines.append(QRandomGenerator::global()->bounded(height()));
            }
            lastGlitchUpdate.restart();
        }
    }

    // Akcesory dla animacji scanlines
    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    void WavelengthDialog::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Główne tło dialogu z gradientem
    QLinearGradient bgGradient(0, 0, 0, height());
    bgGradient.setColorAt(0, QColor(10, 21, 32));
    bgGradient.setColorAt(1, QColor(7, 18, 24));

    // Ścieżka dialogu ze ściętymi rogami
    QPainterPath dialogPath;
    int clipSize = 20; // rozmiar ścięcia

    // Górna krawędź
    dialogPath.moveTo(clipSize, 0);
    dialogPath.lineTo(width() - clipSize, 0);

    // Prawy górny róg
    dialogPath.lineTo(width(), clipSize);

    // Prawa krawędź
    dialogPath.lineTo(width(), height() - clipSize);

    // Prawy dolny róg
    dialogPath.lineTo(width() - clipSize, height());

    // Dolna krawędź
    dialogPath.lineTo(clipSize, height());

    // Lewy dolny róg
    dialogPath.lineTo(0, height() - clipSize);

    // Lewa krawędź
    dialogPath.lineTo(0, clipSize);

    // Lewy górny róg
    dialogPath.lineTo(clipSize, 0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgGradient);
    painter.drawPath(dialogPath);

    // Obramowanie dialogu
    QColor borderColor(0, 195, 255, 150);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(dialogPath);

        if (m_digitalizationProgress > 0.0 && m_digitalizationProgress < 1.0) {
            // Inicjalizujemy bufory, jeśli to potrzebne
            initRenderBuffers();

            // Obliczamy pozycję linii skanującej
            int scanLineY = static_cast<int>(height() * m_digitalizationProgress);

            // Rysujemy tylko gdy pozycja się zmieni - to duża optymalizacja!
            if (scanLineY != m_lastScanlineY) {
                painter.setClipping(false); // Wyłącz przycinanie dla pełnego bufora

                // 1. Najpierw rysujemy linie poziome, ale tylko poniżej linii skanującej
                QPainter::CompositionMode previousMode = painter.compositionMode();
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

                // Używamy prostego źródłowego prostokąta z bufora
                QRect sourceRect(0, 0, width(), scanLineY);
                painter.drawPixmap(0, 0, m_scanLinesBuffer, 0, 0, width(), scanLineY);

                // 2. Teraz rysujemy główną linię skanującą z bufora
                painter.drawPixmap(0, scanLineY - 10, m_scanlineBuffer);

                painter.setCompositionMode(previousMode);

                // Zapamiętujemy ostatnią pozycję
                m_lastScanlineY = scanLineY;
            }
        }

    // NOWY KOD - efekt glitch
    if (m_glitchIntensity > 0.01) {
        // Glitch na brzegach
        painter.setPen(QPen(QColor(255, 50, 120, 150 * m_glitchIntensity), 1));

        for (int i = 0; i < m_glitchLines.size(); i++) {
            int y = m_glitchLines[i];
            int offset = QRandomGenerator::global()->bounded(5, 15) * m_glitchIntensity;
            int width = QRandomGenerator::global()->bounded(30, 80) * m_glitchIntensity;

            // Losowe położenie glitchy
            if (QRandomGenerator::global()->bounded(2) == 0) {
                // Lewa strona
                painter.drawLine(0, y, width, y + offset);
            } else {
                // Prawa strona
                painter.drawLine(this->width() - width, y, this->width(), y + offset);
            }
        }
    }

    // NOWY KOD - sekwencyjne podświetlanie rogów
    if (m_cornerGlowProgress > 0.0) {
        QColor cornerColor(0, 220, 255, 150);
        painter.setPen(QPen(cornerColor, 2));

        // Obliczamy które rogi powinny być podświetlone
        double step = 1.0 / 4; // 4 rogi

        if (m_cornerGlowProgress >= step * 0) { // Lewy górny
            painter.drawLine(0, clipSize * 1.5, 0, clipSize);
            painter.drawLine(0, clipSize, clipSize, 0);
            painter.drawLine(clipSize, 0, clipSize * 1.5, 0);
        }

        if (m_cornerGlowProgress >= step * 1) { // Prawy górny
            painter.drawLine(width() - clipSize * 1.5, 0, width() - clipSize, 0);
            painter.drawLine(width() - clipSize, 0, width(), clipSize);
            painter.drawLine(width(), clipSize, width(), clipSize * 1.5);
        }

        if (m_cornerGlowProgress >= step * 2) { // Prawy dolny
            painter.drawLine(width(), height() - clipSize * 1.5, width(), height() - clipSize);
            painter.drawLine(width(), height() - clipSize, width() - clipSize, height());
            painter.drawLine(width() - clipSize, height(), width() - clipSize * 1.5, height());
        }

        if (m_cornerGlowProgress >= step * 3) { // Lewy dolny
            painter.drawLine(clipSize * 1.5, height(), clipSize, height());
            painter.drawLine(clipSize, height(), 0, height() - clipSize);
            painter.drawLine(0, height() - clipSize, 0, height() - clipSize * 1.5);
        }
    }

    // Linie skanowania (scanlines)
    if (m_scanlineOpacity > 0.01) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * m_scanlineOpacity));

        for (int y = 0; y < height(); y += 3) {
            painter.drawRect(0, y, width(), 1);
        }
    }
}

    double getFrequency() const {
        return m_frequency;
    }

    bool isPasswordProtected() const {
        return passwordProtectedCheckbox->isChecked();
    }

    QString getPassword() const {
        return passwordEdit->text();
    }

private slots:
    void validateInputs() {
        statusLabel->hide();

        bool isPasswordValid = true;
        if (passwordProtectedCheckbox->isChecked()) {
            isPasswordValid = !passwordEdit->text().isEmpty();

            // Nie pokazujemy błędu od razu po zaznaczeniu checkboxa
            // Komunikat pojawi się tylko przy próbie wygenerowania
        }

        // Przycisk jest aktywny tylko jeśli znaleziono częstotliwość i hasło jest prawidłowe (jeśli wymagane)
        generateButton->setEnabled(isPasswordValid && m_frequencyFound);
    }

    void startFrequencySearch() {
        qDebug() << "LOG: Rozpoczynam wyszukiwanie częstotliwości po zakończeniu animacji";
        loadingIndicator->setText("SEARCHING FOR AVAILABLE FREQUENCY...");

        // Timer zabezpieczający przed zbyt długim wyszukiwaniem
        QTimer* timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);
        timeoutTimer->setInterval(5000); // 5 sekund limitu na szukanie
        connect(timeoutTimer, &QTimer::timeout, [this]() {
            if (frequencyWatcher && frequencyWatcher->isRunning()) {
                loadingIndicator->setText("USING DEFAULT FREQUENCY...");
                m_frequency = 130.0;
                m_frequencyFound = true;
                onFrequencyFound();
                qDebug() << "LOG: Timeout podczas szukania częstotliwości - używam wartości domyślnej";
            }
        });
        timeoutTimer->start();

        // Uruchom asynchroniczne wyszukiwanie
        QFuture<double> future = QtConcurrent::run(&WavelengthDialog::findLowestAvailableFrequency);
        frequencyWatcher->setFuture(future);
    }

    void tryGenerate() {
        static bool isGenerating = false;

        if (isGenerating) {
            qDebug() << "LOG: Blokada wielokrotnego wywołania tryGenerate()";
            return;
        }

        isGenerating = true;
        qDebug() << "LOG: tryGenerate - start";

        bool isPasswordProtected = passwordProtectedCheckbox->isChecked();
        QString password = passwordEdit->text();

        // Sprawdź warunki bezpieczeństwa - tutaj pokazujemy błąd, gdy próbujemy wygenerować hasło
        if (isPasswordProtected && password.isEmpty()) {
            statusLabel->setText("PASSWORD REQUIRED");
            statusLabel->show();
            isGenerating = false;
            return;
        }

        qDebug() << "LOG: tryGenerate - walidacja pomyślna, akceptacja dialogu";
        QDialog::accept();

        isGenerating = false;
        qDebug() << "LOG: tryGenerate - koniec";
    }

    void onFrequencyFound() {
        // Zatrzymujemy timer zabezpieczający jeśli istnieje
        QTimer* timeoutTimer = findChild<QTimer*>();
        if (timeoutTimer && timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }

        // Pobierz wynik asynchronicznego wyszukiwania
        if (frequencyWatcher && frequencyWatcher->isFinished()) {
            m_frequency = frequencyWatcher->result();
            qDebug() << "LOG: onFrequencyFound otrzymało wartość:" << m_frequency;
        } else {
            qDebug() << "LOG: onFrequencyFound - brak wyniku, używam domyślnego 130 Hz";
            m_frequency = 130.0;
        }

        m_frequencyFound = true;

        // Przygotuj tekst częstotliwości
        QString frequencyText = formatFrequencyText(m_frequency);

        // Przygotowanie animacji dla wskaźnika ładowania (znikanie)
        QPropertyAnimation *loaderSlideAnimation = new QPropertyAnimation(loadingIndicator, "maximumHeight");
        loaderSlideAnimation->setDuration(400);
        loaderSlideAnimation->setStartValue(loadingIndicator->sizeHint().height());
        loaderSlideAnimation->setEndValue(0);
        loaderSlideAnimation->setEasingCurve(QEasingCurve::OutQuint);

        // Dodanie efektu przezroczystości dla etykiety częstotliwości
        QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(frequencyLabel);
        frequencyLabel->setGraphicsEffect(opacityEffect);
        opacityEffect->setOpacity(0.0); // Początkowo niewidoczny

        // Animacja pojawiania się etykiety częstotliwości
        QPropertyAnimation *frequencyAnimation = new QPropertyAnimation(opacityEffect, "opacity");
        frequencyAnimation->setDuration(600);
        frequencyAnimation->setStartValue(0.0);
        frequencyAnimation->setEndValue(1.0);
        frequencyAnimation->setEasingCurve(QEasingCurve::InQuad);

        // Utwórz grupę animacji, która zostanie uruchomiona sekwencyjnie
        QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
        animGroup->addAnimation(loaderSlideAnimation);

        // Sygnalizacja kiedy pierwsza animacja się kończy
        connect(loaderSlideAnimation, &QPropertyAnimation::finished, [this, frequencyText]() {
            // Ukryj loadingIndicator
            loadingIndicator->hide();

            // Ustaw tekst częstotliwości
            frequencyLabel->setText(frequencyText);
        });

        // Połączenie sekwencyjne - druga animacja po zakończeniu pierwszej
        connect(loaderSlideAnimation, &QPropertyAnimation::finished, [this, frequencyAnimation]() {
            frequencyAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        });

        // Dla większej płynności animacji
        loadingIndicator->setAttribute(Qt::WA_OpaquePaintEvent, false);
        frequencyLabel->setAttribute(Qt::WA_OpaquePaintEvent, false);

        frequencyAnimation->setKeyValueAt(0.2, 0.3);
        frequencyAnimation->setKeyValueAt(0.4, 0.6);
        frequencyAnimation->setKeyValueAt(0.6, 0.8);
        frequencyAnimation->setKeyValueAt(0.8, 0.95);

        // Rozpocznij pierwszą animację
        animGroup->start(QAbstractAnimation::DeleteWhenStopped);

        // Sprawdź czy przycisk powinien być aktywowany
        validateInputs();

        // Dodaj efekt scanline przy znalezieniu częstotliwości
        QPropertyAnimation* scanAnim = new QPropertyAnimation(this, "scanlineOpacity");
        scanAnim->setDuration(1500);
        scanAnim->setStartValue(0.3);
        scanAnim->setEndValue(0.08);
        scanAnim->setKeyValueAt(0.3, 0.4);
        scanAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // Nowa pomocnicza metoda do formatowania tekstu częstotliwości
    QString formatFrequencyText(double frequency) {
        QString unitText;
        double displayValue;

        if (frequency >= 1000000) {
            displayValue = frequency / 1000000.0;
            unitText = " MHz";
        } else if (frequency >= 1000) {
            displayValue = frequency / 1000.0;
            unitText = " kHz";
        } else {
            displayValue = frequency;
            unitText = " Hz";
        }

        // Formatuj z jednym miejscem po przecinku
        return QString::number(displayValue, 'f', 1) + unitText;
    }

private:
    static double findLowestAvailableFrequency() {
        qDebug() << "LOG: Rozpoczęto szukanie dostępnej częstotliwości";

        // Zapytaj serwer o następną dostępną częstotliwość
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;

        // Utwórz żądanie GET do serwera
        QUrl url("http://localhost:3000/api/next-available-frequency");
        QNetworkRequest request(url);

        // Wykonaj żądanie synchronicznie (w kontekście asynchronicznego QtConcurrent::run)
        reply = manager.get(request);

        // Połącz sygnał zakończenia z pętlą zdarzeń
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        // Uruchom pętlę zdarzeń aż do zakończenia żądania
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            // Parsuj odpowiedź jako JSON
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();

            if (obj.contains("frequency")) {
                double frequency = obj["frequency"].toDouble();
                qDebug() << "LOG: Serwer zwrócił dostępną częstotliwość:" << frequency << "Hz";
                reply->deleteLater();
                return frequency;
            }
        } else {
            qDebug() << "LOG: Błąd podczas komunikacji z serwerem:" << reply->errorString();
        }

        reply->deleteLater();

        // W przypadku błędu zwróć domyślną wartość
        qDebug() << "LOG: Używam domyślnej częstotliwości 130 Hz";
        return 130.0;
    }

    void initRenderBuffers() {
        if (!m_buffersInitialized || height() != m_previousHeight) {
            // Bufor głównej linii skanującej
            m_scanlineBuffer = QPixmap(width(), 20);
            m_scanlineBuffer.fill(Qt::transparent);

            QPainter scanPainter(&m_scanlineBuffer);
            scanPainter.setRenderHint(QPainter::Antialiasing, false); // Wyłączenie antialiasingu dla szybkości

            // Główna linia skanująca jako gradient
            QLinearGradient scanGradient(0, 0, 0, 20);
            scanGradient.setColorAt(0, QColor(0, 200, 255, 0));
            scanGradient.setColorAt(0.5, QColor(0, 220, 255, 180));
            scanGradient.setColorAt(1, QColor(0, 200, 255, 0));

            scanPainter.setPen(Qt::NoPen);
            scanPainter.setBrush(scanGradient);
            scanPainter.drawRect(0, 0, width(), 20);

            // Bufor dla linii poziomych (cały ekran)
            m_scanLinesBuffer = QPixmap(width(), height());
            m_scanLinesBuffer.fill(Qt::transparent);

            QPainter linesPainter(&m_scanLinesBuffer);
            linesPainter.setPen(QPen(QColor(0, 180, 255, 40)));

            // Rysujemy linie co 6 pikseli - co drugie linia dla oszczędności
            for (int y = 0; y < height(); y += 6) {
                linesPainter.drawLine(0, y, width(), y);
            }

            m_buffersInitialized = true;
            m_previousHeight = height();
        }
    }

private:
    QLabel *frequencyLabel;
    QLabel *loadingIndicator;
    CyberCheckBox *passwordProtectedCheckbox;
    CyberLineEdit *passwordEdit;
    QLabel *statusLabel;
    CyberButton *generateButton;
    CyberButton *cancelButton;
    QFutureWatcher<double> *frequencyWatcher;
    QTimer *m_refreshTimer;
    double m_frequency = 130.0; // Domyślna wartość
    bool m_frequencyFound = false; // Flaga oznaczająca znalezienie częstotliwości
    const int m_shadowSize; // Rozmiar cienia
    double m_scanlineOpacity; // Przezroczystość linii skanowania
    QPixmap m_scanlineBuffer;
    QPixmap m_scanLinesBuffer;
    bool m_buffersInitialized = false;
    int m_lastScanlineY = -1;
    int m_previousHeight = 0;
};

#endif // WAVELENGTH_DIALOG_H