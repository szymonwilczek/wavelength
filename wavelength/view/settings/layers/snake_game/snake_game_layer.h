#ifndef SNAKE_GAME_LAYER_H
#define SNAKE_GAME_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QKeyEvent>
#include <QPropertyAnimation>

class SnakeGameLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit SnakeGameLayer(QWidget *parent = nullptr);
    ~SnakeGameLayer() override;

    void initialize() override;
    void reset() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void updateGame();
    void finishGame(bool success);
    void openBorder();
    void updateBorderAnimation();

private:
    enum class Direction {
        Up,
        Right,
        Down,
        Left
    };

    enum class CellType {
        Empty,
        Snake,
        SnakeHead,
        Apple,
        OpenBorder
    };

    enum class GameState {
        Playing,
        ExitOpen,
        Completed,
        Failed
    };

    enum class BorderSide {
        Left,
        Right
    };

    void initializeGame();
    void renderGame();
    void moveSnake();
    void generateApple();
    bool isCollision(int x, int y);
    void handleInput(int key);
    bool isExitPoint(int x, int y);
    void checkForExitProgress();

    QLabel* m_gameBoard;
    QLabel* m_scoreLabel;
    QTimer* m_gameTimer;
    QTimer* m_borderAnimationTimer;

    static const int GRID_SIZE = 12; // Zwiększony rozmiar siatki
    static const int CELL_SIZE = 20; // Zmniejszony rozmiar komórki, by całość zmieściła się na ekranie

    CellType m_grid[GRID_SIZE][GRID_SIZE];
    QVector<QPair<int, int>> m_snake;
    QPair<int, int> m_apple;
    Direction m_direction;
    Direction m_lastProcessedDirection;
    int m_applesCollected;
    bool m_gameOver;

    // Zmienne dla trybu wyjścia
    GameState m_gameState;
    BorderSide m_exitSide;     // Lewa lub prawa strona
    int m_exitPosition;        // Współrzędna Y wybranego kwadratu
    int m_borderAnimationProgress;
    int m_snakePartsExited;

    QVector<QPair<int, int>> m_exitPoints; // Punkty wyjściowe
};

#endif // SNAKE_GAME_LAYER_H