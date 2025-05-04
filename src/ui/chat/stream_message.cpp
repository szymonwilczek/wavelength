#include "stream_message.h"

#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>

#include "../../chat/files/attachments/auto_scaling_attachment.h"
#include "../files/cyber_attachment_viewer.h"

StreamMessage::StreamMessage(QString content, QString sender, MessageType type, QString message_id, QWidget *parent): QWidget(parent), message_id_(std::move(message_id)), content_(std::move(content)), sender_(std::move(sender)),
                                                                                                                     type_(type), // Store the message ID
                                                                                                                     opacity_(0.0), glow_intensity_(0.8), disintegration_progress_(0.0), is_read_(false) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumWidth(400);
    setMaximumWidth(600);
    setMinimumHeight(120);

    // Podstawowy layout
    main_layout_ = new QVBoxLayout(this);
    main_layout_->setContentsMargins(45, 35, 45, 30);
    main_layout_->setSpacing(10);

    // Wyczyść treść z tagów HTML
    CleanupContent();

    // Sprawdzamy czy to wiadomość z załącznikiem
    bool has_attachment = content_.contains("placeholder");

    // Określamy, czy wiadomość jest długa
    bool is_long_message = clean_content_.length() > 500;

    if (!has_attachment) {
        if (is_long_message) {
            // Określamy kolor tekstu w zależności od typu wiadomości
            QColor text_color;
            switch (type_) {
                case kTransmitted:
                    text_color = QColor(0, 220, 255);
                    break;
                case kReceived:
                    text_color = QColor(240, 150, 255);
                    break;
                case kSystem:
                    text_color = QColor(255, 200, 0);
                    break;
            }

            // Dla długich wiadomości używamy QScrollArea z CyberLongTextDisplay
            auto scroll_area = new QScrollArea(this);
            scroll_area->setObjectName("cyberpunkScrollArea");
            scroll_area->setWidgetResizable(true);
            scroll_area->setFrameShape(QFrame::NoFrame);
            scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

            // Ustawiamy przezroczyste tło dla obszaru przewijania
            scroll_area->setStyleSheet("QScrollArea { background: transparent; border: none; }");

            // Tworzymy widget do wyświetlania tekstu
            auto long_text_display = new CyberLongTextDisplay(clean_content_, text_color);
            scroll_area->setWidget(long_text_display);

            // Dodajemy scrollArea do głównego layoutu
            main_layout_->addWidget(scroll_area);
            scroll_area_ = scroll_area;
            long_text_display_ = long_text_display;

            // Połącz sygnał przewijania ze slotami
            connect(scroll_area->verticalScrollBar(), &QScrollBar::valueChanged,
                    long_text_display, &CyberLongTextDisplay::SetScrollPosition);

            // Połącz sygnał zmiany wysokości z aktualizacją obszaru przewijania
            connect(long_text_display, &CyberLongTextDisplay::contentHeightChanged, this,
                    [scroll_area](const int height) {
                        // Aktualizuj rzeczywistą wysokość widgetu
                        scroll_area->widget()->setMinimumHeight(height);
                    });

            // Ustawiamy większą wysokość dla długich wiadomości
            setMinimumHeight(350);
        } else {
            CyberTextDisplay::TypingSoundType sound_type;
            if (sender_ == "SYSTEM") { // Sprawdzamy wartość m_sender
                sound_type = CyberTextDisplay::kSystemSound;
            } else {
                sound_type = CyberTextDisplay::kUserSound;
            }

            text_display_ = new CyberTextDisplay(clean_content_, sound_type, this);
            main_layout_->addWidget(text_display_);
        }
    } else {
        // Dla załączników - bez zmian
        content_label_ = new QLabel(this);
        content_label_->setTextFormat(Qt::RichText);
        content_label_->setWordWrap(true);
        content_label_->setStyleSheet(
            "QLabel { color: white; background-color: transparent; font-size: 10pt; }");
        content_label_->setText(clean_content_);
        main_layout_->addWidget(content_label_);
    }

    // Efekt przeźroczystości
    auto opacity = new QGraphicsOpacityEffect(this);
    opacity->setOpacity(0.0);
    setGraphicsEffect(opacity);

    // Inicjalizacja przycisków nawigacyjnych
    next_button_ = new QPushButton(">", this);
    next_button_->setFixedSize(25, 25);
    next_button_->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 200, 255, 0.3);"
        "  color: #00ffff;"
        "  border: 1px solid #00ccff;"
        "  border-radius: 3px;"   // <<< ZMIANA: Lekkie ścięcie
        "  font-weight: bold;"
        "  font-size: 12pt;"      // <<< ZMIANA: Lekko zmniejszona czcionka
        "  padding: 0px 0px 1px 0px;" // <<< ZMIANA: Minimalny padding dla lepszego wyśrodkowania '>'
        "}"
        "QPushButton:hover { background-color: rgba(0, 200, 255, 0.5); }"
        "QPushButton:pressed { background-color: rgba(0, 200, 255, 0.7); }");
    next_button_->hide();

    prev_button_ = new QPushButton("<", this);
    prev_button_->setFixedSize(25, 25);
    // Skopiuje styl z m_nextButton, w tym nowy border-radius i padding
    prev_button_->setStyleSheet(next_button_->styleSheet());
    prev_button_->hide();


    mark_read_button = new QPushButton(this); // Pusty przycisk
    mark_read_button->setFixedSize(25, 25);

    // Ustaw docelowy kolor i rozmiar ikony
    auto icon_color = QColor("#00ffcc"); // Kolor obramówki
    QSize icon_size(20, 20); // Rozmiar ikony wewnątrz przycisku

    // Utwórz pokolorowaną ikonę za pomocą funkcji pomocniczej
    QIcon check_icon = CreateColoredSvgIcon(":/resources/icons/checkmark.svg", icon_color, icon_size);

    if (!check_icon.isNull()) {
        mark_read_button->setIcon(check_icon);
        // Nie ustawiamy już setIconSize, bo ikona ma już właściwy rozmiar z funkcji pomocniczej
    } else {
        qWarning() << "Nie można utworzyć pokolorowanej ikony checkmark.svg!";
        mark_read_button->setText("✓"); // Fallback do tekstu
    }

    mark_read_button->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 255, 150, 0.3);" // Tło przycisku może pozostać
        "  border: 1px solid #00ffcc;" // Obramówka w tym samym kolorze co ikona
        "  border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: rgba(0, 255, 150, 0.5); }"
        "QPushButton:pressed { background-color: rgba(0, 255, 150, 0.7); }");
    mark_read_button->hide();

    connect(mark_read_button, &QPushButton::clicked, this, &StreamMessage::MarkAsRead);

    // Timer dla subtelnych animacji
    animation_timer_ = new QTimer(this);
    connect(animation_timer_, &QTimer::timeout, this, &StreamMessage::UpdateAnimation);
    animation_timer_->start(50);

    // Automatyczne ustawienie focusu na widgecie
    setFocusPolicy(Qt::StrongFocus);

    if (scroll_area_) {
        AdjustScrollAreaStyle();
        QTimer::singleShot(100, this, &StreamMessage::UpdateScrollAreaMaxHeight);
    }
}

