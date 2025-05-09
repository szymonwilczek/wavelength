#ifndef SNAKE_GAME_LAYER_H
#define SNAKE_GAME_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QKeyEvent>

class TranslationManager;
/**
 * @brief A security layer implementing a classic Snake game as a challenge.
 *
 * This layer presents a Snake game where the user controls a snake using arrow keys.
 * The objective is to collect 4 apples. After collecting the apples, an exit opens
 * on either the left or right border of the game board. The user must then guide
 * the entire snake out through the exit. Collision with the walls (except the exit)
 * or the snake's own body results in a game over and restart. Successful completion
 * triggers the layerCompleted() signal.
 */
class SnakeGameLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a SnakeGameLayer.
     * Initializes the UI elements (title, instructions, game board, score label),
     * sets up the layout, creates timers for game updates and border animation,
     * sets focus policy, and randomly determines the exit side and position.
     * @param parent Optional parent widget.
     */
    explicit SnakeGameLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops and deletes the game and border animation timers.
     */
    ~SnakeGameLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the game state, initializes the game logic (snake position, apple),
     * sets focus, ensures the layer is visible, and starts the game timer after a short delay.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Stops timers, resets game variables (score, game over state, snake, direction),
     * randomizes the exit again, resets UI elements (score label, board style), and ensures full opacity.
     */
    void Reset() override;

protected:
    /**
     * @brief Handles key press events for controlling the snake.
     * Passes the key code to HandleInput() and accepts the event.
     * @param event The key event.
     */
    void keyPressEvent(QKeyEvent* event) override;

    /**
     * @brief Filters key press events specifically for the game_board_ QLabel.
     * Ensures that arrow key presses are captured even if the main layer doesn't have focus,
     * as long as the game board does.
     * @param watched The object being watched.
     * @param event The event being processed.
     * @return True if the event was handled (key press on game board), false otherwise.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    /**
     * @brief Slot called periodically by game_timer_ to update the game state.
     * Calls MoveSnake() if the game is not over.
     */
    void UpdateGame();

    /**
     * @brief Handles the end of the game (success or failure).
     * Stops timers. On success, updates UI to green, shows a success message, and fades out the layer.
     * On failure, briefly shows a red background and restarts the game.
     * @param success True if the game was completed successfully, false otherwise.
     */
    void FinishGame(bool success);

    /**
     * @brief Initiates the "exit open" state after collecting 4 apples.
     * Changes the game state, determines exit points, updates the score label,
     * starts the border opening animation, and renders the game board with the exit.
     */
    void OpenBorder();

    /**
     * @brief Slot called periodically by border_animation_timer_ to animate the border opening.
     * Increments the animation progress and triggers a repaint of the game board periodically.
     */
    void UpdateBorderAnimation();

private:
    /**
     * @brief Enum representing the possible movement directions of the snake.
     */
    enum class Direction {
        Up,
        Right,
        Down,
        Left
    };

    /**
     * @brief Enum representing the possible types of content within a grid cell.
     */
    enum class CellType {
        Empty,      ///< Empty cell.
        Snake,      ///< Part of the snake's body.
        SnakeHead,  ///< The head of the snake.
        Apple,      ///< An apple to be collected.
        OpenBorder  ///< Represents the open exit area (not directly used in grid).
    };

    /**
     * @brief Enum representing the current state of the game.
     */
    enum class GameState {
        Playing,    ///< Normal gameplay, collecting apples.
        ExitOpen,   ///< Apples collected, exit is open, snake needs to leave.
        Completed,  ///< Snake successfully exited the board.
        Failed      ///< Snake collided, game over.
    };

    /**
     * @brief Enum representing which border (left or right) the exit will appear on.
     */
    enum class BorderSide {
        Left,
        Right
    };

    /**
     * @brief Sets up the initial state of the game board and snake.
     * Clears the grid, places the snake in the center, sets the initial direction,
     * generates the first apple, and renders the initial board state.
     */
    void InitializeGame();

    /**
     * @brief Renders the current state of the game onto the game_board_ QLabel.
     * Draws the grid, borders (including the animated open exit), snake, and apple.
     */
    void RenderGame() const;

    /**
     * @brief Moves the snake one step in the current direction_.
     * Calculates the new head position, checks for collisions (walls, self),
     * checks for apple collection, handles exiting the board in ExitOpen state,
     * updates the snake's position in the snake_ vector and the grid_ array,
     * and triggers a re-render.
     */
    void MoveSnake();

    /**
     * @brief Generates a new apple at a random empty cell on the grid.
     */
    void GenerateApple();

    /**
     * @brief Checks if the given coordinates (x, y) represent a collision.
     * Considers collisions with walls (respecting the open exit in ExitOpen state)
     * and collisions with the snake's own body.
     * @param x The x-coordinate to check.
     * @param y The y-coordinate to check.
     * @return True if a collision occurs at (x, y), false otherwise.
     */
    bool IsCollision(int x, int y) const;

    /**
     * @brief Processes user input (arrow keys) to change the snake's direction.
     * Prevents the snake from reversing direction directly (e.g., going left immediately after going right).
     * Updates the direction_ member variable.
     * @param key The Qt::Key code pressed by the user.
     */
    void HandleInput(int key);

    /**
     * @brief Checks if the given coordinates (x, y) correspond to the open exit location.
     * Used only when game_state_ is ExitOpen.
     * @param x The x-coordinate to check.
     * @param y The y-coordinate to check.
     * @return True if (x, y) is the exit point, false otherwise.
     */
    bool IsExitPoint(int x, int y) const;

    /**
     * @brief Static placeholder function. Logic moved to MoveSnake.
     */
    static void CheckForExitProgress();

    /** @brief QLabel used as the canvas for rendering the game board. */
    QLabel* game_board_;
    /** @brief QLabel displaying the current score (apples collected). */
    QLabel* score_label_;
    /** @brief Timer controlling the main game loop (snake movement). */
    QTimer* game_timer_;
    /** @brief Timer controlling the animation of the border opening. */
    QTimer* border_animation_timer_;

    /** @brief Size of the game grid (width and height in cells). */
    static constexpr int kGridSize = 12;
    /** @brief Size of each cell in pixels. */
    static constexpr int kCellSize = 20;

    /** @brief 2D array representing the game board state. */
    CellType grid_[kGridSize][kGridSize];
    /** @brief Vector storing the coordinates of each segment of the snake (head is first). */
    QVector<QPair<int, int>> snake_;
    /** @brief Coordinates of the current apple. */
    QPair<int, int> apple_;
    /** @brief Current direction the snake is moving. */
    Direction direction_;
    /** @brief Direction the snake was moving in the *last processed* game tick (used to prevent 180 turns). */
    Direction last_processed_direction_;
    /** @brief Number of apples collected so far. */
    int apples_collected_;
    /** @brief Flag indicating if the game is over (due to collision or successful exit). */
    bool game_over_;

    /** @brief Current state of the game (Playing, ExitOpen, etc.). */
    GameState game_state_;
    /** @brief Which side (Left or Right) the exit is on. */
    BorderSide exit_side_;
    /** @brief The Y-coordinate (row index) of the exit cell. */
    int exit_position_;
    /** @brief Progress counter for the border opening animation. */
    int animation_progress_;
    /** @brief Counter for how many snake segments have successfully exited the board. */
    int parts_exited_;
    /** @brief Stores the coordinates considered as valid exit points (just outside the border). */
    QVector<QPair<int, int>> exit_points_;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager* translator_;
};

#endif // SNAKE_GAME_LAYER_H