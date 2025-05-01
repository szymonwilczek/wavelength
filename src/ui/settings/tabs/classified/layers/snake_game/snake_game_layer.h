#ifndef SNAKE_GAME_LAYER_H
#define SNAKE_GAME_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QKeyEvent>

class SnakeGameLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit SnakeGameLayer(QWidget *parent = nullptr);
    ~SnakeGameLayer() override;

    void Initialize() override;
    void Reset() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void UpdateGame();
    void FinishGame(bool success);
    void OpenBorder();
    void UpdateBorderAnimation();

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

    void InitializeGame();
    void RenderGame() const;
    void MoveSnake();
    void GenerateApple();
    bool IsCollision(int x, int y) const;
    void HandleInput(int key);
    bool IsExitPoint(int x, int y) const;

    static void CheckForExitProgress();

    QLabel* game_board_;
    QLabel* score_label_;
    QTimer* game_timer_;
    QTimer* border_animation_timer_;

    static constexpr int kGridSize = 12; // Zwiększony rozmiar siatki
    static constexpr int kCellSize = 20; // Zmniejszony rozmiar komórki, by całość zmieściła się na ekranie

    CellType grid_[kGridSize][kGridSize];
    QVector<QPair<int, int>> snake_;
    QPair<int, int> apple_;
    Direction direction_;
    Direction last_processed_direction_;
    int apples_collected_;
    bool game_over_;

    // Zmienne dla trybu wyjścia
    GameState game_state_;
    BorderSide exit_side_;     // Lewa lub prawa strona
    int exit_position_;        // Współrzędna Y wybranego kwadratu
    int animation_progress_;
    int parts_exited_;

    QVector<QPair<int, int>> exit_points_; // Punkty wyjściowe
};

#endif // SNAKE_GAME_LAYER_H