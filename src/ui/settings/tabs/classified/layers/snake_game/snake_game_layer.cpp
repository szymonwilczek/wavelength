#include "snake_game_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QRandomGenerator>

#include "../../../../../../app/managers/translation_manager.h"

SnakeGameLayer::SnakeGameLayer(QWidget *parent) 
    : SecurityLayer(parent),
      game_timer_(nullptr),
      border_animation_timer_(nullptr),
      direction_(Direction::Right),
      last_processed_direction_(Direction::Right),
      apples_collected_(0),
      game_over_(false),
      game_state_(GameState::Playing),
      animation_progress_(0),
      parts_exited_(0)
{
    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(15);
    translator_ = TranslationManager::GetInstance();

    const auto title = new QLabel(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.Title",
        "SECURITY VERIFICATION: SNAKE GAME"), this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    const auto instructions = new QLabel(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.Info",
        "Collect 4 apples to open exit\nControls: Arrrows"), this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    // Plansza gry - usuwamy obramowanie z CSS
    game_board_ = new QLabel(this);
    game_board_->setFixedSize(kGridSize * kCellSize, kGridSize * kCellSize);
    game_board_->setStyleSheet("background-color: rgba(10, 25, 40, 220);"); // Bez bordera
    game_board_->setAlignment(Qt::AlignCenter);
    game_board_->installEventFilter(this);

    // Etykieta wyniku
    score_label_ = new QLabel(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.CollectedApples0",
        "Collected apples: 0/4"), this);
    score_label_->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    score_label_->setAlignment(Qt::AlignCenter);

    layout->addWidget(title);
    layout->addWidget(instructions);
    layout->addWidget(game_board_, 0, Qt::AlignCenter);
    layout->addWidget(score_label_);
    layout->addStretch();

    // Inicjalizacja timera gry - zmniejszamy interwał dla większej płynności
    game_timer_ = new QTimer(this);
    game_timer_->setInterval(150); // 150ms dla płynności
    connect(game_timer_, &QTimer::timeout, this, &SnakeGameLayer::UpdateGame);

    // Timer animacji otwierania granicy
    border_animation_timer_ = new QTimer(this);
    border_animation_timer_->setInterval(50); // 50ms dla płynnej animacji
    connect(border_animation_timer_, &QTimer::timeout, this, &SnakeGameLayer::UpdateBorderAnimation);

    // Ustawienie focusu
    setFocusPolicy(Qt::StrongFocus);

    // Losowanie strony wyjścia już na początku
    int exit_side_random = QRandomGenerator::global()->bounded(2);
    exit_side_ = static_cast<BorderSide>(exit_side_random);

    // Losowa pozycja Y dla wyjścia (unikamy narożników)
    exit_position_ = QRandomGenerator::global()->bounded(1, kGridSize - 1);
}

SnakeGameLayer::~SnakeGameLayer() {
    if (game_timer_) {
        game_timer_->stop();
        delete game_timer_;
        game_timer_ = nullptr;
    }

    if (border_animation_timer_) {
        border_animation_timer_->stop();
        delete border_animation_timer_;
        border_animation_timer_ = nullptr;
    }
}

void SnakeGameLayer::Initialize() {
    Reset(); // Reset najpierw
    InitializeGame(); // Inicjalizuj logikę gry
    setFocus(); // Ustaw focus na warstwie
    game_board_->setFocus(); // Ustaw focus na planszy (ważne dla eventFilter)

    // Upewnij się, że widget jest widoczny przed startem
    if (graphicsEffect()) {
         static_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }

    // Rozpocznij grę po krótkim opóźnieniu
    QTimer::singleShot(200, this, [this]() {
        if (!game_over_) { // Sprawdź, czy gra nie została już zakończona (np. przez szybki reset)
            game_timer_->start();
        }
    });
}

