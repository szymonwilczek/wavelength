#ifndef CYBER_ATTACHMENT_VIEWER_H
#define CYBER_ATTACHMENT_VIEWER_H

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QPainter>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QDebug>
#include <QEvent>

#include "chat/effects/mask_overlay.h"

class CyberAttachmentViewer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int decryptionCounter READ decryptionCounter WRITE setDecryptionCounter)

public:
    CyberAttachmentViewer(QWidget* parent = nullptr)
    : QWidget(parent), m_decryptionCounter(0),
      m_isScanning(false), m_isDecrypted(false)
{
    // Główny układ
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(10);

    // Label statusu
    m_statusLabel = new QLabel("INICJALIZACJA SEKWENCJI DESZYFRUJĄCEJ", this);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  color: #00ffff;"
        "  background-color: #001822;"
        "  border: 1px solid #00aaff;"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 9pt;"
        "  padding: 4px;"
        "  border-radius: 2px;"
        "  font-weight: bold;"
        "}"
    );
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);

    // Kontener na zawartość
    m_contentContainer = new QWidget(this);
    m_contentLayout = new QVBoxLayout(m_contentContainer);
        m_contentLayout->setContentsMargins(0, 0, 0, 0); // Zmieniono marginesy na 0
        // m_contentLayout->setAlignment(Qt::AlignCenter); // Usunięto alignment, pozwalamy zawartości wypełnić
        m_layout->addWidget(m_contentContainer, 1);

        m_maskOverlay = new MaskOverlay(m_contentContainer);
        m_maskOverlay->setVisible(false);


    // Timer dla animacji
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &CyberAttachmentViewer::updateAnimation);

    // Ustaw styl tła
    setStyleSheet(
        "CyberAttachmentViewer {"
        "  background-color: rgba(10, 20, 30, 200);"
        "  border: 1px solid #00aaff;"
        "}"
    );

    // Usuwamy ograniczenia minimum, aby mógł się dostosować do zawartości
    setMinimumSize(0, 0);

    // Ustawiamy dobrą politykę rozmiaru
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // Zmieniono na Preferred
        m_contentContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);


        connect(this, &CyberAttachmentViewer::decryptionCounterChanged, m_maskOverlay, &MaskOverlay::setRevealProgress);

    qDebug() << "CyberAttachmentViewer: utworzono z rozmiarem początkowym" << size();

    QTimer::singleShot(500, this, &CyberAttachmentViewer::onActionButtonClicked);
}

    ~CyberAttachmentViewer() {
        if (m_contentWidget) {
            m_contentWidget->setGraphicsEffect(nullptr);
        }
    }

    int decryptionCounter() const { return m_decryptionCounter; }
    void setDecryptionCounter(int counter) {
        if (m_decryptionCounter != counter) {
            m_decryptionCounter = counter;
            updateDecryptionStatus();
            emit decryptionCounterChanged(m_decryptionCounter); // Emituj sygnał
        }
    }

    void updateContentLayout() {
        if (m_contentWidget) {
            // Wymuszamy aktualizację layoutu
            m_contentLayout->invalidate();
            m_contentLayout->activate();

            // Aktualizujemy geometrię
            m_contentWidget->updateGeometry();
            updateGeometry();

            // Aktualizujemy rozmiar i propagujemy zmianę w górę hierarchii
            QTimer::singleShot(50, this, [this]() {
                // Powiadom rodziców o zmianie rozmiaru
                QEvent event(QEvent::LayoutRequest);
                QApplication::sendEvent(this, &event);

                if (parentWidget()) {
                    parentWidget()->updateGeometry();
                    QApplication::sendEvent(parentWidget(), &event);
                }
            });
        }
    }


    void setContent(QWidget* content) {
        qDebug() << "CyberAttachmentViewer::setContent - ustawianie zawartości";

        if (m_contentWidget) {
            m_contentLayout->removeWidget(m_contentWidget);
            m_contentWidget->deleteLater();
            m_contentWidget = nullptr;
        }

        m_contentWidget = content;
        m_contentLayout->addWidget(m_contentWidget);

        content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        content->setMinimumSize(0, 0);
        content->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

        // *** ZMIANA: Zawartość jest początkowo UKRYTA ***
        content->setVisible(false);
        // *** ZMIANA: Maska jest pokazywana zamiast zawartości ***
        m_maskOverlay->setVisible(true);
        m_maskOverlay->raise(); // Upewnij się, że maska jest na wierzchu
        m_maskOverlay->startScanning(); // Rozpocznij animację skanowania na masce

        m_statusLabel->setText("WYKRYTO ZASZYFROWANE DANE");
        m_isDecrypted = false;
        m_isScanning = false; // Resetuj stan skanowania
        setDecryptionCounter(0); // Resetuj licznik

        m_contentLayout->activate();
        updateGeometry(); // Ważne, aby dostosować rozmiar kontenera

        // *** ZMIANA: Wymuś aktualizację rozmiaru maski po krótkim opóźnieniu ***
        QTimer::singleShot(10, this, [this]() {
            if (m_contentWidget && m_maskOverlay) {
                 // Ustaw rozmiar maski na rozmiar kontenera zawartości
                 m_maskOverlay->setGeometry(m_contentContainer->rect());
                 m_maskOverlay->raise(); // Ponownie na wszelki wypadek
            }
            // Poinformuj rodzica o potencjalnej zmianie rozmiaru
            if (parentWidget()) {
                parentWidget()->updateGeometry();
                if (parentWidget()->layout()) parentWidget()->layout()->activate();
                QEvent event(QEvent::LayoutRequest);
                QApplication::sendEvent(parentWidget(), &event);
            }
        });
    }

    QSize sizeHint() const override {
        QSize hint;
        int extraHeight = m_statusLabel->sizeHint().height() + m_layout->spacing();
        int extraWidth = m_layout->contentsMargins().left() + m_layout->contentsMargins().right();
        extraHeight += m_layout->contentsMargins().top() + m_layout->contentsMargins().bottom();

        if (m_contentWidget) {
            // Pobieramy sizeHint zawartości (czyli AutoScalingAttachment)
            QSize contentHint = m_contentWidget->sizeHint();
            if (contentHint.isValid()) {
                hint.setWidth(contentHint.width() + extraWidth);
                hint.setHeight(contentHint.height() + extraHeight);
                return hint;
            } else {
                // Jeśli contentHint jest nieprawidłowy, spróbujmy rozmiar widgetu
                QSize contentSize = m_contentWidget->size();
                if (contentSize.isValid() && contentSize.width() > 0) {
                    hint.setWidth(contentSize.width() + extraWidth);
                    hint.setHeight(contentSize.height() + extraHeight);
                    return hint;
                }
            }
        }

        // Podstawowy rozmiar, jeśli nie mamy zawartości lub jej rozmiar jest nieznany
        QSize defaultSize(200, 100); // Zmniejszony domyślny rozmiar
        hint.setWidth(defaultSize.width() + extraWidth);
        hint.setHeight(defaultSize.height() + extraHeight);
        return hint;
    }

