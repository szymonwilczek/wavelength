#include "snake_game_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QTimer>

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
      parts_exited_(0) {
    setFocusPolicy(Qt::StrongFocus);

    const auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(15);
    translator_ = TranslationManager::GetInstance();

    const auto title = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.SnakeGame.Title",
                               "SECURITY VERIFICATION: SNAKE GAME"), this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    const auto instructions = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.SnakeGame.Info",
                               "Collect 4 apples to open exit\nControls: Arrrows"),
        this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    game_board_ = new QLabel(this);
    game_board_->setFixedSize(kGridSize * kCellSize, kGridSize * kCellSize);
    game_board_->setStyleSheet("background-color: rgba(10, 25, 40, 220);");
    game_board_->setAlignment(Qt::AlignCenter);
    game_board_->installEventFilter(this);

    score_label_ = new QLabel(
        translator_->Translate("ClassifiedSettingsWidget.SnakeGame.CollectedApples0",
                               "Collected apples: 0/4"), this);
    score_label_->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    score_label_->setAlignment(Qt::AlignCenter);

    layout->addWidget(title);
    layout->addWidget(instructions);
    layout->addWidget(game_board_, 0, Qt::AlignCenter);
    layout->addWidget(score_label_);
    layout->addStretch();

    game_timer_ = new QTimer(this);
    game_timer_->setInterval(150);
    connect(game_timer_, &QTimer::timeout, this, &SnakeGameLayer::UpdateGame);

    border_animation_timer_ = new QTimer(this);
    border_animation_timer_->setInterval(50);
    connect(border_animation_timer_, &QTimer::timeout, this, &SnakeGameLayer::UpdateBorderAnimation);

    int exit_side_random = QRandomGenerator::global()->bounded(2);
    exit_side_ = static_cast<BorderSide>(exit_side_random);
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
    Reset();
    InitializeGame();
    setFocus();
    game_board_->setFocus();

    if (graphicsEffect()) {
        static_cast<QGraphicsOpacityEffect *>(graphicsEffect())->setOpacity(1.0);
    }

    QTimer::singleShot(200, this, [this] {
        if (!game_over_) {
            game_timer_->start();
        }
    });
}

void SnakeGameLayer::Reset() {
    if (game_timer_ && game_timer_->isActive()) {
        game_timer_->stop();
    }
    if (border_animation_timer_ && border_animation_timer_->isActive()) {
        border_animation_timer_->stop();
    }

    apples_collected_ = 0;
    game_over_ = false;
    game_state_ = GameState::Playing;
    animation_progress_ = 0;
    parts_exited_ = 0;
    exit_points_.clear();
    snake_.clear();
    direction_ = Direction::Right;
    last_processed_direction_ = Direction::Right;

    int exit_side_random = QRandomGenerator::global()->bounded(2);
    exit_side_ = static_cast<BorderSide>(exit_side_random);
    exit_position_ = QRandomGenerator::global()->bounded(1, kGridSize - 1);

    score_label_->setText(
        translator_->Translate("ClassifiedSettingsWidget.SnakeGame.CollectedApples0",
                               "Collected apples: 0/4"));
    score_label_->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    game_board_->setStyleSheet("background-color: rgba(10, 25, 40, 220);");
    game_board_->clear();

    if (const auto effect = qobject_cast<QGraphicsOpacityEffect *>(this->graphicsEffect())) {
        effect->setOpacity(1.0);
    }
}

void SnakeGameLayer::InitializeGame() {
    for (int i = 0; i < kGridSize; i++) {
        for (int j = 0; j < kGridSize; j++) {
            grid_[i][j] = CellType::Empty;
        }
    }

    snake_.clear();
    constexpr int start_x = kGridSize / 2;
    constexpr int start_y = kGridSize / 2;

    snake_.append(QPair(start_x, start_y)); // head
    snake_.append(QPair(start_x - 1, start_y)); // body
    snake_.append(QPair(start_x - 2, start_y)); // tail

    grid_[start_x][start_y] = CellType::SnakeHead;
    grid_[start_x - 1][start_y] = CellType::Snake;
    grid_[start_x - 2][start_y] = CellType::Snake;

    direction_ = Direction::Right;
    last_processed_direction_ = Direction::Right;

    GenerateApple();
    RenderGame();
}