void SnakeGameLayer::Reset() {
    // Zatrzymaj timery
    if (game_timer_ && game_timer_->isActive()) {
        game_timer_->stop();
    }
    if (border_animation_timer_ && border_animation_timer_->isActive()) {
        border_animation_timer_->stop();
    }

    // Resetuj stan gry
    apples_collected_ = 0;
    game_over_ = false; // Kluczowe - resetuj stan game over
    game_state_ = GameState::Playing;
    animation_progress_ = 0;
    parts_exited_ = 0;
    exit_points_.clear();
    snake_.clear(); // Wyczyść węża
    direction_ = Direction::Right; // Resetuj kierunek
    last_processed_direction_ = Direction::Right;

    // Ponowne losowanie strony wyjścia i pozycji
    int exit_side_random = QRandomGenerator::global()->bounded(2);
    exit_side_ = static_cast<BorderSide>(exit_side_random);
    exit_position_ = QRandomGenerator::global()->bounded(1, kGridSize - 1);

    // Resetuj UI
    score_label_->setText(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.CollectedApples0",
        "Collected apples: 0/4"));
    score_label_->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;"); // Szary tekst wyniku
    game_board_->setStyleSheet("background-color: rgba(10, 25, 40, 220);"); // Domyślne tło planszy (bez bordera)
    game_board_->clear();

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void SnakeGameLayer::InitializeGame() {
    // Wyczyszczenie planszy
    for (int i = 0; i < kGridSize; i++) {
        for (int j = 0; j < kGridSize; j++) {
            grid_[i][j] = CellType::Empty;
        }
    }

    // Inicjalizacja węża
    snake_.clear();
    // Zaczynamy od środka planszy
    constexpr int start_x = kGridSize / 2;
    constexpr int start_y = kGridSize / 2;

    snake_.append(QPair(start_x, start_y));     // Głowa
    snake_.append(QPair(start_x-1, start_y));   // Ciało
    snake_.append(QPair(start_x-2, start_y));   // Ogon

    grid_[start_x][start_y] = CellType::SnakeHead;
    grid_[start_x-1][start_y] = CellType::Snake;
    grid_[start_x-2][start_y] = CellType::Snake;

    // Początkowo wąż porusza się w prawo
    direction_ = Direction::Right;
    last_processed_direction_ = Direction::Right;

    // Generowanie jabłka
    GenerateApple();

    // Renderowanie planszy
    RenderGame();
}

