#ifndef SNAKE_GAME_LAYER_H
#define SNAKE_GAME_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QKeyEvent>

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
        Apple
    };

    void initializeGame();
    void renderGame();
    void moveSnake();
    void generateApple();
    bool isCollision(int x, int y);
    void handleInput(int key);

    QLabel* m_gameBoard;
    QLabel* m_scoreLabel;
    QTimer* m_gameTimer;

    static const int GRID_SIZE = 8;
    static const int CELL_SIZE = 30;

    QVector<QPair<int, int>> m_snake;
    QPair<int, int> m_apple;
    Direction m_direction;
    Direction m_lastProcessedDirection;
    int m_applesCollected;
    bool m_gameOver;

    CellType m_grid[GRID_SIZE][GRID_SIZE];
};

#endif // SNAKE_GAME_LAYER_H