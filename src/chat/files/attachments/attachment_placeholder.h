//
// Created by szymo on 17.03.2025.
//

#ifndef ATTACHMENT_PLACEHOLDER_H
#define ATTACHMENT_PLACEHOLDER_H
#include <qfileinfo.h>
#include <qfuture.h>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <qtconcurrentrun.h>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

#include "attachment_queue_manager.h"
#include "auto_scaling_attachment.h"
#include "../../messages/formatter/message_formatter.h"
#include "../../../ui/files/cyber_attachment_viewer.h"
#include "../gif/player/inline_gif_player.h"
#include "../image/displayer/image_viewer.h"
#include "../video/player/video_player_overlay.h"

class AttachmentPlaceholder : public QWidget {
    Q_OBJECT

public:
     AttachmentPlaceholder(const QString& filename, const QString& type, QWidget* parent = nullptr, bool autoLoad = true)
    : QWidget(parent), m_filename(filename), m_isLoaded(false) {

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);

        // Ikona i typ załącznika
        QString icon;
        if (type == "image") icon = "🖼️";
        else if (type == "audio") icon = "🎵";
        else if (type == "video") icon = "📹";
        else if (type == "gif") icon = "🎞️";
        else icon = "📎";

         QFileInfo fileInfo(filename);
         QString baseName = fileInfo.baseName(); // Nazwa pliku bez ostatniego rozszerzenia
         QString suffix = fileInfo.completeSuffix(); // Całe rozszerzenie (np. tar.gz)

         const int maxBaseNameLength = 25;
         QString displayedName;

         if (baseName.length() > maxBaseNameLength) {
             displayedName = baseName.left(maxBaseNameLength) + "[...]";
         } else {
             displayedName = baseName;
         }

         // Dodaj rozszerzenie, jeśli istnieje
         if (!suffix.isEmpty()) {
             displayedName += "." + suffix;
         }

         // Użycie skróconej nazwy w etykiecie
         m_infoLabel = new QLabel(QString("<span style='color:#00ccff;'>%1</span> <span style='color:#aaaaaa; font-size:9pt;'>%2</span>").arg(icon, displayedName), this); // <<< UŻYTO displayedName
         m_infoLabel->setTextFormat(Qt::RichText);
         layout->addWidget(m_infoLabel);

        // Przycisk do ładowania załącznika - teraz ukryty domyślnie
        m_loadButton = new QPushButton("PONÓW DEKODOWANIE", this);
        m_loadButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #002b3d;"
            "  color: #00ffff;"
            "  border: 1px solid #00aaff;"
            "  border-radius: 2px;"
            "  padding: 6px;"
            "  font-family: 'Consolas', monospace;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #003e59;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #005580;"
            "}"
        );
        m_loadButton->setVisible(false); // Domyślnie ukryty
        layout->addWidget(m_loadButton);

        // Placeholder dla załącznika
        m_contentContainer = new QWidget(this);
        m_contentContainer->setVisible(false);
        m_contentLayout = new QVBoxLayout(m_contentContainer);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_contentContainer);

        // Status ładowania
        m_progressLabel = new QLabel("<span style='color:#00ccff;'>Inicjowanie sekwencji dekodowania...</span>", this);
        layout->addWidget(m_progressLabel);

        // Połączenia sygnałów
        connect(m_loadButton, &QPushButton::clicked, this, &AttachmentPlaceholder::onLoadButtonClicked);

        // Automatycznie rozpocznij ładowanie po krótkim opóźnieniu
        QTimer::singleShot(300, this, &AttachmentPlaceholder::onLoadButtonClicked);
    }

    void setContent(QWidget* content) {
         qDebug() << "AttachmentPlaceholder::setContent - ustawianie zawartości";

         QLayoutItem* item;
         while ((item = m_contentLayout->takeAt(0)) != nullptr) {
             if (QWidget* widget = item->widget()) {
                 widget->deleteLater();
             }
             delete item;
         }

         // Sprawdzamy rozmiar nowej zawartości
         if (content->sizeHint().isValid()) {
             qDebug() << "Rozmiar zawartości (sizeHint):" << content->sizeHint();
         }

         // Dodajemy zawartość do layoutu
         m_contentLayout->addWidget(content);
         m_contentContainer->setVisible(true);
         m_loadButton->setVisible(false);
         m_progressLabel->setVisible(false);
         m_isLoaded = true;

         // Usuwamy wszystkie ograniczenia rozmiaru
         content->setMinimumSize(0, 0);
         content->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
         m_contentContainer->setMinimumSize(0, 0);
         m_contentContainer->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
         setMinimumSize(0, 0);
         setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

         // Ustawiamy politykę rozmiaru - preferowana, nie ekspansywna
         content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
         m_contentContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
         // Placeholder powinien się dopasować do zawartości
         setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

         // Aktywujemy layout i aktualizujemy geometrię
         m_contentLayout->activate();
         layout()->activate(); // Aktywuj główny layout placeholdera
         updateGeometry(); // Zaktualizuj geometrię placeholdera

         // Dajemy trochę czasu na ustalenie rozmiaru i informujemy rodzica
         QTimer::singleShot(50, this, [this, content]() {
             // Wymuszamy update layoutu w CyberAttachmentViewer, jeśli to on jest zawartością
             if (CyberAttachmentViewer* viewer = qobject_cast<CyberAttachmentViewer*>(content)) {
                 viewer->updateContentLayout(); // Ta funkcja powinna zadbać o aktualizację w dół
             } else {
                 // Jeśli zawartością nie jest viewer, nadal aktualizuj geometrię
                 content->updateGeometry();
             }
             updateGeometry(); // Ponownie zaktualizuj placeholder

             // Powiadom rodzica o zmianie rozmiaru
             if (parentWidget()) {
                 parentWidget()->updateGeometry();
                 if(parentWidget()->layout()) parentWidget()->layout()->activate();
                 QEvent event(QEvent::LayoutRequest);
                 QApplication::sendEvent(parentWidget(), &event);
             }

             // Emitujemy sygnał po ustawieniu zawartości z opóźnieniem
             QTimer::singleShot(50, this, &AttachmentPlaceholder::notifyLoaded);
         });
     }

