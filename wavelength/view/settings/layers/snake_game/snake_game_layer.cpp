#include "snake_game_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QRandomGenerator>

SnakeGameLayer::SnakeGameLayer(QWidget *parent) 
    : SecurityLayer(parent),
      m_gameTimer(nullptr),
      m_direction(Direction::Right),
      m_lastProcessedDirection(Direction::Right),
      m_applesCollected(0),
      m_gameOver(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(15);

    QLabel *title = new QLabel("SECURITY VERIFICATION: SNAKE GAME", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    QLabel *instructions = new QLabel("Zbierz 4 jabłka, aby kontynuować\nSterowanie: strzałki", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    // Plansza gry
    m_gameBoard = new QLabel(this);
    m_gameBoard->setFixedSize(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);
    m_gameBoard->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #ff3333; border-radius: 3px;");
    m_gameBoard->setAlignment(Qt::AlignCenter);
    m_gameBoard->installEventFilter(this);
    
    // Etykieta wyniku
    m_scoreLabel = new QLabel("Zebrane jabłka: 0/4", this);
    m_scoreLabel->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    m_scoreLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(title);
    layout->addWidget(instructions);
    layout->addWidget(m_gameBoard, 0, Qt::AlignCenter);
    layout->addWidget(m_scoreLabel);
    layout->addStretch();

    // Inicjalizacja timera gry
    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(250); // 250ms na ruch
    connect(m_gameTimer, &QTimer::timeout, this, &SnakeGameLayer::updateGame);
    
    // Ustawienie focusu
    setFocusPolicy(Qt::StrongFocus);
}

SnakeGameLayer::~SnakeGameLayer() {
    if (m_gameTimer) {
        m_gameTimer->stop();
        delete m_gameTimer;
        m_gameTimer = nullptr;
    }
}

void SnakeGameLayer::initialize() {
    reset();
    initializeGame();
    setFocus();
    m_gameBoard->setFocus();
    m_gameTimer->start();
}

void SnakeGameLayer::reset() {
    if (m_gameTimer && m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }
    
    m_applesCollected = 0;
    m_gameOver = false;
    m_scoreLabel->setText("Zebrane jabłka: 0/4");
    
    // Reset stylów
    m_gameBoard->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #ff3333; border-radius: 3px;");
}

void SnakeGameLayer::initializeGame() {
    // Wyczyszczenie planszy
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            m_grid[i][j] = CellType::Empty;
        }
    }
    
    // Inicjalizacja węża
    m_snake.clear();
    m_snake.append(QPair<int, int>(3, 4)); // Głowa
    m_snake.append(QPair<int, int>(2, 4)); // Ciało
    m_snake.append(QPair<int, int>(1, 4)); // Ogon
    
    m_grid[3][4] = CellType::SnakeHead;
    m_grid[2][4] = CellType::Snake;
    m_grid[1][4] = CellType::Snake;
    
    // Początkowo wąż porusza się w prawo
    m_direction = Direction::Right;
    m_lastProcessedDirection = Direction::Right;
    
    // Generowanie jabłka
    generateApple();
    
    // Renderowanie planszy
    renderGame();
}

void SnakeGameLayer::renderGame() {
    // Rysowanie planszy
    QImage board(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, QImage::Format_ARGB32);
    board.fill(QColor(10, 25, 40, 220));
    
    QPainter painter(&board);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Rysowanie siatki
    painter.setPen(QPen(QColor(50, 50, 50, 100), 1, Qt::SolidLine));
    for (int i = 1; i < GRID_SIZE; i++) {
        // Linie poziome
        painter.drawLine(0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE, i * CELL_SIZE);
        // Linie pionowe
        painter.drawLine(i * CELL_SIZE, 0, i * CELL_SIZE, GRID_SIZE * CELL_SIZE);
    }
    
    // Rysowanie elementów gry
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            QRect cellRect(i * CELL_SIZE, j * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            QRect stemRect(cellRect.center().x() - 1, cellRect.top() + 2, 2, 4);
            cellRect.adjust(2, 2, -2, -2); // Margines
            
            switch (m_grid[i][j]) {
                case CellType::SnakeHead:
                    // Głowa węża - jaśniejszy zielony
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(50, 240, 50));
                    painter.drawRoundedRect(cellRect, 5, 5);
                    break;
                    
                case CellType::Snake:
                    // Ciało węża - zielony
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(30, 200, 30));
                    painter.drawRoundedRect(cellRect, 5, 5);
                    break;
                    
                case CellType::Apple:
                    // Jabłko - czerwone
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(220, 40, 40));
                    painter.drawEllipse(cellRect);
                    
                    // Szypułka jabłka
                    painter.setBrush(QColor(100, 70, 20));
                    painter.drawRect(stemRect);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    painter.end();
    
    m_gameBoard->setPixmap(QPixmap::fromImage(board));
}