void SnakeGameLayer::RenderGame() const {
    // Rysowanie planszy
    QImage board(kGridSize * kCellSize, kGridSize * kCellSize, QImage::Format_ARGB32);
    board.fill(QColor(10, 25, 40, 220));

    QPainter painter(&board);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysowanie siatki
    painter.setPen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    for (int i = 1; i < kGridSize; i++) {
        // Linie poziome
        painter.drawLine(0, i * kCellSize, kGridSize * kCellSize, i * kCellSize);
        // Linie pionowe
        painter.drawLine(i * kCellSize, 0, i * kCellSize, kGridSize * kCellSize);
    }

    // Rysowanie obramowania planszy
    QPen border_pen(QColor(255, 51, 51), 2, Qt::SolidLine);
    painter.setPen(border_pen);

    // Rysowanie obramowania z uwzględnieniem otwartego wyjścia
    // Górna linia
    painter.drawLine(0, 0, kGridSize * kCellSize, 0);

    // Dolna linia
    painter.drawLine(0, kGridSize * kCellSize, kGridSize * kCellSize, kGridSize * kCellSize);

    // Jeśli jesteśmy w trybie wyjścia, narysuj specjalne obramowanie z otworem
    if (game_state_ == GameState::ExitOpen) {
        // Rysujemy lewe i prawe obramowanie w zależności od tego, gdzie jest wyjście

        if (exit_side_ == BorderSide::Left) {
            // Lewa strona ma otwór
            // Górna część lewego obramowania
            painter.drawLine(0, 0, 0, exit_position_ * kCellSize);
            // Dolna część lewego obramowania
            painter.drawLine(0, (exit_position_ + 1) * kCellSize, 0, kGridSize * kCellSize);

            // Otwarta część z animacją
            QColor open_color(50, 255, 50); // Zielony kolor dla otwartego przejścia
            painter.setPen(QPen(open_color, 2, Qt::DotLine));

            // Stopniowe otwieranie - w zależności od postępu animacji
            int open_position = std::min(animation_progress_ / 2, kCellSize);
            painter.drawLine(open_position, exit_position_ * kCellSize, open_position, (exit_position_ + 1) * kCellSize);

            // Prawa strona pełna
            painter.setPen(border_pen);
            painter.drawLine(kGridSize * kCellSize, 0, kGridSize * kCellSize, kGridSize * kCellSize);

            // Narysuj strzałkę wskazującą wyjście
            QColor arrow_color(50, 255, 50, 150 + (animation_progress_ % 2) * 50); // Pulsująca zielona strzałka
            painter.setPen(QPen(arrow_color, 2));
            painter.setBrush(QBrush(arrow_color));

            QPolygonF arrow;
            int arrow_size = 6;
            arrow << QPointF(5, exit_position_ * kCellSize + kCellSize/2)
                  << QPointF(arrow_size * 1.5, exit_position_ * kCellSize + kCellSize/2 - arrow_size)
                  << QPointF(arrow_size * 1.5, exit_position_ * kCellSize + kCellSize/2 + arrow_size);

            painter.drawPolygon(arrow);
        }
        else { // m_exitSide == BorderSide::Right
            // Prawa strona ma otwór
            // Górna część prawego obramowania
            painter.drawLine(kGridSize * kCellSize, 0, kGridSize * kCellSize, exit_position_ * kCellSize);
            // Dolna część prawego obramowania
            painter.drawLine(kGridSize * kCellSize, (exit_position_ + 1) * kCellSize, kGridSize * kCellSize, kGridSize * kCellSize);

            // Otwarta część z animacją
            QColor open_color(50, 255, 50); // Zielony kolor dla otwartego przejścia
            painter.setPen(QPen(open_color, 2, Qt::DotLine));

            // Stopniowe otwieranie - w zależności od postępu animacji
            int open_position = kGridSize * kCellSize - std::min(animation_progress_ / 2, kCellSize);
            painter.drawLine(open_position, exit_position_ * kCellSize, open_position, (exit_position_ + 1) * kCellSize);

            // Lewa strona pełna
            painter.setPen(border_pen);
            painter.drawLine(0, 0, 0, kGridSize * kCellSize);

            // Narysuj strzałkę wskazującą wyjście
            QColor arrow_color(50, 255, 50, 150 + (animation_progress_ % 2) * 50); // Pulsująca zielona strzałka
            painter.setPen(QPen(arrow_color, 2));
            painter.setBrush(QBrush(arrow_color));

            QPolygonF arrow;
            int arrow_size = 6;
            arrow << QPointF(kGridSize * kCellSize - 5, exit_position_ * kCellSize + kCellSize/2)
                  << QPointF(kGridSize * kCellSize - arrow_size * 1.5, exit_position_ * kCellSize + kCellSize/2 - arrow_size)
                  << QPointF(kGridSize * kCellSize - arrow_size * 1.5, exit_position_ * kCellSize + kCellSize/2 + arrow_size);

            painter.drawPolygon(arrow);
        }
    } else {
        // Standardowe obramowanie bez wyjścia
        painter.drawLine(0, 0, 0, kGridSize * kCellSize);  // Lewa
        painter.drawLine(kGridSize * kCellSize, 0, kGridSize * kCellSize, kGridSize * kCellSize);  // Prawa
    }

    // Rysowanie elementów gry
    for (int i = 0; i < kGridSize; i++) {
        for (int j = 0; j < kGridSize; j++) {
            QRect cell_rect(i * kCellSize, j * kCellSize, kCellSize, kCellSize);
            QRect stem_rect(cell_rect.center().x() - 1, cell_rect.top() + 2, 2, 4);
            cell_rect.adjust(2, 2, -2, -2); // Margines

            // Tylko komórki w granicach planszy
            if (i >= 0 && j >= 0) {
                switch (grid_[i][j]) {
                    case CellType::SnakeHead:
                        // Głowa węża - jaśniejszy zielony
                            painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(50, 240, 50));
                    painter.drawRoundedRect(cell_rect, 5, 5);
                    break;

                    case CellType::Snake:
                        // Ciało węża - zielony
                            painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(30, 200, 30));
                    painter.drawRoundedRect(cell_rect, 5, 5);
                    break;

                    case CellType::Apple:
                        // Jabłko - czerwone
                            painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(220, 40, 40));
                    painter.drawEllipse(cell_rect);

                    // Szypułka jabłka
                    painter.setBrush(QColor(100, 70, 20));
                    painter.drawRect(stem_rect);
                    break;

                    default:
                        break;
                }
            }
        }
    }

    painter.end();

    game_board_->setPixmap(QPixmap::fromImage(board));
}