QSize sizeHint() const override {
    // Bazowy rozmiar dla widgetu bez zawartości
    QSize baseSize(400, 100);

    // Jeśli mamy zawartość, sprawdzamy jej rozmiar
    QWidget* content = nullptr;
    if (m_contentContainer && m_contentContainer->isVisible() && m_contentLayout->count() > 0) {
        content = m_contentLayout->itemAt(0)->widget();
    }

    if (content) {
        QSize contentSize = content->sizeHint();
        if (contentSize.isValid() && contentSize.width() > 0 && contentSize.height() > 0) {
            // Dodajemy przestrzeń na etykietę i przyciski
            int totalHeight = contentSize.height() +
                            m_infoLabel->sizeHint().height() +
                            (m_progressLabel->isVisible() ? m_progressLabel->sizeHint().height() : 0) +
                            (m_loadButton->isVisible() ? m_loadButton->sizeHint().height() : 0) +
                            20; // dodatkowy margines

            int totalWidth = qMax(contentSize.width(), baseSize.width());

            QSize result(totalWidth, totalHeight);
            return result;
        }
    }

    return baseSize;
}

    void setAttachmentReference(const QString& attachmentId, const QString& mimeType) {
        m_attachmentId = attachmentId;
        m_mimeType = mimeType;
        m_hasReference = true;
    }

    void setBase64Data(const QString& base64Data, const QString& mimeType) {
        m_base64Data = base64Data;
        m_mimeType = mimeType;
    }

    void setLoading(bool loading) {
        if (loading) {
            m_loadButton->setEnabled(false);
            m_loadButton->setText("DEKODOWANIE W TOKU...");
            m_progressLabel->setText("<span style='color:#00ccff;'>Pozyskiwanie zaszyfrowanych danych...</span>");
            m_progressLabel->setVisible(true);
        } else {
            m_loadButton->setEnabled(true);
            m_loadButton->setText("INICJUJ DEKODOWANIE");
            m_progressLabel->setVisible(false);
        }
    }



    signals:
    void attachmentLoaded();