void StreamMessage::UpdateContent(const QString &new_content) {
    content_ = new_content;
    CleanupContent(); // Wyczyść nowy content z HTML

    if (text_display_) {
        // Wywołaj setText w CyberTextDisplay, co zrestartuje animację
        text_display_->SetText(clean_content_);
    } else if (long_text_display_) {
        // Podobnie dla długiego tekstu (zakładając, że ma analogiczną metodę setText)
        QMetaObject::invokeMethod(long_text_display_, "setText", Q_ARG(QString, clean_content_));
    } else if (content_label_) {
        // Dla zwykłego QLabel po prostu ustaw tekst
        content_label_->setText(clean_content_);
    }
    // Nie ma potrzeby aktualizować załącznika, bo wiadomości progresu go nie mają
    AdjustSizeToContent(); // Dostosuj rozmiar, jeśli treść się zmieniła
    update(); // Wymuś odświeżenie wyglądu
}

void StreamMessage::SetOpacity(const qreal opacity) {
    opacity_ = opacity;
    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
        effect->setOpacity(opacity);
    }
}

void StreamMessage::SetGlowIntensity(const qreal intensity) {
    glow_intensity_ = intensity;
    update();
}

void StreamMessage::SetDisintegrationProgress(const qreal progress) {
    disintegration_progress_ = progress;
    if (disintegration_effect_) {
        disintegration_effect_->SetProgress(progress);
    }
}

void StreamMessage::SetShutdownProgress(const qreal progress) {
    shutdown_progress_ = progress;
    if (shutdown_effect_) {
        shutdown_effect_->SetProgress(progress);
    }
}