void SnakeGameLayer::MoveSnake() {
    if (game_over_) {
        return;
    }

    // Zapamiętanie ostatniego przetwarzanego kierunku
    last_processed_direction_ = direction_;

    // Pobranie pozycji głowy węża
    const int head_x = snake_.first().first;
    const int head_y = snake_.first().second;

    // Obliczenie nowej pozycji głowy
    int new_head_x = head_x;
    int new_head_y = head_y;

    switch (direction_) {
        case Direction::Up:
            new_head_y--;
            break;
        case Direction::Right:
            new_head_x++;
            break;
        case Direction::Down:
            new_head_y++;
            break;
        case Direction::Left:
            new_head_x--;
            break;
    }

    // W trybie wyjścia sprawdzamy, czy wąż wychodzi przez otwarte wyjście
    if (game_state_ == GameState::ExitOpen && IsExitPoint(new_head_x, new_head_y)) {
        // Rejestrujemy, że segment węża opuścił planszę
        parts_exited_++;

        // Sprawdzamy, czy cały wąż opuścił planszę
        if (parts_exited_ >= snake_.size()) {
            // Zamiast finishGame(true), najpierw zabezpieczamy stan gry
            game_over_ = true;
            QTimer::singleShot(10, this, [this]() {
                FinishGame(true);
            });
            return;
        }

        // Bezpieczne usunięcie typu komórki dla starej głowy (tylko jeśli jest w granicach planszy)
        if (head_x >= 0 && head_x < kGridSize && head_y >= 0 && head_y < kGridSize) {
            grid_[head_x][head_y] = CellType::Snake;
        }

        // W przypadku wychodzenia poza planszę, usuwamy ostatni segment bez względu na jabłko
        const QPair<int, int> tail = snake_.last();
        // Bezpieczne usunięcie ostatniego segmentu (tylko jeśli jest w granicach planszy)
        if (tail.first >= 0 && tail.first < kGridSize && tail.second >= 0 && tail.second < kGridSize) {
            grid_[tail.first][tail.second] = CellType::Empty;
        }
        snake_.removeLast();

        // Dodajemy nową głowę poza planszą
        snake_.prepend(QPair(new_head_x, new_head_y));

        // Aktualizacja widoku
        RenderGame();
        return;
    }

    // Sprawdzenie kolizji w normalnej grze
    if (IsCollision(new_head_x, new_head_y)) {
        // W trybie wyjścia, jeśli jest to kolizja ze ścianą, sprawdzamy czy to punkt wyjścia
        if (game_state_ == GameState::ExitOpen &&
           ((new_head_x < 0 && exit_side_ == BorderSide::Left) ||
            (new_head_x >= kGridSize && exit_side_ == BorderSide::Right))) {

            // Sprawdzamy, czy kolizja jest na wysokości wyjścia
            if (new_head_y == exit_position_) {
                // To jest punkt wyjścia - dodajemy segment i kontynuujemy
                snake_.prepend(QPair(new_head_x, new_head_y));
                parts_exited_++;

                // Usunięcie ogona
                const QPair<int, int> tail = snake_.last();
                grid_[tail.first][tail.second] = CellType::Empty;
                snake_.removeLast();

                // Sprawdzamy, czy połowa węża opuściła planszę
                if (parts_exited_ >= snake_.size()) {
                    FinishGame(true);
                }
                return;
            }
        }

        // Standardowa kolizja
        game_over_ = true;
        FinishGame(false);
        return;
    }

    // Sprawdzenie, czy wąż zebrał jabłko
    const bool apple_collected = (new_head_x == apple_.first && new_head_y == apple_.second);

    // Aktualizacja pozycji węża
    // Usunięcie typu komórki dla starej głowy
    grid_[head_x][head_y] = CellType::Snake;

    // Dodanie nowej głowy
    snake_.prepend(QPair(new_head_x, new_head_y));

    // Jeśli nowa głowa jest w granicach planszy, aktualizujemy grid
    if (new_head_x >= 0 && new_head_x < kGridSize && new_head_y >= 0 && new_head_y < kGridSize) {
        grid_[new_head_x][new_head_y] = CellType::SnakeHead;
    }

    // Jeśli nie zebrano jabłka, usunięcie ostatniego segmentu
    if (!apple_collected) {
        const QPair<int, int> tail = snake_.last();
        snake_.removeLast();

        // Jeśli ogon jest w granicach planszy, aktualizujemy grid
        if (tail.first >= 0 && tail.first < kGridSize && tail.second >= 0 && tail.second < kGridSize) {
            grid_[tail.first][tail.second] = CellType::Empty;
        }
    } else {
        // Jeśli zebrano jabłko
        apples_collected_++;
        score_label_->setText(QString("%1: %2/4")
            .arg(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.CollectedApples", "Collected apples"))
            .arg(apples_collected_));

        // Generowanie nowego jabłka (jeśli nie zebrano jeszcze 4)
        if (apples_collected_ < 4) {
            GenerateApple();
        } else {
            // Po zebraniu 4 jabłka - otwórz wyjście
            OpenBorder();
        }
    }

    // Aktualizacja widoku
    RenderGame();
}

void SnakeGameLayer::GenerateApple() {
    // Znajdowanie wolnych komórek
    QVector<QPair<int, int>> empty_cells;

    for (int i = 0; i < kGridSize; i++) {
        for (int j = 0; j < kGridSize; j++) {
            if (grid_[i][j] == CellType::Empty) {
                empty_cells.append(QPair(i, j));
            }
        }
    }

    if (empty_cells.isEmpty()) {
        return; // Brak wolnych komórek
    }

    // Wybór losowej pustej komórki
    const int random_index = QRandomGenerator::global()->bounded(empty_cells.size());
    apple_ = empty_cells.at(random_index);

    // Ustawienie jabłka na planszy
    grid_[apple_.first][apple_.second] = CellType::Apple;
}

bool SnakeGameLayer::IsCollision(const int x, const int y) const {
    // Kolizja ze ścianą - w trybie wyjścia sprawdzamy czy to punkt wyjścia
    if (x < 0 || x >= kGridSize || y < 0 || y >= kGridSize) {
        if (game_state_ == GameState::ExitOpen) {
            // Jeśli to wyjście po lewej i x < 0 lub wyjście po prawej i x >= GRID_SIZE
            if ((x < 0 && exit_side_ == BorderSide::Left) ||
                (x >= kGridSize && exit_side_ == BorderSide::Right)) {
                // Sprawdzamy, czy y jest na wysokości wyjścia
                return y != exit_position_;
            }
        }
        return true; // Kolizja ze ścianą w innych przypadkach
    }

    // Kolizja z wężem
    return grid_[x][y] == CellType::Snake || grid_[x][y] == CellType::SnakeHead;
}

void SnakeGameLayer::HandleInput(const int key) {
    Direction new_direction = direction_;

    switch (key) {
        case Qt::Key_Up:
            new_direction = Direction::Up;
            break;
        case Qt::Key_Right:
            new_direction = Direction::Right;
            break;
        case Qt::Key_Down:
            new_direction = Direction::Down;
            break;
        case Qt::Key_Left:
            new_direction = Direction::Left;
            break;
        default:
            return; // Ignorowanie innych klawiszy
    }

    // Zapobieganie zawracaniu o 180 stopni
    if ((new_direction == Direction::Up && last_processed_direction_ == Direction::Down) ||
        (new_direction == Direction::Down && last_processed_direction_ == Direction::Up) ||
        (new_direction == Direction::Left && last_processed_direction_ == Direction::Right) ||
        (new_direction == Direction::Right && last_processed_direction_ == Direction::Left)) {
        return;
    }

    direction_ = new_direction;
}

void SnakeGameLayer::keyPressEvent(QKeyEvent* event) {
    HandleInput(event->key());
    event->accept();
}

bool SnakeGameLayer::eventFilter(QObject* watched, QEvent* event) {
    if (watched == game_board_ && event->type() == QEvent::KeyPress) {
        const auto key_event = static_cast<QKeyEvent*>(event);
        HandleInput(key_event->key());
        return true;
    }
    return SecurityLayer::eventFilter(watched, event);
}

void SnakeGameLayer::UpdateGame() {
    if (!game_over_) {
        MoveSnake();
    }
}

void SnakeGameLayer::FinishGame(const bool success) {
    game_timer_->stop();
    border_animation_timer_->stop();

    if (success) {
        // Ustawienie stanu gry na zakończony sukcesem
        game_state_ = GameState::Completed;

        // Zakończenie powodzeniem - zielone obramowanie
        game_board_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33;");

        // Aktualizacja etykiety wyniku
        score_label_->setText(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.SnakeExited",
        "Snake exited the screen! Level completed."));
        score_label_->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");

        // Krótkie opóźnienie przed zakończeniem warstwy
        QTimer::singleShot(1000, this, [this]() {
            const auto effect = new QGraphicsOpacityEffect(this);
            this->setGraphicsEffect(effect);

            const auto animation = new QPropertyAnimation(effect, "opacity");
            animation->setDuration(500);
            animation->setStartValue(1.0);
            animation->setEndValue(0.0);
            animation->setEasingCurve(QEasingCurve::OutQuad);

            connect(animation, &QPropertyAnimation::finished, this, [this]() {
                emit layerCompleted();
            });

            animation->start(QPropertyAnimation::DeleteWhenStopped);
        });
    } else {
        // Ustawienie stanu gry na zakończony niepowodzeniem
        game_state_ = GameState::Failed;

        // Niepowodzenie - czerwone tło i restart gry
        game_board_->setStyleSheet("background-color: rgba(40, 10, 10, 220);");

        // Krótkie opóźnienie przed restartem
        QTimer::singleShot(500, this, [this]() {
            Reset();
            InitializeGame();
            game_timer_->start();
        });
    }
}