private slots:
    void onLoadButtonClicked() {
        if (m_isLoaded) return;

        setLoading(true);

        // Sprawdź czy mamy referencję czy pełne dane
        if (m_hasReference) {
            // Pobieramy dane z magazynu w wątku roboczym
            AttachmentQueueManager::getInstance()->addTask([this]() {
                QString base64Data = AttachmentDataStore::getInstance()->getAttachmentData(m_attachmentId);

                if (base64Data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, "⚠️ Nie można odnaleźć danych załącznika"));
                    return;
                }

                // Dekodujemy dane i kontynuujemy jak wcześniej
                QByteArray data = QByteArray::fromBase64(base64Data.toUtf8());

                if (data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, "⚠️ Nie można zdekodować danych załącznika"));
                    return;
                }

                // Wybieramy odpowiednią metodę wyświetlania na podstawie MIME type
                if (m_mimeType.startsWith("image/")) {
                    if (m_mimeType == "image/gif") {
                        QMetaObject::invokeMethod(this, "showCyberGif",
                            Qt::QueuedConnection,
                            Q_ARG(QByteArray, data));
                    } else {
                        QMetaObject::invokeMethod(this, "showCyberImage",
                            Qt::QueuedConnection,
                            Q_ARG(QByteArray, data));
                    }
                }
                else if (m_mimeType.startsWith("audio/")) {
                    QMetaObject::invokeMethod(this, "showCyberAudio",
                        Qt::QueuedConnection,
                        Q_ARG(QByteArray, data));
                }
                else if (m_mimeType.startsWith("video/")) {
                    QMetaObject::invokeMethod(this, "showCyberVideo",
                        Qt::QueuedConnection,
                        Q_ARG(QByteArray, data));
                }
            });
        } else {
            AttachmentQueueManager::getInstance()->addTask([this]() {
                try {
                    QByteArray data = QByteArray::fromBase64(m_base64Data.toUtf8());

                    if (data.isEmpty()) {
                        QMetaObject::invokeMethod(this, "setError",
                            Qt::QueuedConnection,
                            Q_ARG(QString, "⚠️ Nie można zdekodować danych załącznika"));
                        return;
                    }

                } catch (const std::exception& e) {
                    QMetaObject::invokeMethod(this, "setError",
                        Qt::QueuedConnection,
                        Q_ARG(QString, QString("⚠️ Błąd przetwarzania: %1").arg(e.what())));
                }
            });
        }
    }