void StreamMessage::AddAttachment(const QString &html) {
    // Sprawdzamy, jakiego typu załącznik zawiera wiadomość
    QString type, attachment_id, mime_type, filename;

    if (html.contains("video-placeholder")) {
        type = "video";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else if (html.contains("audio-placeholder")) {
        type = "audio";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else if (html.contains("gif-placeholder")) {
        type = "gif";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else if (html.contains("image-placeholder")) {
        type = "image";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else {
        return; // Brak rozpoznanego załącznika
    }

    // Usuwamy poprzedni załącznik jeśli istnieje
    if (attachment_widget_) {
        main_layout_->removeWidget(attachment_widget_);
        delete attachment_widget_;
    }

    // KLUCZOWA ZMIANA: Całkowicie usuwamy ograniczenia rozmiaru wiadomości
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);
    setMinimumWidth(0);
    setMaximumWidth(QWIDGETSIZE_MAX);

    // Tworzymy placeholder załącznika i ustawiamy referencję
    const auto attachment_widget = new AttachmentPlaceholder(
        filename, type, this);
    attachment_widget->SetAttachmentReference(attachment_id, mime_type);

    // Ustawiamy nowy załącznik
    attachment_widget_ = attachment_widget;
    main_layout_->addWidget(attachment_widget_);

    // Ustawiamy politykę rozmiaru dla attachmentWidget (Preferred zamiast Expanding)
    attachment_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Oczyszczamy treść HTML z tagów i aktualizujemy tekst
    CleanupContent();
    if (content_label_) {
        content_label_->setText(clean_content_);
    }

    // Bezpośrednie połączenie sygnału zmiany rozmiaru załącznika
    connect(attachment_widget, &AttachmentPlaceholder::attachmentLoaded,
            this, [this]() {

                // Ustanawiamy timer, aby dać widgetowi czas na pełne renderowanie
                QTimer::singleShot(200, this, [this]() {
                    if (attachment_widget_) {
                        // Określamy rozmiar widgetu załącznika
                        const QSize attachment_size = attachment_widget_->sizeHint();

                        // Znajdujemy wszystkie widgety CyberAttachmentViewer w hierarchii
                        QList<CyberAttachmentViewer*> viewers =
                                attachment_widget_->findChildren<CyberAttachmentViewer*>();

                        if (!viewers.isEmpty()) {
                            // Wymuszamy prawidłowe obliczenie rozmiarów po całkowitym renderowaniu
                            QTimer::singleShot(100, this, [this, viewers]() {
                                // Odświeżamy rozmiar widgetu z załącznikiem
                                viewers.first()->updateGeometry();

                                // Dostosowujemy rozmiar wiadomości do załącznika
                                AdjustSizeToContent();
                            });
                        } else {
                            // Jeśli nie ma CyberAttachmentViewer, po prostu dostosowujemy rozmiar
                            AdjustSizeToContent();
                        }
                    }
                });
            });

    // Dostosuj początkowy rozmiar wiadomości
    QTimer::singleShot(100, this, [this]() {
        if (attachment_widget_) {
            const QSize initial_size = attachment_widget_->sizeHint();
            setMinimumSize(qMax(initial_size.width() + 60, 500),
                           initial_size.height() + 100);
        }
    });
}

void StreamMessage::AdjustSizeToContent() {

    // Wymuszamy recalkulację layoutu
    main_layout_->invalidate();
    main_layout_->activate();

    // Pobieramy informacje o dostępnym miejscu na ekranie
    const QScreen* screen = QApplication::primaryScreen();
    if (window()) {
        screen = window()->screen();
    }

    const QRect available_geometry = screen->availableGeometry();

    // Maksymalne dozwolone wymiary wiadomości (80% dostępnej przestrzeni)
    int max_width = available_geometry.width() * 0.8;
    int max_height = available_geometry.height() * 0.35;


    // Dajemy czas na poprawne renderowanie zawartości
    QTimer::singleShot(100, this, [this, max_width, max_height]() {
        if (attachment_widget_) {
            // Pobieramy rozmiar załącznika
            const QSize attachment_size = attachment_widget_->sizeHint();

            // Szukamy wszystkich CyberAttachmentViewer
            QList<CyberAttachmentViewer*> viewers = attachment_widget_->findChildren<CyberAttachmentViewer*>();
            if (!viewers.isEmpty()) {
                CyberAttachmentViewer* viewer = viewers.first();

                // Aktualizujemy dane widgetu
                viewer->updateGeometry();
                const QSize viewer_size = viewer->sizeHint();


                // Obliczamy rozmiar, uwzględniając marginesy i dodatkową przestrzeń
                const int new_width = qMin(
                    qMax(viewer_size.width() +
                         main_layout_->contentsMargins().left() +
                         main_layout_->contentsMargins().right() + 100, 500),
                    max_width);

                const int new_height = qMin(
                    viewer_size.height() +
                    main_layout_->contentsMargins().top() +
                    main_layout_->contentsMargins().bottom() + 80,
                    max_height);

                // Ustawiamy nowy rozmiar minimalny
                setMinimumSize(new_width, new_height);


                // Wymuszamy ponowne obliczenie rozmiaru
                updateGeometry();

                // Szukamy AutoScalingAttachment
                QList<AutoScalingAttachment*> scalers = viewer->findChildren<AutoScalingAttachment*>();
                if (!scalers.isEmpty()) {
                    // Ustawiamy maksymalny dozwolony rozmiar dla auto-skalowanej zawartości
                    const QSize content_max_size(
                        max_width - main_layout_->contentsMargins().left() - main_layout_->contentsMargins().right() + 20,
                        max_height - main_layout_->contentsMargins().top() - main_layout_->contentsMargins().bottom() - 100
                    );

                    scalers.first()->SetMaxAllowedSize(content_max_size);
                }

                // Wymuszamy aktualizację layoutu rodzica
                if (parentWidget() && parentWidget()->layout()) {
                    parentWidget()->layout()->invalidate();
                    parentWidget()->layout()->activate();
                    parentWidget()->updateGeometry();
                    parentWidget()->update();
                }
            }
        }

        // Aktualizujemy pozycję przycisków
        UpdateLayout();
        update();
    });
}

QSize StreamMessage::sizeHint() const {
    constexpr QSize base_size(550, 180);

    if (scroll_area_) {
        return QSize(550, 300); // Docelowy rozmiar dla długich wiadomości
    }

    // Jeśli mamy załącznik, dostosuj rozmiar na jego podstawie
    if (attachment_widget_) {
        const QSize attachment_size = attachment_widget_->sizeHint();

        // Szukamy CyberAttachmentViewer w hierarchii
        QList<const CyberAttachmentViewer*> viewers =
                attachment_widget_->findChildren<const CyberAttachmentViewer*>();

        if (!viewers.isEmpty()) {
            // Użyj rozmiaru CyberAttachmentViewer zamiast bezpośredniego attachmentSize
            const QSize viewer_size = viewers.first()->sizeHint();

            // Dodajemy marginesy i przestrzeń
            const int total_height = viewer_size.height() +
                              main_layout_->contentsMargins().top() +
                              main_layout_->contentsMargins().bottom() + 80;

            const int total_width = qMax(viewer_size.width() +
                                  main_layout_->contentsMargins().left() +
                                  main_layout_->contentsMargins().right() + 40, base_size.width());

            return QSize(total_width, total_height);
        }
        // Standardowe zachowanie dla innych załączników
        const int height = attachment_size.height() +
                                main_layout_->contentsMargins().top() +
                                main_layout_->contentsMargins().bottom() + 70;

        const int width = qMax(attachment_size.width() +
                                    main_layout_->contentsMargins().left() +
                                    main_layout_->contentsMargins().right() + 30, base_size.width());

        return QSize(width, height);
    }

    return base_size;
}

void StreamMessage::FadeIn() {
    // Używamy QPropertyAnimation zamiast QTimeLine dla lepszej wydajności
    const auto opacity_animation = new QPropertyAnimation(this, "opacity");
    opacity_animation->setDuration(400); // krótszy czas dla lepszej responsywności
    opacity_animation->setStartValue(0.0);
    opacity_animation->setEndValue(1.0);
    opacity_animation->setEasingCurve(QEasingCurve::OutQuad);

    // Krótkotrwały efekt poświaty
    const auto glow_animation = new QPropertyAnimation(this, "glowIntensity");
    glow_animation->setDuration(600);
    glow_animation->setStartValue(0.9);
    glow_animation->setKeyValueAt(0.4, 0.7);
    glow_animation->setEndValue(0.5);
    glow_animation->setEasingCurve(QEasingCurve::OutQuad);

    // Grupa animacji - oddziela renderowanie animacji od głównej pętli
    auto animation_group = new QParallelAnimationGroup(this);
    animation_group->addAnimation(opacity_animation);
    animation_group->addAnimation(glow_animation);

    // Podłączamy sygnał zakończenia
    connect(animation_group, &QParallelAnimationGroup::finished, this, [this, animation_group]() {
        animation_group->deleteLater();

        // Uruchamiamy animację wpisywania tekstu dopiero gdy widget jest w pełni widoczny
        if (text_display_) {
            QTimer::singleShot(50, text_display_, &CyberTextDisplay::StartReveal);
        }
    });

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);

    // Ustawiamy focus na widgecie
    QTimer::singleShot(50, this, QOverload<>::of(&StreamMessage::setFocus));
}

void StreamMessage::FadeOut() {
    // Używamy QPropertyAnimation dla lepszej wydajności renderowania
    const auto opacity_animation = new QPropertyAnimation(this, "opacity");
    opacity_animation->setDuration(300);
    opacity_animation->setStartValue(1.0);
    opacity_animation->setEndValue(0.0);
    opacity_animation->setEasingCurve(QEasingCurve::InQuad);

    // Efekt błysku przy znikaniu
    const auto glow_animation = new QPropertyAnimation(this, "glowIntensity");
    glow_animation->setDuration(300);
    glow_animation->setStartValue(0.8);
    glow_animation->setKeyValueAt(0.3, 0.6);
    glow_animation->setKeyValueAt(0.6, 0.3);
    glow_animation->setEndValue(0.0);

    // Grupa animacji - używa osobnej pętli czasowej
    auto animation_group = new QParallelAnimationGroup(this);
    animation_group->addAnimation(opacity_animation);
    animation_group->addAnimation(glow_animation);

    // Podłączamy sygnał zakończenia
    connect(animation_group, &QParallelAnimationGroup::finished, this, [this, animation_group]() {
        animation_group->deleteLater();
        hide();
        emit hidden();
    });

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void StreamMessage::UpdateScrollAreaMaxHeight() const {
    if (!scroll_area_)
        return;

    // Znajdź ekran, na którym wyświetlana jest wiadomość
    const QScreen* screen = QApplication::primaryScreen();
    if (window()) {
        screen = window()->screen();
    }
    const QRect available_geometry = screen->availableGeometry();

    // Ograniczamy maksymalną wysokość scrollArea do 50% dostępnej wysokości ekranu
    const int max_height = available_geometry.height() * 0.5;
    scroll_area_->setMaximumHeight(max_height);

    // Aktualizujemy rozmiar widget'u wewnątrz scrollArea
    if (QWidget* content_widget = scroll_area_->widget()) {
        // Upewniamy się, że widget ma wystarczającą szerokość
        content_widget->setMinimumWidth(scroll_area_->width() - 20);
    }
}

void StreamMessage::StartDisintegrationAnimation() {
    if (graphicsEffect() && graphicsEffect() != shutdown_effect_) {
        delete graphicsEffect();
    }

    shutdown_effect_ = new ElectronicShutdownEffect(this);
    shutdown_effect_->SetProgress(0.0);
    setGraphicsEffect(shutdown_effect_);

    const auto shutdown_animation = new QPropertyAnimation(this, "shutdownProgress");
    shutdown_animation->setDuration(1200); // Nieco szybsza animacja (1.2 sekundy)
    shutdown_animation->setStartValue(0.0);
    shutdown_animation->setEndValue(1.0);
    shutdown_animation->setEasingCurve(QEasingCurve::InQuad);

    connect(shutdown_animation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        emit hidden();
    });

    shutdown_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void StreamMessage::ShowNavigationButtons(const bool has_previous, const bool has_next) const {
    // Pozycjonowanie przycisków nawigacyjnych
    next_button_->setVisible(has_next);
    prev_button_->setVisible(has_previous);
    mark_read_button->setVisible(true);

    UpdateLayout();

    // Frontowanie przycisków
    next_button_->raise();
    prev_button_->raise();
    mark_read_button->raise();
}

void StreamMessage::MarkAsRead() {
    if (!is_read_) {
        is_read_ = true;

        // Wybierz odpowiednią animację zamykania w zależności od typu wiadomości
        if (scroll_area_ && long_text_display_) {
            // Dla długich wiadomości używamy prostszej animacji
            StartLongMessageClosingAnimation();
        } else {
            // Dla standardowych wiadomości zachowujemy oryginalną animację
            StartDisintegrationAnimation();
        }

        emit messageRead();
    }
}

void StreamMessage::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Wybieramy kolory w stylu cyberpunk zależnie od typu wiadomości
    QColor background_color, border_color, glow_color, text_color;

    switch (type_) {
        case kTransmitted:
            // Neonowy niebieski dla wysyłanych
            background_color = QColor(0, 20, 40, 180);
            border_color = QColor(0, 200, 255);
            glow_color = QColor(0, 150, 255, 80);
            text_color = QColor(0, 220, 255);
            break;
        case kReceived:
            // Różowo-fioletowy dla przychodzących
            background_color = QColor(30, 0, 30, 180);
            border_color = QColor(220, 50, 255);
            glow_color = QColor(180, 0, 255, 80);
            text_color = QColor(240, 150, 255);
            break;
        case kSystem:
            // Żółto-pomarańczowy dla systemowych
            background_color = QColor(40, 25, 0, 180);
            border_color = QColor(255, 180, 0);
            glow_color = QColor(255, 150, 0, 80);
            text_color = QColor(255, 200, 0);
            break;
    }

    // Tworzymy ściętą formę geometryczną z większą liczbą ścięć (bardziej futurystyczną)
    QPainterPath path;
    int clip_size = 20; // rozmiar ścięcia rogu
    int notch_size = 10; // rozmiar wcięcia

    // Górna krawędź - prosta bez wcięć
    path.moveTo(clip_size, 0);
    path.lineTo(width() - clip_size, 0);

    // Prawy górny róg i prawa krawędź z wcięciem
    path.lineTo(width(), clip_size);
    path.lineTo(width(), height() / 2 - notch_size);
    path.lineTo(width() - notch_size, height() / 2);
    path.lineTo(width(), height() / 2 + notch_size);
    path.lineTo(width(), height() - clip_size);

    // Prawy dolny róg i dolna krawędź
    path.lineTo(width() - clip_size, height());
    path.lineTo(clip_size, height());

    // Lewy dolny róg i lewa krawędź z wcięciem
    path.lineTo(0, height() - clip_size);
    path.lineTo(0, height() / 2 + notch_size);
    path.lineTo(notch_size, height() / 2);
    path.lineTo(0, height() / 2 - notch_size);
    path.lineTo(0, clip_size);

    // Zamknięcie ścieżki
    path.closeSubpath();

    // Tło z gradientem
    QLinearGradient background_gradient(0, 0, width(), height());
    background_gradient.setColorAt(0, background_color.lighter(110));
    background_gradient.setColorAt(1, background_color);

    painter.setPen(Qt::NoPen);
    painter.setBrush(background_gradient);
    painter.drawPath(path);

    // Poświata neonu
    if (glow_intensity_ > 0.1) {
        painter.setPen(QPen(glow_color, 6 + glow_intensity_ * 6));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    // Neonowe obramowanie
    painter.setPen(QPen(border_color, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Linie dekoracyjne (poziome)
    painter.setPen(QPen(border_color.lighter(120), 1, Qt::DashLine));
    painter.drawLine(clip_size, 30, width() - clip_size, 30);
    painter.drawLine(40, height() - 25, width() - 40, height() - 25);

    // Linie dekoracyjne (pionowe)
    painter.setPen(QPen(border_color.lighter(120), 1, Qt::DotLine));
    painter.drawLine(40, 30, 40, height() - 25);
    painter.drawLine(width() - 40, 30, width() - 40, height() - 25);

    // Znaczniki AR w rogach
    int ar_marker_size = 15;
    painter.setPen(QPen(border_color, 1, Qt::SolidLine));

    // Lewy górny marker
    painter.drawLine(clip_size, 10, clip_size + ar_marker_size, 10);
    painter.drawLine(clip_size, 10, clip_size, 10 + ar_marker_size);

    // Prawy górny marker
    painter.drawLine(width() - clip_size - ar_marker_size, 10, width() - clip_size, 10);
    painter.drawLine(width() - clip_size, 10, width() - clip_size, 10 + ar_marker_size);

    // Tekst nagłówka (nadawca)
    painter.setPen(QPen(text_color, 1));
    painter.setFont(QFont("Consolas", 10, QFont::Bold));
    painter.drawText(QRect(clip_size + 5, 7, width() - 2*clip_size - 10, 22),
                     Qt::AlignLeft | Qt::AlignVCenter, sender_);

    // Znacznik czasu i lokalizacji w stylu AR
    QDateTime current_time = QDateTime::currentDateTime();
    QString time_string = current_time.toString("HH:mm:ss");


    // Dodanie znaczników w prawym górnym rogu
    painter.setFont(QFont("Consolas", 8));
    painter.setPen(QPen(text_color.lighter(120), 1));

    // Timestamp
    painter.drawText(QRect(width() - 150, 12, 120, 12),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QString("TS: %1").arg(time_string));

    // Wskaźnik priorytetu
    QColor priority_color;
    QString priority_text;

    switch (type_) {
        case kTransmitted:
            priority_text = "OUT";
            priority_color = QColor(0, 220, 255);
            break;
        case kReceived:
            priority_text = "IN";
            priority_color = QColor(255, 50, 240);
            break;
        case kSystem:
            priority_text = "SYS";
            priority_color = QColor(255, 220, 0);
            break;
    }

    // Rysujemy ramkę wskaźnika priorytetów
    QRect priority_rect(width() - 70, height() - 40, 60, 20);
    painter.setPen(QPen(priority_color, 1, Qt::SolidLine));
    painter.setBrush(QBrush(priority_color.darker(600)));
    painter.drawRect(priority_rect);

    // Rysujemy tekst priorytetów
    painter.setPen(QPen(priority_color, 1));
    painter.setFont(QFont("Consolas", 8, QFont::Bold));
    painter.drawText(priority_rect, Qt::AlignCenter, priority_text);

}

void StreamMessage::resizeEvent(QResizeEvent *event) {
    UpdateLayout(); // Istniejąca funkcja do aktualizacji przycisków

    // Dostosuj rozmiar scrollArea jeśli istnieje
    if (scroll_area_) {
        if (QWidget* content_widget = scroll_area_->widget()) {
            // Ustaw minimalną szerokość contentu, aby zapewnić poprawne przewijanie
            content_widget->setMinimumWidth(scroll_area_->width() - 30);
        }
    }

    QWidget::resizeEvent(event);
}

void StreamMessage::keyPressEvent(QKeyEvent *event) {
    // Obsługa klawiszy bezpośrednio w widgecie wiadomości
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
        MarkAsRead();
        event->accept();
    } else if (event->key() == Qt::Key_Right && next_button_->isVisible()) {
        next_button_->click();
        event->accept();
    } else if (event->key() == Qt::Key_Left && prev_button_->isVisible()) {
        prev_button_->click();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void StreamMessage::focusInEvent(QFocusEvent *event) {
    // Dodajemy subtelny efekt podświetlenia przy focusie
    glow_intensity_ += 0.2;
    update();
    QWidget::focusInEvent(event);
}

void StreamMessage::focusOutEvent(QFocusEvent *event) {
    // Wracamy do normalnego stanu
    glow_intensity_ -= 0.2;
    update();
    QWidget::focusOutEvent(event);
}

void StreamMessage::UpdateAnimation() {
    // Subtelna pulsacja poświaty
    const qreal pulse = 0.05 * sin(QDateTime::currentMSecsSinceEpoch() * 0.002);
    SetGlowIntensity(glow_intensity_ + pulse * (!is_read_ ? 1.0 : 0.3));
}

void StreamMessage::AdjustScrollAreaStyle() const {
    if (!scroll_area_) return;

    // Typy wiadomości mają różne kolory
    QString handle_color;
    QColor text_color;

    switch (type_) {
        case kTransmitted:
            handle_color = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00c8ff, stop:1 #0080ff)";
            text_color = QColor(0, 220, 255);
            break;
        case kReceived:
            handle_color = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff50dd, stop:1 #b000ff)";
            text_color = QColor(240, 150, 255);
            break;
        case kSystem:
            handle_color = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ffb400, stop:1 #ff8000)";
            text_color = QColor(255, 200, 0);
            break;
    }

    // Utwórz styl z odpowiednim kolorem scrollbara
    QString style = QString(
                "QScrollArea { background: transparent; border: none; }"
                "QScrollBar:vertical { background: rgba(10, 20, 30, 150); width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: %1; "
                "border-radius: 5px; min-height: 30px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: rgba(0, 20, 40, 100); }")
            .arg(handle_color);

    scroll_area_->setStyleSheet(style);

    // Aktualizacja koloru tekstu jeśli istnieje longTextDisplay
    if (long_text_display_) {
        long_text_display_->SetTextColor(text_color);
    }
}

QString StreamMessage::ExtractAttribute(const QString &html, const QString &attribute) {
    int attribute_position = html.indexOf(attribute + "='");
    if (attribute_position >= 0) {
        attribute_position += attribute.length() + 2; // przesunięcie za ='
        const int endPos = html.indexOf("'", attribute_position);
        if (endPos >= 0) {
            return html.mid(attribute_position, endPos - attribute_position);
        }
    }
    return QString();
}

void StreamMessage::StartLongMessageClosingAnimation() {
    // Prostszy, ale nadal cyberpunkowy efekt dla długich wiadomości
    const auto animation_group = new QSequentialAnimationGroup(this);

    // Faza 1: Szybkie migotanie (efekt glitch)
    const auto glitch_animation = new QPropertyAnimation(this, "glowIntensity");
    glitch_animation->setDuration(300);
    glitch_animation->setStartValue(0.8);
    glitch_animation->setKeyValueAt(0.2, 1.2);
    glitch_animation->setKeyValueAt(0.4, 0.3);
    glitch_animation->setKeyValueAt(0.6, 1.0);
    glitch_animation->setKeyValueAt(0.8, 0.5);
    glitch_animation->setEndValue(0.9);
    glitch_animation->setEasingCurve(QEasingCurve::OutInQuad);
    animation_group->addAnimation(glitch_animation);

    // Faza 2: Szybkie zniknięcie całej wiadomości
    const auto fade_animation = new QPropertyAnimation(this, "opacity");
    fade_animation->setDuration(250);  // krótki czas = szybka animacja
    fade_animation->setStartValue(1.0);
    fade_animation->setEndValue(0.0);
    fade_animation->setEasingCurve(QEasingCurve::OutQuad);
    animation_group->addAnimation(fade_animation);

    // Po zakończeniu animacji - ukryj wiadomość i wyemituj sygnał
    connect(animation_group, &QSequentialAnimationGroup::finished, this, [this]() {
        hide();
        emit hidden();
    });

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void StreamMessage::ProcessImageAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    // Tworzymy widget zawierający placeholder załącznika
    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    // Tworzymy placeholder załącznika
    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "image", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    // Ustawiamy widget załącznika
    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    // Zwiększamy rozmiar wiadomości
    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::ProcessGifAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    // Tworzymy widget zawierający placeholder załącznika
    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    // Tworzymy placeholder załącznika
    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "gif", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    // Ustawiamy widget załącznika
    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    // Zwiększamy rozmiar wiadomości
    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::ProcessAudioAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    // Tworzymy widget zawierający placeholder załącznika
    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    // Tworzymy placeholder załącznika
    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "audio", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    // Ustawiamy widget załącznika
    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    // Zwiększamy rozmiar wiadomości
    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::ProcessVideoAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    // Tworzymy widget zawierający placeholder załącznika
    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    // Tworzymy placeholder załącznika
    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "video", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    // Ustawiamy widget załącznika
    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    // Zwiększamy rozmiar wiadomości
    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::CleanupContent() {
    clean_content_ = content_;

    // Usuń wszystkie znaczniki HTML
    clean_content_.remove(QRegExp("<[^>]*>"));

    // Zajmij się placeholderami załączników
    if (content_.contains("placeholder")) {
        const int placeholder_start = content_.indexOf("<div class='");
        if (placeholder_start >= 0) {
            const int placeholder_end = content_.indexOf("</div>", placeholder_start);
            if (placeholder_end > 0) {
                const QString before = clean_content_.left(placeholder_start);
                const QString after = clean_content_.mid(placeholder_end + 6);
                clean_content_ = before.trimmed() + " " + after.trimmed();
            }
        }
    }

    // Poprawki dla znaków specjalnych HTML
    clean_content_.replace("&nbsp;", " ");
    clean_content_.replace("&lt;", "<");
    clean_content_.replace("&gt;", ">");
    clean_content_.replace("&amp;", "&");

    // Dla długich tekstów zachowaj formatowanie akapitów
    if (clean_content_.length() > 300) {
        // Zachowaj oryginalny podział na linie
        clean_content_.replace("\n\n", "||PARAGRAPH||");
        clean_content_ = clean_content_.simplified();
        clean_content_.replace("||PARAGRAPH||", "\n\n");

        // Dodatkowe poprawki dla bardziej czytelnego formatowania
        clean_content_.replace(". ", ".\n");
        clean_content_.replace("? ", "?\n");
        clean_content_.replace("! ", "!\n");
        clean_content_.replace(":\n", ": ");
    } else {
        clean_content_ = clean_content_.simplified();
    }
}

void StreamMessage::UpdateLayout() const {
    // Pozycjonowanie przycisków nawigacyjnych
    if (next_button_->isVisible()) {
        next_button_->move(width() - next_button_->width() - 10, height() / 2 - next_button_->height() / 2);
    }
    if (prev_button_->isVisible()) {
        prev_button_->move(10, height() / 2 - prev_button_->height() / 2);
    }
    if (mark_read_button->isVisible()) {
        // Zmieniona pozycja przycisku odczytu - prawy dolny róg
        mark_read_button->move(width() - mark_read_button->width() - 10, height() - mark_read_button->height() - 10);
    }
}
