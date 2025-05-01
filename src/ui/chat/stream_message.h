#ifndef STREAM_MESSAGE_H
#define STREAM_MESSAGE_H

#include <QWidget>
#include <QSvgRenderer>
#include <QBitmap>
#include <qtimeline.h>
#include <utility>

#include "../chat/effects/cyber_long_text_display.h"
#include "../chat/effects/cyber_text_display.h"
#include "../../chat/files/attachments/attachment_placeholder.h"
#include "effects/disintegration_effect.h"
#include "effects/electronic_shutdown_effect.h"



class StreamMessage final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)
    Q_PROPERTY(qreal disintegrationProgress READ disintegrationProgress WRITE setDisintegrationProgress)
    Q_PROPERTY(qreal shutdownProgress READ shutdownProgress WRITE setShutdownProgress)

public:
    enum MessageType {
        kReceived,
        kTransmitted,
        kSystem
    };
    QPushButton* mark_read_button;


    static QIcon CreateColoredSvgIcon(const QString& svg_path, const QColor& color, const QSize& size) {
        QSvgRenderer renderer;
        if (!renderer.load(svg_path)) {
            qWarning() << "Nie można załadować SVG:" << svg_path;
            return QIcon(); // Zwróć pustą ikonę w razie błędu
        }

        // Utwórz pixmapę o żądanym rozmiarze z przezroczystym tłem
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        // Narysuj SVG na pixmapie
        QPainter painter(&pixmap);
        renderer.render(&painter);
        painter.end(); // Zakończ malowanie na oryginalnej pixmapie

        // Utwórz maskę z kanału alfa oryginalnej pixmapy
        const QBitmap mask = pixmap.createMaskFromColor(Qt::transparent);

        // Utwórz nową pixmapę wypełnioną docelowym kolorem
        QPixmap colored_pixmap(size);
        colored_pixmap.fill(color);

        // Zastosuj maskę do kolorowej pixmapy
        colored_pixmap.setMask(mask);

        return QIcon(colored_pixmap);
    }

    StreamMessage(QString  content, QString  sender, MessageType type, QString message_id = QString(), QWidget* parent = nullptr);

    void UpdateContent(const QString& new_content);

    // Właściwości do animacji
    qreal GetOpacity() const { return opacity_; }
    void SetOpacity(qreal opacity);

    qreal GetGlowIntensity() const { return glow_intensity_; }
    void SetGlowIntensity(qreal intensity);

    qreal GetDisintegrationProgress() const { return disintegration_progress_; }
    void SetDisintegrationProgress(qreal progress);

    qreal GetShutdownProgress() const { return shutdown_progress_; }
    void SetShutdownProgress(qreal progress);

    bool IsRead() const { return is_read_; }
    QString GetMessageSender() const { return sender_; }
    QString GetContent() const { return content_; }
    MessageType GetType() const { return type_; }
    QString GetMessageId() const {return message_id_; }

void AddAttachment(const QString& html);

    // Nowa metoda do dostosowywania rozmiaru wiadomości
void AdjustSizeToContent();

    QSize sizeHint() const override;

    // Zoptymalizowane metody fadeIn i fadeOut

void FadeIn();

    void FadeOut();

    void UpdateScrollAreaMaxHeight() const;

    void StartDisintegrationAnimation();

    void ShowNavigationButtons(bool has_previous, bool has_next) const;

    QPushButton* GetNextButton() const { return next_button_; }
    QPushButton* GetPrevButton() const { return prev_button_; }


signals:
    void messageRead();
    void hidden();

public slots:
    void MarkAsRead();

protected:

    void paintEvent(QPaintEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void UpdateAnimation();

    void AdjustScrollAreaStyle() const;

private:
    // Wyciąga atrybut z HTML
    static QString ExtractAttribute(const QString& html, const QString& attribute);

    void StartLongMessageClosingAnimation();

    void ProcessImageAttachment(const QString& html);

    void ProcessGifAttachment(const QString& html);

    void ProcessAudioAttachment(const QString& html);

    void ProcessVideoAttachment(const QString& html);

    // Funkcja do czyszczenia zawartości wiadomości, usuwania HTML i placeholderów
    void CleanupContent();

    void UpdateLayout() const;

    QString message_id_;
    QString content_;
    QString clean_content_;
    QString sender_;
    MessageType type_;
    qreal opacity_;
    qreal glow_intensity_;
    qreal disintegration_progress_;
    bool is_read_;

    QVBoxLayout* main_layout_;
    QPushButton* next_button_;
    QPushButton* prev_button_;
    QTimer* animation_timer_;
    DisintegrationEffect* disintegration_effect_ = nullptr;
    QWidget* attachment_widget_ = nullptr;
    QLabel* content_label_ = nullptr;
    CyberTextDisplay* text_display_ = nullptr;
    ElectronicShutdownEffect* shutdown_effect_ = nullptr;
    qreal shutdown_progress_ = 0.0;
    QScrollArea* scroll_area_ = nullptr;
    CyberLongTextDisplay* long_text_display_ = nullptr;
};

#endif // STREAM_MESSAGE_H