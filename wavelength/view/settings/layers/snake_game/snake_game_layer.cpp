#include "snake_game_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QRandomGenerator>

SnakeGameLayer::SnakeGameLayer(QWidget *parent) 
    : SecurityLayer(parent),
      m_gameTimer(nullptr),
      m_borderAnimationTimer(nullptr),
      m_direction(Direction::Right),
      m_lastProcessedDirection(Direction::Right),
      m_applesCollected(0),
      m_gameOver(false),
      m_gameState(GameState::Playing),
      m_borderAnimationProgress(0),
      m_snakePartsExited(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(15);

    QLabel *title = new QLabel("SECURITY VERIFICATION: SNAKE GAME", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    QLabel *instructions = new QLabel("Zbierz 4 jabłka, aby otworzyć wyjście\nSterowanie: strzałki", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    // Plansza gry - usuwamy obramowanie z CSS
    m_gameBoard = new QLabel(this);
    m_gameBoard->setFixedSize(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);
    m_gameBoard->setStyleSheet("background-color: rgba(10, 25, 40, 220);"); // Bez bordera
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

    // Inicjalizacja timera gry - zmniejszamy interwał dla większej płynności
    m_gameTimer = new QTimer(this);
    m_gameTimer->setInterval(150); // 150ms dla płynności
    connect(m_gameTimer, &QTimer::timeout, this, &SnakeGameLayer::updateGame);

    // Timer animacji otwierania granicy
    m_borderAnimationTimer = new QTimer(this);
    m_borderAnimationTimer->setInterval(50); // 50ms dla płynnej animacji
    connect(m_borderAnimationTimer, &QTimer::timeout, this, &SnakeGameLayer::updateBorderAnimation);

    // Ustawienie focusu
    setFocusPolicy(Qt::StrongFocus);

    // Losowanie strony wyjścia już na początku
    int exitSideRandom = QRandomGenerator::global()->bounded(2);
    m_exitSide = static_cast<BorderSide>(exitSideRandom);

    // Losowa pozycja Y dla wyjścia (unikamy narożników)
    m_exitPosition = QRandomGenerator::global()->bounded(1, GRID_SIZE - 1);
}

SnakeGameLayer::~SnakeGameLayer() {
    if (m_gameTimer) {
        m_gameTimer->stop();
        delete m_gameTimer;
        m_gameTimer = nullptr;
    }

    if (m_borderAnimationTimer) {
        m_borderAnimationTimer->stop();
        delete m_borderAnimationTimer;
        m_borderAnimationTimer = nullptr;
    }
}

void SnakeGameLayer::initialize() {
    reset(); // Reset najpierw
    initializeGame(); // Inicjalizuj logikę gry
    setFocus(); // Ustaw focus na warstwie
    m_gameBoard->setFocus(); // Ustaw focus na planszy (ważne dla eventFilter)

    // Upewnij się, że widget jest widoczny przed startem
    if (graphicsEffect()) {
         static_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }

    // Rozpocznij grę po krótkim opóźnieniu
    QTimer::singleShot(200, this, [this]() {
        if (!m_gameOver) { // Sprawdź, czy gra nie została już zakończona (np. przez szybki reset)
            m_gameTimer->start();
        }
    });
}

void SnakeGameLayer::reset() {
    // Zatrzymaj timery
    if (m_gameTimer && m_gameTimer->isActive()) {
        m_gameTimer->stop();
    }
    if (m_borderAnimationTimer && m_borderAnimationTimer->isActive()) {
        m_borderAnimationTimer->stop();
    }

    // Resetuj stan gry
    m_applesCollected = 0;
    m_gameOver = false; // Kluczowe - resetuj stan game over
    m_gameState = GameState::Playing;
    m_borderAnimationProgress = 0;
    m_snakePartsExited = 0;
    m_exitPoints.clear();
    m_snake.clear(); // Wyczyść węża
    m_direction = Direction::Right; // Resetuj kierunek
    m_lastProcessedDirection = Direction::Right;

    // Ponowne losowanie strony wyjścia i pozycji
    int exitSideRandom = QRandomGenerator::global()->bounded(2);
    m_exitSide = static_cast<BorderSide>(exitSideRandom);
    m_exitPosition = QRandomGenerator::global()->bounded(1, GRID_SIZE - 1);

    // Resetuj UI
    m_scoreLabel->setText("Zebrane jabłka: 0/4");
    m_scoreLabel->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;"); // Szary tekst wyniku
    m_gameBoard->setStyleSheet("background-color: rgba(10, 25, 40, 220);"); // Domyślne tło planszy (bez bordera)
    m_gameBoard->clear();

    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect());
    if (effect) {
        effect->setOpacity(1.0);
    }
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
    // Zaczynamy od środka planszy
    int startX = GRID_SIZE / 2;
    int startY = GRID_SIZE / 2;

    m_snake.append(QPair<int, int>(startX, startY));     // Głowa
    m_snake.append(QPair<int, int>(startX-1, startY));   // Ciało
    m_snake.append(QPair<int, int>(startX-2, startY));   // Ogon

    m_grid[startX][startY] = CellType::SnakeHead;
    m_grid[startX-1][startY] = CellType::Snake;
    m_grid[startX-2][startY] = CellType::Snake;

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

    // Rysowanie obramowania planszy
    QPen borderPen(QColor(255, 51, 51), 2, Qt::SolidLine);
    painter.setPen(borderPen);

    // Rysowanie obramowania z uwzględnieniem otwartego wyjścia
    // Górna linia
    painter.drawLine(0, 0, GRID_SIZE * CELL_SIZE, 0);

    // Dolna linia
    painter.drawLine(0, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);

    // Jeśli jesteśmy w trybie wyjścia, narysuj specjalne obramowanie z otworem
    if (m_gameState == GameState::ExitOpen) {
        // Rysujemy lewe i prawe obramowanie w zależności od tego, gdzie jest wyjście

        if (m_exitSide == BorderSide::Left) {
            // Lewa strona ma otwór
            // Górna część lewego obramowania
            painter.drawLine(0, 0, 0, m_exitPosition * CELL_SIZE);
            // Dolna część lewego obramowania
            painter.drawLine(0, (m_exitPosition + 1) * CELL_SIZE, 0, GRID_SIZE * CELL_SIZE);

            // Otwarta część z animacją
            QColor openColor(50, 255, 50); // Zielony kolor dla otwartego przejścia
            painter.setPen(QPen(openColor, 2, Qt::DotLine));

            // Stopniowe otwieranie - w zależności od postępu animacji
            int openPosition = std::min(m_borderAnimationProgress / 2, CELL_SIZE);
            painter.drawLine(openPosition, m_exitPosition * CELL_SIZE, openPosition, (m_exitPosition + 1) * CELL_SIZE);

            // Prawa strona pełna
            painter.setPen(borderPen);
            painter.drawLine(GRID_SIZE * CELL_SIZE, 0, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);

            // Narysuj strzałkę wskazującą wyjście
            QColor arrowColor(50, 255, 50, 150 + (m_borderAnimationProgress % 2) * 50); // Pulsująca zielona strzałka
            painter.setPen(QPen(arrowColor, 2));
            painter.setBrush(QBrush(arrowColor));

            QPolygonF arrow;
            int arrowSize = 6;
            arrow << QPointF(5, m_exitPosition * CELL_SIZE + CELL_SIZE/2)
                  << QPointF(arrowSize * 1.5, m_exitPosition * CELL_SIZE + CELL_SIZE/2 - arrowSize)
                  << QPointF(arrowSize * 1.5, m_exitPosition * CELL_SIZE + CELL_SIZE/2 + arrowSize);

            painter.drawPolygon(arrow);
        }
        else { // m_exitSide == BorderSide::Right
            // Prawa strona ma otwór
            // Górna część prawego obramowania
            painter.drawLine(GRID_SIZE * CELL_SIZE, 0, GRID_SIZE * CELL_SIZE, m_exitPosition * CELL_SIZE);
            // Dolna część prawego obramowania
            painter.drawLine(GRID_SIZE * CELL_SIZE, (m_exitPosition + 1) * CELL_SIZE, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);

            // Otwarta część z animacją
            QColor openColor(50, 255, 50); // Zielony kolor dla otwartego przejścia
            painter.setPen(QPen(openColor, 2, Qt::DotLine));

            // Stopniowe otwieranie - w zależności od postępu animacji
            int openPosition = GRID_SIZE * CELL_SIZE - std::min(m_borderAnimationProgress / 2, CELL_SIZE);
            painter.drawLine(openPosition, m_exitPosition * CELL_SIZE, openPosition, (m_exitPosition + 1) * CELL_SIZE);

            // Lewa strona pełna
            painter.setPen(borderPen);
            painter.drawLine(0, 0, 0, GRID_SIZE * CELL_SIZE);

            // Narysuj strzałkę wskazującą wyjście
            QColor arrowColor(50, 255, 50, 150 + (m_borderAnimationProgress % 2) * 50); // Pulsująca zielona strzałka
            painter.setPen(QPen(arrowColor, 2));
            painter.setBrush(QBrush(arrowColor));

            QPolygonF arrow;
            int arrowSize = 6;
            arrow << QPointF(GRID_SIZE * CELL_SIZE - 5, m_exitPosition * CELL_SIZE + CELL_SIZE/2)
                  << QPointF(GRID_SIZE * CELL_SIZE - arrowSize * 1.5, m_exitPosition * CELL_SIZE + CELL_SIZE/2 - arrowSize)
                  << QPointF(GRID_SIZE * CELL_SIZE - arrowSize * 1.5, m_exitPosition * CELL_SIZE + CELL_SIZE/2 + arrowSize);

            painter.drawPolygon(arrow);
        }
    } else {
        // Standardowe obramowanie bez wyjścia
        painter.drawLine(0, 0, 0, GRID_SIZE * CELL_SIZE);  // Lewa
        painter.drawLine(GRID_SIZE * CELL_SIZE, 0, GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);  // Prawa
    }

    // Rysowanie elementów gry
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            QRect cellRect(i * CELL_SIZE, j * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            QRect stemRect(cellRect.center().x() - 1, cellRect.top() + 2, 2, 4);
            cellRect.adjust(2, 2, -2, -2); // Margines

            // Tylko komórki w granicach planszy
            if (i >= 0 && i < GRID_SIZE && j >= 0 && j < GRID_SIZE) {
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

    // W trybie wyjścia sprawdzamy, czy wąż wychodzi przez otwarte wyjście
    if (m_gameState == GameState::ExitOpen && isExitPoint(newHeadX, newHeadY)) {
        // Rejestrujemy, że segment węża opuścił planszę
        m_snakePartsExited++;

        // Sprawdzamy, czy cały wąż opuścił planszę
        if (m_snakePartsExited >= m_snake.size()) {
            // Zamiast finishGame(true), najpierw zabezpieczamy stan gry
            m_gameOver = true;
            QTimer::singleShot(10, this, [this]() {
                finishGame(true);
            });
            return;
        }

        // Bezpieczne usunięcie typu komórki dla starej głowy (tylko jeśli jest w granicach planszy)
        if (headX >= 0 && headX < GRID_SIZE && headY >= 0 && headY < GRID_SIZE) {
            m_grid[headX][headY] = CellType::Snake;
        }

        // W przypadku wychodzenia poza planszę, usuwamy ostatni segment bez względu na jabłko
        QPair<int, int> tail = m_snake.last();
        // Bezpieczne usunięcie ostatniego segmentu (tylko jeśli jest w granicach planszy)
        if (tail.first >= 0 && tail.first < GRID_SIZE && tail.second >= 0 && tail.second < GRID_SIZE) {
            m_grid[tail.first][tail.second] = CellType::Empty;
        }
        m_snake.removeLast();

        // Dodajemy nową głowę poza planszą
        m_snake.prepend(QPair<int, int>(newHeadX, newHeadY));

        // Aktualizacja widoku
        renderGame();
        return;
    }

    // Sprawdzenie kolizji w normalnej grze
    if (isCollision(newHeadX, newHeadY)) {
        // W trybie wyjścia, jeśli jest to kolizja ze ścianą, sprawdzamy czy to punkt wyjścia
        if (m_gameState == GameState::ExitOpen &&
           ((newHeadX < 0 && m_exitSide == BorderSide::Left) ||
            (newHeadX >= GRID_SIZE && m_exitSide == BorderSide::Right))) {

            // Sprawdzamy, czy kolizja jest na wysokości wyjścia
            if (newHeadY == m_exitPosition) {
                // To jest punkt wyjścia - dodajemy segment i kontynuujemy
                m_snake.prepend(QPair<int, int>(newHeadX, newHeadY));
                m_snakePartsExited++;

                // Usunięcie ogona
                QPair<int, int> tail = m_snake.last();
                m_grid[tail.first][tail.second] = CellType::Empty;
                m_snake.removeLast();

                // Sprawdzamy, czy połowa węża opuściła planszę
                if (m_snakePartsExited >= m_snake.size()) {
                    finishGame(true);
                }
                return;
            }
        }

        // Standardowa kolizja
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

    // Jeśli nowa głowa jest w granicach planszy, aktualizujemy grid
    if (newHeadX >= 0 && newHeadX < GRID_SIZE && newHeadY >= 0 && newHeadY < GRID_SIZE) {
        m_grid[newHeadX][newHeadY] = CellType::SnakeHead;
    }

    // Jeśli nie zebrano jabłka, usunięcie ostatniego segmentu
    if (!appleCollected) {
        QPair<int, int> tail = m_snake.last();
        m_snake.removeLast();

        // Jeśli ogon jest w granicach planszy, aktualizujemy grid
        if (tail.first >= 0 && tail.first < GRID_SIZE && tail.second >= 0 && tail.second < GRID_SIZE) {
            m_grid[tail.first][tail.second] = CellType::Empty;
        }
    } else {
        // Jeśli zebrano jabłko
        m_applesCollected++;
        m_scoreLabel->setText(QString("Zebrane jabłka: %1/4").arg(m_applesCollected));

        // Generowanie nowego jabłka (jeśli nie zebrano jeszcze 4)
        if (m_applesCollected < 4) {
            generateApple();
        } else {
            // Po zebraniu 4 jabłka - otwórz wyjście
            openBorder();
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
    // Kolizja ze ścianą - w trybie wyjścia sprawdzamy czy to punkt wyjścia
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        if (m_gameState == GameState::ExitOpen) {
            // Jeśli to wyjście po lewej i x < 0 lub wyjście po prawej i x >= GRID_SIZE
            if ((x < 0 && m_exitSide == BorderSide::Left) ||
                (x >= GRID_SIZE && m_exitSide == BorderSide::Right)) {
                // Sprawdzamy, czy y jest na wysokości wyjścia
                return y != m_exitPosition;
            }
        }
        return true; // Kolizja ze ścianą w innych przypadkach
    }

    // Kolizja z wężem
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
    m_borderAnimationTimer->stop();

    if (success) {
        // Ustawienie stanu gry na zakończony sukcesem
        m_gameState = GameState::Completed;

        // Zakończenie powodzeniem - zielone obramowanie
        m_gameBoard->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33;");

        // Aktualizacja etykiety wyniku
        m_scoreLabel->setText("Wąż opuścił planszę! Poziom zaliczony.");
        m_scoreLabel->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");

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
        // Ustawienie stanu gry na zakończony niepowodzeniem
        m_gameState = GameState::Failed;

        // Niepowodzenie - czerwone tło i restart gry
        m_gameBoard->setStyleSheet("background-color: rgba(40, 10, 10, 220);");

        // Krótkie opóźnienie przed restartem
        QTimer::singleShot(500, this, [this]() {
            reset();
            initializeGame();
            m_gameTimer->start();
        });
    }
}

void SnakeGameLayer::openBorder() {
    // Zmiana stanu gry
    m_gameState = GameState::ExitOpen;

    // Czyszczenie poprzednich punktów wyjścia
    m_exitPoints.clear();

    // Określenie punktu wyjścia w zależności od wybranej strony
    if (m_exitSide == BorderSide::Left) {
        // Lewe wyjście
        m_exitPoints.append(QPair<int, int>(-1, m_exitPosition));
    } else {
        // Prawe wyjście
        m_exitPoints.append(QPair<int, int>(GRID_SIZE, m_exitPosition));
    }

    // Aktualizacja etykiety wyniku
    m_scoreLabel->setText("Wyjście otwarte! Wyprowadź węża z planszy.");
    m_scoreLabel->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 10pt;");

    // Inicjalizacja licznika animacji
    m_borderAnimationProgress = 0;

    // Start timera animacji
    m_borderAnimationTimer->start();

    // Renderowanie planszy z otwartym wyjściem
    renderGame();
}

void SnakeGameLayer::updateBorderAnimation() {
    // Inkrementacja licznika animacji
    m_borderAnimationProgress++;

    // Odświeżenie planszy co określoną liczbę kroków
    if (m_borderAnimationProgress % 3 == 0) {
        renderGame();
    }
}

bool SnakeGameLayer::isExitPoint(int x, int y) {
    // Sprawdzenie, czy punkt (x, y) jest punktem wyjścia
    if (m_exitSide == BorderSide::Left && x < 0 && y == m_exitPosition) {
        return true;
    }

    if (m_exitSide == BorderSide::Right && x >= GRID_SIZE && y == m_exitPosition) {
        return true;
    }

    return false;
}

void SnakeGameLayer::checkForExitProgress() {
    // Ten kod jest teraz obsługiwany w moveSnake()
}