void SnakeGameLayer::moveSnake() {
    if (m_gameOver) {
        return;
    }
    
    // Zapamiętanie ostatniego przetwarzanego kierunku
    m_lastProcessedDirection = m_direction;
    
    // Pobranie pozycji głowy węża
    int headX = m_snake.first().first;
    int headY = m_snake.first().second;
    
    // Obliczenie nowej pozycji głowy
    int newHeadX = headX;
    int newHeadY = headY;
    
    switch (m_direction) {
        case Direction::Up:
            newHeadY--;
            break;
        case Direction::Right:
            newHeadX++;
            break;
        case Direction::Down:
            newHeadY++;
            break;
        case Direction::Left:
            newHeadX--;
            break;
    }
    
    // Sprawdzenie kolizji
    if (isCollision(newHeadX, newHeadY)) {
        m_gameOver = true;
        finishGame(false);
        return;
    }
    
    // Sprawdzenie, czy wąż zebrał jabłko
    bool appleCollected = (newHeadX == m_apple.first && newHeadY == m_apple.second);
    
    // Aktualizacja pozycji węża
    // Usunięcie typu komórki dla starej głowy
    m_grid[headX][headY] = CellType::Snake;
    
    // Dodanie nowej głowy
    m_snake.prepend(QPair<int, int>(newHeadX, newHeadY));
    m_grid[newHeadX][newHeadY] = CellType::SnakeHead;
    
    // Jeśli nie zebrano jabłka, usunięcie ostatniego segmentu
    if (!appleCollected) {
        QPair<int, int> tail = m_snake.last();
        m_grid[tail.first][tail.second] = CellType::Empty;
        m_snake.removeLast();
    } else {
        // Jeśli zebrano jabłko
        m_applesCollected++;
        m_scoreLabel->setText(QString("Zebrane jabłka: %1/4").arg(m_applesCollected));
        
        // Generowanie nowego jabłka
        generateApple();
        
        // Sprawdzenie, czy zebrano 4 jabłka (koniec gry)
        if (m_applesCollected >= 4) {
            finishGame(true);
            return;
        }
    }
    
    // Aktualizacja widoku
    renderGame();
}

void SnakeGameLayer::generateApple() {
    // Znajdowanie wolnych komórek
    QVector<QPair<int, int>> emptyCells;
    
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (m_grid[i][j] == CellType::Empty) {
                emptyCells.append(QPair<int, int>(i, j));
            }
        }
    }
    
    if (emptyCells.isEmpty()) {
        return; // Brak wolnych komórek
    }
    
    // Wybór losowej pustej komórki
    int randomIndex = QRandomGenerator::global()->bounded(emptyCells.size());
    m_apple = emptyCells.at(randomIndex);
    
    // Ustawienie jabłka na planszy
    m_grid[m_apple.first][m_apple.second] = CellType::Apple;
}

bool SnakeGameLayer::isCollision(int x, int y) {
    // Kolizja ze ścianą
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        return true;
    }
    
    // Kolizja z wężem (z wyjątkiem jabłka)
    return m_grid[x][y] == CellType::Snake || m_grid[x][y] == CellType::SnakeHead;
}

void SnakeGameLayer::handleInput(int key) {
    Direction newDirection = m_direction;
    
    switch (key) {
        case Qt::Key_Up:
            newDirection = Direction::Up;
            break;
        case Qt::Key_Right:
            newDirection = Direction::Right;
            break;
        case Qt::Key_Down:
            newDirection = Direction::Down;
            break;
        case Qt::Key_Left:
            newDirection = Direction::Left;
            break;
        default:
            return; // Ignorowanie innych klawiszy
    }
    
    // Zapobieganie zawracaniu o 180 stopni
    if ((newDirection == Direction::Up && m_lastProcessedDirection == Direction::Down) ||
        (newDirection == Direction::Down && m_lastProcessedDirection == Direction::Up) ||
        (newDirection == Direction::Left && m_lastProcessedDirection == Direction::Right) ||
        (newDirection == Direction::Right && m_lastProcessedDirection == Direction::Left)) {
        return;
    }
    
    m_direction = newDirection;
}

void SnakeGameLayer::keyPressEvent(QKeyEvent* event) {
    handleInput(event->key());
    event->accept();
}

bool SnakeGameLayer::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_gameBoard && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        handleInput(keyEvent->key());
        return true;
    }
    return SecurityLayer::eventFilter(watched, event);
}

void SnakeGameLayer::updateGame() {
    if (!m_gameOver) {
        moveSnake();
    }
}

void SnakeGameLayer::finishGame(bool success) {
    m_gameTimer->stop();
    
    if (success) {
        // Zakończenie powodzeniem
        m_gameBoard->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 3px;");
        
        // Krótkie opóźnienie przed zakończeniem warstwy
        QTimer::singleShot(1000, this, [this]() {
            QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
            this->setGraphicsEffect(effect);

            QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
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
        // Niepowodzenie - restart gry
        m_gameBoard->setStyleSheet("background-color: rgba(40, 10, 10, 220); border: 2px solid #ff3333; border-radius: 3px;");
        
        // Krótkie opóźnienie przed restartem
        QTimer::singleShot(500, this, [this]() {
            reset();
            initializeGame();
            m_gameTimer->start();
        });
    }
}