public slots:

    void setError(const QString& errorMsg) {
    m_loadButton->setEnabled(true);
    m_loadButton->setText("PONÓW DEKODOWANIE");
    m_loadButton->setVisible(true); // Pokaż przycisk przy błędzie
    m_progressLabel->setText("<span style='color:#ff3333;'>⚠️ BŁĄD: " + errorMsg + "</span>");
    m_progressLabel->setVisible(true);
    m_isLoaded = false;
}

    void showImage(const QByteArray& data) {
        InlineImageViewer* imageViewer = new InlineImageViewer(data, m_contentContainer);
        setContent(imageViewer);
        setLoading(false);
    }

    void showGif(const QByteArray& data) {
        InlineGifPlayer* gifPlayer = new InlineGifPlayer(data, m_contentContainer);
        setContent(gifPlayer);
        setLoading(false);
    }

    void showAudio(const QByteArray& data) {
        InlineAudioPlayer* audioPlayer = new InlineAudioPlayer(data, m_mimeType, m_contentContainer);
        setContent(audioPlayer);
        setLoading(false);
    }

    // Funkcja pomocnicza do tworzenia i pokazywania dialogu
    void showFullSizeDialog(const QByteArray& data, bool isGif) {
    QWidget* parentWindow = window();
    QDialog* fullSizeDialog = new QDialog(parentWindow, Qt::Window | Qt::FramelessWindowHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    fullSizeDialog->setWindowTitle(isGif ? "Podgląd animacji" : "Podgląd załącznika");
    fullSizeDialog->setModal(false);
    fullSizeDialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(fullSizeDialog);
    layout->setContentsMargins(5, 5, 5, 5);

    QScrollArea* scrollArea = new QScrollArea(fullSizeDialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet("QScrollArea { background-color: #001018; border: none; }");

    QWidget* contentWidget = nullptr;
    InlineGifPlayer* fullGif = nullptr;

    // Lambda do wywołania po uzyskaniu rozmiaru (przeniesiona logika z sizeReadyCallback)
    auto adjustAndShowWithSizeCheck = [this, fullSizeDialog, scrollArea](QWidget* contentWgt, QSize size) {
        if (size.isValid()) {
            qDebug() << "Dialog: Content size ready:" << size;
            adjustAndShowDialog(fullSizeDialog, scrollArea, contentWgt, size);
        } else {
            qDebug() << "Dialog: Content size invalid after load.";
            adjustAndShowDialog(fullSizeDialog, scrollArea, contentWgt, QSize(400, 300)); // Fallback
        }
    };

    if (isGif) {
        fullGif = new InlineGifPlayer(data, scrollArea); // Przypisz do wskaźnika
        contentWidget = fullGif;
        // Połącz gifLoaded, aby uzyskać rozmiar i pokazać dialog
        connect(fullGif, &InlineGifPlayer::gifLoaded, this, [=]() mutable { // mutable, aby móc modyfikować fullGif
            QTimer::singleShot(0, this, [=]() mutable {
                adjustAndShowWithSizeCheck(fullGif, fullGif->sizeHint());
                // Rozpocznij odtwarzanie *po* pokazaniu dialogu
                if (fullGif) { // Sprawdź czy wskaźnik jest nadal ważny
                     fullGif->startPlayback();
                }
            });
        });
        // Połącz finished dialogu, aby zatrzymać odtwarzanie
        connect(fullSizeDialog, &QDialog::finished, this, [fullGif]() mutable {
             if (fullGif) {
                 // Nie trzeba jawnie stopPlayback(), bo WA_DeleteOnClose wywoła destruktor
                 // fullGif->stopPlayback();
                 // Destruktor InlineGifPlayer wywoła releaseResources(), które zatrzyma dekoder.
                 qDebug() << "Full size GIF dialog finished.";
                 // Można by tu ustawić fullGif = nullptr, ale WA_DeleteOnClose i tak go usunie.
             }
        });
    } else {
        InlineImageViewer* fullImage = new InlineImageViewer(data, scrollArea);
        contentWidget = fullImage;
        connect(fullImage, &InlineImageViewer::imageLoaded, this, [=]() {
            QTimer::singleShot(0, this, [=]() {
                // Wywołaj lambdę pomocniczą bezpośrednio
                adjustAndShowWithSizeCheck(fullImage, fullImage->sizeHint());
            });
        });
        // Fallback z imageInfoReady
        connect(fullImage, &InlineImageViewer::imageInfoReady, this, [=](int w, int h, bool) {
            // Sprawdź, czy dialog nie został już pokazany (np. przez imageLoaded)
            // Proste sprawdzenie widoczności może wystarczyć
            if (!fullSizeDialog->isVisible()) {
                 QTimer::singleShot(0, this, [=]() {
                     // Pobierz sizeHint ponownie, na wypadek gdyby imageLoaded zadziałało w międzyczasie
                     QSize currentHint = fullImage->sizeHint();
                     if (currentHint.isValid()) {
                          adjustAndShowWithSizeCheck(fullImage, currentHint);
                     } else {
                          adjustAndShowWithSizeCheck(fullImage, QSize(w, h)); // Użyj rozmiaru z info jako fallback
                     }
                 });
            }
        });
    }

    scrollArea->setWidget(contentWidget);
    layout->addWidget(scrollArea, 1);

    QPushButton* closeButton = new QPushButton("Zamknij", fullSizeDialog);
        closeButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #002b3d;"
            "  color: #00ffff;"
            "  border: 1px solid #00aaff;"
            "  border-radius: 2px;"
            "  padding: 6px;"
            "  font-family: 'Consolas', monospace;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #003e59;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #005580;"
            "}"
        );
        layout->addWidget(closeButton, 0, Qt::AlignHCenter);
        connect(closeButton, &QPushButton::clicked, fullSizeDialog, &QDialog::accept);

        // Styl okna
        fullSizeDialog->setStyleSheet(
            "QDialog {"
            "  background-color: #001822;"
            "  border: 2px solid #00aaff;"
            "}"
        );
    }

    // Funkcja pomocnicza do dostosowania rozmiaru i pokazania dialogu
    void adjustAndShowDialog(QDialog* dialog, QScrollArea* scrollArea, QWidget* contentWidget, QSize originalContentSize) {
    if (!dialog || !contentWidget || !originalContentSize.isValid()) {
        qDebug() << "adjustAndShowDialog: Invalid arguments or size.";
        if(dialog) dialog->deleteLater();
        return;
    }

    // 1. Pobierz geometrię ekranu (bez zmian)
    QScreen* screen = nullptr;
    if (window() && window()->windowHandle()) {
        screen = window()->windowHandle()->screen();
    }
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    QRect availableGeometry = screen ? screen->availableGeometry() : QRect(0, 0, 1024, 768);
    qDebug() << "Dialog: Using Screen geometry:" << availableGeometry;

    // 2. Marginesy okna dialogowego (bez zmian)
    const int margin = 50;

    // 3. Ustaw ROZMIAR ZAWARTOŚCI na ORYGINALNY
    qDebug() << "Dialog: Setting content widget fixed size to ORIGINAL:" << originalContentSize;
    contentWidget->setFixedSize(originalContentSize); // Kluczowa zmiana!

    // 4. Oblicz preferowany rozmiar okna dialogu na podstawie ORYGINALNEGO rozmiaru zawartości
    QPushButton* closeButton = dialog->findChild<QPushButton*>();
    int buttonHeight = closeButton ? closeButton->sizeHint().height() : 30; // Użyj sizeHint przycisku
    int verticalMargins = dialog->layout()->contentsMargins().top() + dialog->layout()->contentsMargins().bottom() + dialog->layout()->spacing() + buttonHeight;
    int horizontalMargins = dialog->layout()->contentsMargins().left() + dialog->layout()->contentsMargins().right();

    QSize preferredDialogSize = originalContentSize;
    preferredDialogSize.rwidth() += horizontalMargins;
    preferredDialogSize.rheight() += verticalMargins;

    // Dodaj miejsce na paski przewijania, jeśli będą potrzebne (porównaj oryginalny rozmiar z dostępnym miejscem *wewnątrz* okna)
    QSize maxContentAreaSize(
        availableGeometry.width() - margin * 2 - horizontalMargins - scrollArea->verticalScrollBar()->sizeHint().width(),
        availableGeometry.height() - margin * 2 - verticalMargins - scrollArea->horizontalScrollBar()->sizeHint().height() // Dodano odjęcie paska poziomego
    );

    if (originalContentSize.width() > maxContentAreaSize.width()) {
        preferredDialogSize.rheight() += scrollArea->horizontalScrollBar()->sizeHint().height();
    }
    if (originalContentSize.height() > maxContentAreaSize.height()) {
        preferredDialogSize.rwidth() += scrollArea->verticalScrollBar()->sizeHint().width();
    }
     qDebug() << "Dialog: Preferred dialog size (based on original content):" << preferredDialogSize;


    // 5. OGRANICZ rozmiar okna dialogu do dostępnej geometrii ekranu (z marginesami)
    QSize finalDialogSize = preferredDialogSize;
    finalDialogSize.setWidth(qMin(finalDialogSize.width(), availableGeometry.width() - margin * 2));
    finalDialogSize.setHeight(qMin(finalDialogSize.height(), availableGeometry.height() - margin * 2));
    qDebug() << "Dialog: Final dialog size (limited to screen):" << finalDialogSize;

    dialog->resize(finalDialogSize);

    // 6. Wyśrodkuj dialog na ekranie (bez zmian)
    int x = availableGeometry.left() + (availableGeometry.width() - finalDialogSize.width()) / 2;
    int y = availableGeometry.top() + (availableGeometry.height() - finalDialogSize.height()) / 2;
    dialog->move(x, y);
    qDebug() << "Dialog: Moving to:" << QPoint(x,y);

    // 7. Pokaż dialog (bez zmian)
    dialog->show();
}

void showCyberImage(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z obrazem z zachowaniem oryginalnych wymiarów
    InlineImageViewer* imageViewer = new InlineImageViewer(data, viewer);
    imageViewer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    qDebug() << "showCyberImage - rozmiar obrazu:" << imageViewer->sizeHint();

    // Opakowujemy w AutoScalingAttachment
    AutoScalingAttachment* scalingAttachment = new AutoScalingAttachment(imageViewer, viewer);

    QSize maxSize(400, 300);

    if (parentWidget() && parentWidget()->parentWidget()) { // Zakładając hierarchię StreamMessage -> Placeholder -> Viewer -> ScalingAttachment
        QWidget* streamMessage = parentWidget()->parentWidget();
        maxSize.setWidth(qMin(maxSize.width(), streamMessage->width() - 50)); // Odejmij marginesy
        maxSize.setHeight(qMin(maxSize.height(), streamMessage->height() / 2));
    } else {
        QScreen* screen = QApplication::primaryScreen();
        if(screen) {
            maxSize.setWidth(qMin(maxSize.width(), screen->availableGeometry().width() / 3));
            maxSize.setHeight(qMin(maxSize.height(), screen->availableGeometry().height() / 3));
        }
    }
    qDebug() << "showCyberImage - Ustawiam maxSize dla miniatury:" << maxSize;
    scalingAttachment->setMaxAllowedSize(maxSize);

    // Podłączamy obsługę kliknięcia
    connect(scalingAttachment, &AutoScalingAttachment::clicked, this, [this, data]() {
            showFullSizeDialog(data, false); // false oznacza, że to nie GIF
        });

    // Ustawiamy zawartość viewera
    viewer->setContent(scalingAttachment);
    setContent(viewer);
    setLoading(false);
    // QTimer::singleShot(50, this, [this](){ updateGeometry(); });
}

void showCyberGif(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z gifem
    InlineGifPlayer* gifPlayer = new InlineGifPlayer(data, viewer);

    // Opakowujemy w AutoScalingAttachment
    AutoScalingAttachment* scalingAttachment = new AutoScalingAttachment(gifPlayer, viewer);

    QSize maxSize(400, 300);
    if (parentWidget() && parentWidget()->parentWidget()) {
        QWidget* streamMessage = parentWidget()->parentWidget();
        maxSize.setWidth(qMin(maxSize.width(), streamMessage->width() - 50));
        maxSize.setHeight(qMin(maxSize.height(), streamMessage->height() / 2));
    } else {
        QScreen* screen = QApplication::primaryScreen();
        if(screen) {
            maxSize.setWidth(qMin(maxSize.width(), screen->availableGeometry().width() / 3));
            maxSize.setHeight(qMin(maxSize.height(), screen->availableGeometry().height() / 3));
        }
    }

    scalingAttachment->setMaxAllowedSize(maxSize);

    // Podłączamy obsługę kliknięcia
    connect(scalingAttachment, &AutoScalingAttachment::clicked, this, [this, data]() {
            showFullSizeDialog(data, true); // true oznacza, że to GIF
        });

    viewer->setContent(scalingAttachment);
    setContent(viewer);
    setLoading(false);
    // QTimer::singleShot(50, this, [this](){ updateGeometry(); });
}

void showCyberAudio(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z audio
    InlineAudioPlayer* audioPlayer = new InlineAudioPlayer(data, m_mimeType, viewer);

    // Opakowujemy w AutoScalingAttachment

    viewer->setContent(audioPlayer);

    // Podłączamy sygnał zakończenia
    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        m_loadButton->setText("Załaduj ponownie");
        m_loadButton->setVisible(true);
        m_contentContainer->setVisible(false);
    });

    setContent(viewer);
    setLoading(false);
}