void SnakeGameLayer::RenderGame() const {
    QImage board(kGridSize * kCellSize, kGridSize * kCellSize, QImage::Format_ARGB32);
    board.fill(QColor(10, 25, 40, 220));

    QPainter painter(&board);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    for (int i = 1; i < kGridSize; i++) {
        painter.drawLine(0, i * kCellSize, kGridSize * kCellSize, i * kCellSize);
        painter.drawLine(i * kCellSize, 0, i * kCellSize, kGridSize * kCellSize);
    }

    QPen border_pen(QColor(255, 51, 51), 2, Qt::SolidLine);
    painter.setPen(border_pen);

    // top line
    painter.drawLine(0, 0, kGridSize * kCellSize, 0);

    // bottom line
    painter.drawLine(0, kGridSize * kCellSize, kGridSize * kCellSize, kGridSize * kCellSize);

    if (game_state_ == GameState::ExitOpen) {
        if (exit_side_ == BorderSide::Left) {
            painter.drawLine(0, 0, 0, exit_position_ * kCellSize);
            painter.drawLine(0, (exit_position_ + 1) * kCellSize, 0, kGridSize * kCellSize);

            QColor open_color(50, 255, 50);
            painter.setPen(QPen(open_color, 2, Qt::DotLine));

            int open_position = std::min(animation_progress_ / 2, kCellSize);
            painter.drawLine(open_position, exit_position_ * kCellSize, open_position,
                             (exit_position_ + 1) * kCellSize);

            painter.setPen(border_pen);
            painter.drawLine(kGridSize * kCellSize, 0, kGridSize * kCellSize, kGridSize * kCellSize);

            QColor arrow_color(50, 255, 50, 150 + animation_progress_ % 2 * 50);
            painter.setPen(QPen(arrow_color, 2));
            painter.setBrush(QBrush(arrow_color));

            QPolygonF arrow;
            int arrow_size = 6;
            arrow << QPointF(5, exit_position_ * kCellSize + kCellSize / 2)
                    << QPointF(arrow_size * 1.5, exit_position_ * kCellSize + kCellSize / 2 - arrow_size)
                    << QPointF(arrow_size * 1.5, exit_position_ * kCellSize + kCellSize / 2 + arrow_size);

            painter.drawPolygon(arrow);
        } else {
            painter.drawLine(kGridSize * kCellSize, 0, kGridSize * kCellSize, exit_position_ * kCellSize);
            painter.drawLine(kGridSize * kCellSize, (exit_position_ + 1) * kCellSize, kGridSize * kCellSize,
                             kGridSize * kCellSize);

            QColor open_color(50, 255, 50);
            painter.setPen(QPen(open_color, 2, Qt::DotLine));

            int open_position = kGridSize * kCellSize - std::min(animation_progress_ / 2, kCellSize);
            painter.drawLine(open_position, exit_position_ * kCellSize, open_position,
                             (exit_position_ + 1) * kCellSize);

            painter.setPen(border_pen);
            painter.drawLine(0, 0, 0, kGridSize * kCellSize);

            QColor arrow_color(50, 255, 50, 150 + animation_progress_ % 2 * 50);
            painter.setPen(QPen(arrow_color, 2));
            painter.setBrush(QBrush(arrow_color));

            QPolygonF arrow;
            int arrow_size = 6;
            arrow << QPointF(kGridSize * kCellSize - 5, exit_position_ * kCellSize + kCellSize / 2)
                    << QPointF(kGridSize * kCellSize - arrow_size * 1.5,
                               exit_position_ * kCellSize + kCellSize / 2 - arrow_size)
                    << QPointF(kGridSize * kCellSize - arrow_size * 1.5,
                               exit_position_ * kCellSize + kCellSize / 2 + arrow_size);

            painter.drawPolygon(arrow);
        }
    } else {
        painter.drawLine(0, 0, 0, kGridSize * kCellSize);
        painter.drawLine(kGridSize * kCellSize, 0, kGridSize * kCellSize, kGridSize * kCellSize);
    }

    for (int i = 0; i < kGridSize; i++) {
        for (int j = 0; j < kGridSize; j++) {
            QRect cell_rect(i * kCellSize, j * kCellSize, kCellSize, kCellSize);
            QRect stem_rect(cell_rect.center().x() - 1, cell_rect.top() + 2, 2, 4);
            cell_rect.adjust(2, 2, -2, -2);

            if (i >= 0 && j >= 0) {
                switch (grid_[i][j]) {
                    case CellType::SnakeHead:
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(QColor(50, 240, 50));
                        painter.drawRoundedRect(cell_rect, 5, 5);
                        break;

                    case CellType::Snake:
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(QColor(30, 200, 30));
                        painter.drawRoundedRect(cell_rect, 5, 5);
                        break;

                    case CellType::Apple:
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(QColor(220, 40, 40));
                        painter.drawEllipse(cell_rect);

                        // apple stem
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

    last_processed_direction_ = direction_;

    const int head_x = snake_.first().first;
    const int head_y = snake_.first().second;

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

    if (game_state_ == GameState::ExitOpen && IsExitPoint(new_head_x, new_head_y)) {
        parts_exited_++;

        if (parts_exited_ >= snake_.size()) {
            game_over_ = true;
            QTimer::singleShot(10, this, [this] {
                FinishGame(true);
            });
            return;
        }

        if (head_x >= 0 && head_x < kGridSize && head_y >= 0 && head_y < kGridSize) {
            grid_[head_x][head_y] = CellType::Snake;
        }

        const QPair<int, int> tail = snake_.last();
        if (tail.first >= 0 && tail.first < kGridSize && tail.second >= 0 && tail.second < kGridSize) {
            grid_[tail.first][tail.second] = CellType::Empty;
        }

        snake_.removeLast();
        snake_.prepend(QPair(new_head_x, new_head_y));

        RenderGame();
        return;
    }

    if (IsCollision(new_head_x, new_head_y)) {
        if (game_state_ == GameState::ExitOpen &&
            ((new_head_x < 0 && exit_side_ == BorderSide::Left) ||
             (new_head_x >= kGridSize && exit_side_ == BorderSide::Right))) {
            if (new_head_y == exit_position_) {
                snake_.prepend(QPair(new_head_x, new_head_y));
                parts_exited_++;

                const QPair<int, int> tail = snake_.last();
                grid_[tail.first][tail.second] = CellType::Empty;
                snake_.removeLast();

                if (parts_exited_ >= snake_.size()) {
                    FinishGame(true);
                }
                return;
            }
        }

        game_over_ = true;
        FinishGame(false);
        return;
    }

    const bool apple_collected = new_head_x == apple_.first && new_head_y == apple_.second;

    grid_[head_x][head_y] = CellType::Snake;
    snake_.prepend(QPair(new_head_x, new_head_y));

    if (new_head_x >= 0 && new_head_x < kGridSize && new_head_y >= 0 && new_head_y < kGridSize) {
        grid_[new_head_x][new_head_y] = CellType::SnakeHead;
    }

    if (!apple_collected) {
        const QPair<int, int> tail = snake_.last();
        snake_.removeLast();

        if (tail.first >= 0 && tail.first < kGridSize && tail.second >= 0 && tail.second < kGridSize) {
            grid_[tail.first][tail.second] = CellType::Empty;
        }
    } else {
        apples_collected_++;
        score_label_->setText(QString("%1: %2/4")
            .arg(translator_->Translate(
                "ClassifiedSettingsWidget.SnakeGame.CollectedApples",
                "Collected apples"))
            .arg(apples_collected_));

        if (apples_collected_ < 4) {
            GenerateApple();
        } else {
            OpenBorder();
        }
    }

    RenderGame();
}

void SnakeGameLayer::GenerateApple() {
    QVector<QPair<int, int> > empty_cells;

    for (int i = 0; i < kGridSize; i++) {
        for (int j = 0; j < kGridSize; j++) {
            if (grid_[i][j] == CellType::Empty) {
                empty_cells.append(QPair(i, j));
            }
        }
    }

    if (empty_cells.isEmpty()) {
        return;
    }

    const int random_index = QRandomGenerator::global()->bounded(empty_cells.size());
    apple_ = empty_cells.at(random_index);

    grid_[apple_.first][apple_.second] = CellType::Apple;
}

bool SnakeGameLayer::IsCollision(const int x, const int y) const {
    if (x < 0 || x >= kGridSize || y < 0 || y >= kGridSize) {
        if (game_state_ == GameState::ExitOpen) {
            if ((x < 0 && exit_side_ == BorderSide::Left) ||
                (x >= kGridSize && exit_side_ == BorderSide::Right)) {
                return y != exit_position_;
            }
        }
        return true;
    }

    return grid_[x][y] == CellType::Snake || grid_[x][y] == CellType::SnakeHead;
}

void SnakeGameLayer::HandleInput(const int key) {
    // ReSharper disable once CppDFAUnusedValue
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
            return;
    }

    if ((new_direction == Direction::Up && last_processed_direction_ == Direction::Down) ||
        (new_direction == Direction::Down && last_processed_direction_ == Direction::Up) ||
        (new_direction == Direction::Left && last_processed_direction_ == Direction::Right) ||
        (new_direction == Direction::Right && last_processed_direction_ == Direction::Left)) {
        return;
    }

    direction_ = new_direction;
}

void SnakeGameLayer::keyPressEvent(QKeyEvent *event) {
    HandleInput(event->key());
    event->accept();
}

bool SnakeGameLayer::eventFilter(QObject *watched, QEvent *event) {
    if (watched == game_board_ && event->type() == QEvent::KeyPress) {
        const auto key_event = static_cast<QKeyEvent *>(event);
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
        game_state_ = GameState::Completed;

        game_board_->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33;");

        score_label_->setText(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.SnakeExited",
                                                     "Snake exited the screen! Level completed."));
        score_label_->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");

        QTimer::singleShot(1000, this, [this] {
            const auto effect = new QGraphicsOpacityEffect(this);
            this->setGraphicsEffect(effect);

            const auto animation = new QPropertyAnimation(effect, "opacity");
            animation->setDuration(500);
            animation->setStartValue(1.0);
            animation->setEndValue(0.0);
            animation->setEasingCurve(QEasingCurve::OutQuad);

            connect(animation, &QPropertyAnimation::finished, this, [this] {
                emit layerCompleted();
            });

            animation->start(QPropertyAnimation::DeleteWhenStopped);
        });
    } else {
        game_state_ = GameState::Failed;

        game_board_->setStyleSheet("background-color: rgba(40, 10, 10, 220);");

        QTimer::singleShot(500, this, [this] {
            Reset();
            InitializeGame();
            game_timer_->start();
        });
    }
}

void SnakeGameLayer::OpenBorder() {
    game_state_ = GameState::ExitOpen;

    exit_points_.clear();

    if (exit_side_ == BorderSide::Left) {
        exit_points_.append(QPair(-1, exit_position_));
    } else {
        exit_points_.append(QPair(kGridSize, exit_position_));
    }

    score_label_->setText(translator_->Translate("ClassifiedSettingsWidget.SnakeGame.ExitOpened",
                                                 "Exit opened!\nESCAPE!!!"));
    score_label_->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");

    animation_progress_ = 0;

    border_animation_timer_->start();

    RenderGame();
}

void SnakeGameLayer::UpdateBorderAnimation() {
    animation_progress_++;
    if (animation_progress_ % 3 == 0) {
        RenderGame();
    }
}

bool SnakeGameLayer::IsExitPoint(const int x, const int y) const {
    if (exit_side_ == BorderSide::Left && x < 0 && y == exit_position_) return true;
    if (exit_side_ == BorderSide::Right && x >= kGridSize && y == exit_position_) return true;
    return false;
}