protected:

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        // Dopasuj rozmiar i pozycję maski do kontenera zawartości
        if (m_contentContainer && m_maskOverlay) {
            m_maskOverlay->setGeometry(m_contentContainer->rect());
            m_maskOverlay->raise(); // Upewnij się, że jest na wierzchu
        }
    }

    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Ramki w stylu AR
        QColor borderColor(0, 200, 255);
        painter.setPen(QPen(borderColor, 1));

        // Technologiczna ramka
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

        // Dodatkowe linie technologiczne
        painter.setPen(QPen(borderColor, 1, Qt::DotLine));
        painter.drawLine(2, 30, width() - 2, 30);

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

        // Techniczne dane w rogach
        painter.setPen(borderColor);
        painter.setFont(QFont("Consolas", 7));

        // Lewy dolny - poziom zabezpieczeń
        int secLevel = m_isDecrypted ? 0 : QRandomGenerator::global()->bounded(1, 6);
        painter.drawText(10, height() - 10, QString("SEC:LVL%1").arg(secLevel));

        // Prawy dolny - status
        QString status = m_isDecrypted ? "UNLOCKED" : "LOCKED";
        painter.drawText(width() - 85, height() - 10, status);

        // // W trybie skanowania dodajemy więcej linii skanujących
        // if (m_isScanning && !m_isDecrypted) {
        //     painter.setPen(QPen(QColor(0, 255, 255, 80), 1, Qt::SolidLine));
        //
        //     int scanLines = 8;
        //     for (int i = 0; i < scanLines; i++) {
        //         qreal phase = static_cast<qreal>(QDateTime::currentMSecsSinceEpoch() % 2000) / 2000.0;
        //         qreal pos = (phase + static_cast<qreal>(i) / scanLines) - floor(phase + static_cast<qreal>(i) / scanLines);
        //         int y = static_cast<int>(pos * height());
        //
        //         painter.drawLine(0, y, width(), y);
        //     }
        // }
    }