void showCyberVideo(const QByteArray& data) {
    CyberAttachmentViewer* viewer = new CyberAttachmentViewer(m_contentContainer);

    // Tworzymy widget z miniaturką wideo
    QWidget* videoPreview = new QWidget(viewer);
    QVBoxLayout* previewLayout = new QVBoxLayout(videoPreview);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    // Tworzymy miniaturkę
    QLabel* thumbnailLabel = new QLabel(videoPreview);
    thumbnailLabel->setFixedSize(480, 270);
    thumbnailLabel->setAlignment(Qt::AlignCenter);
    thumbnailLabel->setStyleSheet("background-color: #000000; color: white; font-size: 48px;");
    thumbnailLabel->setText("▶");
    thumbnailLabel->setCursor(Qt::PointingHandCursor);

    // Generujemy miniaturkę z pierwszej klatki
    generateThumbnail(data, thumbnailLabel);

    previewLayout->addWidget(thumbnailLabel);

    // Dodajemy przycisk odtwarzania pod miniaturką
    QPushButton* playButton = new QPushButton("ODTWÓRZ WIDEO", videoPreview);
    playButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #002b3d; "
        "  color: #00ffff; "
        "  border: 1px solid #00aaff; "
        "  padding: 6px; "
        "  border-radius: 2px; "
        "  font-family: 'Consolas', monospace; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #003e59; }"
    );
    previewLayout->addWidget(playButton);

    // Opakowujemy w AutoScalingAttachment

    viewer->setContent(videoPreview);

    // Po kliknięciu miniaturki lub przycisku, otwórz dialog z odtwarzaczem
    auto openPlayer = [this, data]() {
        m_videoData = data;
        VideoPlayerOverlay* playerOverlay = new VideoPlayerOverlay(data, m_mimeType, nullptr);

        // Dodajemy cyberpunkowy styl do overlay'a
        playerOverlay->setStyleSheet(
            "QDialog { "
            "  background-color: #001520; "
            "  border: 2px solid #00aaff; "
            "}"
        );

        playerOverlay->setAttribute(Qt::WA_DeleteOnClose);

        // Rozłącz połączenia przed zamknięciem
        QMetaObject::Connection conn = connect(playerOverlay, &QDialog::finished,
            [this, playerOverlay]() {
                disconnect(playerOverlay, nullptr, this, nullptr);
                m_videoData.clear();
            });

        playerOverlay->show();
    };

    connect(thumbnailLabel, &QLabel::linkActivated, this, openPlayer);
    connect(playButton, &QPushButton::clicked, this, openPlayer);

    // Dodatkowe połączenie dla kliknięcia w miniaturkę
    thumbnailLabel->installEventFilter(this);
    m_thumbnailLabel = thumbnailLabel;
    m_clickHandler = openPlayer;

    // Podłączamy sygnał zakończenia
    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        m_loadButton->setText("Załaduj ponownie");
        m_loadButton->setVisible(true);
        m_contentContainer->setVisible(false);
    });

    setContent(viewer);
    setLoading(false);
}

    void showVideo(const QByteArray& data) {
    // Zamiast osadzać odtwarzacz bezpośrednio, tworzymy miniaturkę z przyciskiem odtwarzania
    QWidget* videoPreview = new QWidget(m_contentContainer);
    QVBoxLayout* previewLayout = new QVBoxLayout(videoPreview);

    // Tworzymy miniaturkę (czarny prostokąt z ikoną odtwarzania)
    QLabel* thumbnailLabel = new QLabel(videoPreview);
    thumbnailLabel->setFixedSize(480, 270);
    thumbnailLabel->setAlignment(Qt::AlignCenter);
    thumbnailLabel->setStyleSheet("background-color: #000000; color: white; font-size: 48px;");
    thumbnailLabel->setText("▶");
    thumbnailLabel->setCursor(Qt::PointingHandCursor);

    // Generujemy miniaturkę z pierwszej klatki (asynchronicznie)
    generateThumbnail(data, thumbnailLabel);

    previewLayout->addWidget(thumbnailLabel);

    // Dodajemy przycisk odtwarzania pod miniaturką
    QPushButton* playButton = new QPushButton("Odtwórz wideo", videoPreview);
    playButton->setStyleSheet("QPushButton { background-color: #2c5e9e; color: white; padding: 6px; border-radius: 3px; }");
    previewLayout->addWidget(playButton);

    // Po kliknięciu na miniaturkę lub przycisk, otwórz dialog z odtwarzaczem
        auto openPlayer = [this, data]() {
            // Zachowujemy kopię danych wideo w m_videoData
            m_videoData = data;
            VideoPlayerOverlay* playerOverlay = new VideoPlayerOverlay(data, m_mimeType, nullptr);
            playerOverlay->setAttribute(Qt::WA_DeleteOnClose);
    
            // Ważne - rozłącz połączenie przed zamknięciem
            QMetaObject::Connection conn = connect(playerOverlay, &QDialog::finished,
                [this, playerOverlay]() {
                    // Rozłącz wszystkie połączenia z tym obiektem przed usunięciem
                    disconnect(playerOverlay, nullptr, this, nullptr);
                    // Zwolnij pamięć
                    m_videoData.clear();
                });

            playerOverlay->show();
        };

    connect(thumbnailLabel, &QLabel::linkActivated, this, openPlayer);
    connect(playButton, &QPushButton::clicked, this, openPlayer);

    // Dodatkowe połączenie dla kliknięcia w miniaturkę
    thumbnailLabel->installEventFilter(this);
    m_thumbnailLabel = thumbnailLabel;
    m_clickHandler = openPlayer;

    setContent(videoPreview);
    setLoading(false);
}