void SnakeGameLayer::OpenBorder() {
    // Zmiana stanu gry
    game_state_ = GameState::ExitOpen;

    // Czyszczenie poprzednich punktów wyjścia
    exit_points_.clear();

    // Określenie punktu wyjścia w zależności od wybranej strony
    if (exit_side_ == BorderSide::Left) {
        // Lewe wyjście
        exit_points_.append(QPair<int, int>(-1, exit_position_));
    } else {
        // Prawe wyjście
        exit_points_.append(QPair<int, int>(kGridSize, exit_position_));
    }

    // Aktualizacja etykiety wyniku
    score_label_->setText(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.ExitOpened",
        "Exit opened!\nESCAPE!!!"));
    score_label_->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");

    // Inicjalizacja licznika animacji
    animation_progress_ = 0;

    // Start timera animacji
    border_animation_timer_->start();

    // Renderowanie planszy z otwartym wyjściem
    RenderGame();
}

void SnakeGameLayer::UpdateBorderAnimation() {
    // Inkrementacja licznika animacji
    animation_progress_++;

    // Odświeżenie planszy co określoną liczbę kroków
    if (animation_progress_ % 3 == 0) {
        RenderGame();
    }
}

bool SnakeGameLayer::IsExitPoint(const int x, const int y) const {
    // Sprawdzenie, czy punkt (x, y) jest punktem wyjścia
    if (exit_side_ == BorderSide::Left && x < 0 && y == exit_position_) {
        return true;
    }

    if (exit_side_ == BorderSide::Right && x >= kGridSize && y == exit_position_) {
        return true;
    }

    return false;
}

void SnakeGameLayer::CheckForExitProgress() {
    // Ten kod jest teraz obsługiwany w moveSnake()
}