private slots:
    void onActionButtonClicked() {
        // Logika pozostaje podobna, ale operuje na stanie i masce
        if (!m_isDecrypted) {
            if (!m_isScanning) {
                startScanningAnimation(); // Rozpoczyna tylko wizualne skanowanie na masce
            }
            // Nie ma już potrzeby ręcznego klikania, aby rozpocząć deszyfrację,
            // startDecryptionAnimation jest wywoływane automatycznie po skanowaniu.
        } else {
            closeViewer();
        }
    }

    void startScanningAnimation() {
        if (!m_contentWidget) return; // Nie rób nic, jeśli nie ma zawartości

        m_isScanning = true;
        m_isDecrypted = false;
        setDecryptionCounter(0); // Resetuj postęp
        m_statusLabel->setText("SKANOWANIE ZABEZPIECZEŃ...");

        // *** ZMIANA: Nie pokazujemy m_contentWidget ***
        // m_contentWidget->setVisible(true); // USUNIĘTE

        // *** ZMIANA: Upewnij się, że maska jest widoczna i animowana ***
        m_maskOverlay->setGeometry(m_contentContainer->rect()); // Upewnij się co do rozmiaru
        m_maskOverlay->raise();
        m_maskOverlay->startScanning(); // Rozpocznij/kontynuuj animację skanowania

        // Używamy timera do symulacji czasu skanowania przed deszyfracją
        QTimer::singleShot(2000, this, [this]() { // Czas trwania "skanowania"
            if (m_isScanning) { // Sprawdź, czy nadal jesteśmy w trybie skanowania
                 m_statusLabel->setText("SKANOWANIE ZAKOŃCZONE. PRZYGOTOWANIE DESZYFRACJI...");
                 // Automatycznie rozpocznij dekodowanie po krótkim opóźnieniu
                 QTimer::singleShot(800, this, &CyberAttachmentViewer::startDecryptionAnimation);
            }
        });

        update(); // Przerenderuj ramki itp.
    }

    void startDecryptionAnimation() {
        if (!m_contentWidget) return; // Nie rób nic, jeśli nie ma zawartości
        if (m_isDecrypted) return;   // Nie zaczynaj, jeśli już zakończono

        m_isScanning = false; // Skanowanie zakończone, zaczyna się deszyfracja
        m_statusLabel->setText("ROZPOCZĘTO DEKODOWANIE... 0%");

        // *** ZMIANA: Maska nadal jest widoczna, ale zacznie się odsłaniać ***
        m_maskOverlay->setVisible(true);
        m_maskOverlay->raise();
        // Animacja licznika (która przez sygnał/slot aktualizuje maskę)
        QPropertyAnimation* decryptAnim = new QPropertyAnimation(this, "decryptionCounter");
        decryptAnim->setDuration(6000);
        decryptAnim->setStartValue(0);
        decryptAnim->setEndValue(100);
        decryptAnim->setEasingCurve(QEasingCurve::OutQuad);

        connect(decryptAnim, &QPropertyAnimation::finished, this, &CyberAttachmentViewer::finishDecryption);

        decryptAnim->start(QPropertyAnimation::DeleteWhenStopped);

        // Timer animacji (np. dla glitchy w tekście) nie jest już potrzebny do maski
        // m_animTimer->start(50); // Można zostawić dla innych efektów jeśli są
    }

    void updateAnimation() {
        // Losowe znaki hackowania w etykiecie statusu
        if (!m_isDecrypted) {
            // Aktualizujemy tekst czasami z losowymi zakłóceniami
            if (QRandomGenerator::global()->bounded(100) < 30) {
                QString baseText = m_statusLabel->text();
                if (baseText.contains("%")) {
                    baseText = QString("DEKODOWANIE... %1%").arg(m_decryptionCounter);
                }

                // Dodaj losowe znaki
                int charToGlitch = QRandomGenerator::global()->bounded(baseText.length());
                if (charToGlitch < baseText.length()) {
                    QChar glitchChar = QChar(QRandomGenerator::global()->bounded(33, 126));
                    baseText[charToGlitch] = glitchChar;
                }

                m_statusLabel->setText(baseText);
            }
        }
    }

    void updateDecryptionStatus() {
        // Aktualizuje tylko etykietę tekstową
        m_statusLabel->setText(QString("DEKODOWANIE... %1%").arg(m_decryptionCounter));
        // Nie trzeba już ręcznie aktualizować maski, robi to sygnał decryptionCounterChanged
    }

    void finishDecryption() {
        // m_animTimer->stop(); // Zatrzymaj, jeśli był używany
        m_isDecrypted = true;
        m_isScanning = false;

        m_statusLabel->setText("DESZYFRACJA ZAKOŃCZONA - DOSTĘP PRZYZNANY");

        // *** ZMIANA: Ukryj maskę i pokaż zawartość ***
        m_maskOverlay->stopScanning(); // Zatrzymuje timer i ukrywa maskę
        if (m_contentWidget) {
            m_contentWidget->setVisible(true); // Pokaż finalną zawartość
        }

        update(); // Przerenderuj stan końcowy (np. status UNLOCKED)
    }

    void closeViewer() {
        // Emitujemy sygnał zakończenia
        emit viewingFinished();
    }

signals:
    void decryptionCounterChanged(int value);
    void viewingFinished();

private:
    int m_decryptionCounter;
    bool m_isScanning;
    bool m_isDecrypted;

    QVBoxLayout* m_layout;
    QWidget* m_contentContainer;
    QVBoxLayout* m_contentLayout;
    QWidget* m_contentWidget = nullptr;
    QLabel* m_statusLabel;
    QTimer* m_animTimer;
    QTimer* m_blinkTimer = nullptr;
    int m_scanProgress = 0;
    MaskOverlay* m_maskOverlay;
};

#endif // CYBER_ATTACHMENT_VIEWER_H