// Dodajemy metodę generującą miniaturkę
void generateThumbnail(const QByteArray& videoData, QLabel* thumbnailLabel) {
    // Dodajemy zadanie do menedżera kolejki
    AttachmentQueueManager::getInstance()->addTask([this, videoData, thumbnailLabel]() {
        try {
            // Tworzymy tymczasowy dekoder wideo tylko dla miniatury
            auto tempDecoder = std::make_shared<VideoDecoder>(videoData, nullptr);

            // Łączymy sygnał z dekoderem
            QObject::connect(tempDecoder.get(), &VideoDecoder::frameReady,
                thumbnailLabel, [thumbnailLabel, tempDecoder](const QImage& frame) {
                    // Skalujemy klatkę do rozmiaru miniaturki
                    QImage scaledFrame = frame.scaled(thumbnailLabel->width(), thumbnailLabel->height(),
                                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);

                    // Tworzymy obraz z ikoną odtwarzania na środku
                    QImage overlayImage(thumbnailLabel->width(), thumbnailLabel->height(), QImage::Format_RGB32);
                    overlayImage.fill(Qt::black);

                    // Rysujemy przeskalowaną klatkę na środku
                    QPainter painter(&overlayImage);
                    int x = (thumbnailLabel->width() - scaledFrame.width()) / 2;
                    int y = (thumbnailLabel->height() - scaledFrame.height()) / 2;
                    painter.drawImage(x, y, scaledFrame);

                    // Dodajemy ikonę odtwarzania
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(255, 255, 255, 180));
                    painter.drawEllipse(QRect(thumbnailLabel->width()/2 - 30,
                                             thumbnailLabel->height()/2 - 30, 60, 60));

                    painter.setBrush(QColor(0, 0, 0, 200));
                    QPolygon triangle;
                    triangle << QPoint(thumbnailLabel->width()/2 - 15, thumbnailLabel->height()/2 - 20);
                    triangle << QPoint(thumbnailLabel->width()/2 + 25, thumbnailLabel->height()/2);
                    triangle << QPoint(thumbnailLabel->width()/2 - 15, thumbnailLabel->height()/2 + 20);
                    painter.drawPolygon(triangle);

                    // Aktualizacja UI w głównym wątku
                    QMetaObject::invokeMethod(thumbnailLabel, "setPixmap",
                        Qt::QueuedConnection,
                        Q_ARG(QPixmap, QPixmap::fromImage(overlayImage)));

                    // Zatrzymujemy dekoder
                    tempDecoder->stop();
                },
                Qt::QueuedConnection);

            // Wyekstrahuj pierwszą klatkę
            tempDecoder->extractFirstFrame();

            // Dajemy czas na przetworzenie klatki
            QThread::msleep(500);

            // Zatrzymujemy dekoder
            tempDecoder->stop();
            tempDecoder->wait(300);

        } catch (...) {
            // W przypadku błędu pozostawiamy domyślną czarną miniaturkę
        }
    });
}

private:
    QString m_filename;
    QString m_base64Data;
    QString m_mimeType;
    QLabel* m_infoLabel;
    QLabel* m_progressLabel;
    QPushButton* m_loadButton;
    QWidget* m_contentContainer;
    QVBoxLayout* m_contentLayout;
    bool m_isLoaded;
    QByteArray m_videoData; // Do przechowywania danych wideo
    QLabel* m_thumbnailLabel = nullptr;
    std::function<void()> m_clickHandler;
    QString m_attachmentId;
    bool m_hasReference = false;

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_thumbnailLabel && event->type() == QEvent::MouseButtonRelease) {
            if (m_clickHandler) {
                m_clickHandler();
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void notifyLoaded() {
        qDebug() << "AttachmentPlaceholder::notifyLoaded - powiadamianie o załadowaniu załącznika";
        qDebug() << "Aktualny rozmiar:" << size();
        qDebug() << "SizeHint:" << sizeHint();

        // Informujemy o załadowaniu załącznika
        emit attachmentLoaded();
    }
};

#endif //ATTACHMENT_PLACEHOLDER_H
