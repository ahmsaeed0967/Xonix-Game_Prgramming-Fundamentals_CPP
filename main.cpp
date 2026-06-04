#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <time.h>
#include <sstream>
#include <fstream>
#include <cmath>
using namespace sf;

// Game grid dimensions: M rows, N columns
const int M = 25;
const int N = 40;

// Grid to store game state: 0 (empty), 1 (wall), 2 (P1 trail), 3 (P2 trail), -1 (temp for area capture)
int grid[M][N] = {0};
// Tile size in pixels
int ts = 18;
// Tracks total game time
float gameTime = 0;
// Number of active enemies
int enemyCount;
// Continuous mode flag for endless enemy spawning
bool continuousMode = false;

// Game state: 0 (menu), 1 (playing), 2 (game over), 3 (scoreboard)
int gameState = 0;
// Selected difficulty level: 0 (easy), 1 (medium), 2 (hard), 3 (continuous), 4 (scoreboard), 5 (exit)
int selectedLevel = 0;
// Two-player mode flag
bool twoPlayerMode = false;

// Scoreboard arrays for top 5 scores and times
int scores[5] = {0};
float scoreTimes[5] = {0};
int scoreCount = 0;

// Player 1 variables: position, movement direction, moves count, score, bonuses, power-ups
int p1_x = 10, p1_y = 0, p1_dx = 0, p1_dy = 0, p1_moves = 0, p1_score = 0;
int p1_bonusCount = 0, p1_bonusThreshold = 10, p1_powerUps = 0;
bool p1_moved = false;
// Player 2 variables (used in two-player mode)
int p2_x = N - 11, p2_y = M - 1, p2_dx = 0, p2_dy = 0, p2_moves = 0, p2_score = 0;
int p2_bonusCount = 0, p2_bonusThreshold = 10, p2_powerUps = 0;
bool p2_moved = false;

// Enemy arrays: position, direction, pattern (0: linear, 1: zigzag, 2: circular), angle, speed
int enemy_x[10], enemy_y[10], enemy_dx[10], enemy_dy[10], enemy_pattern[10];
float enemy_angle[10], enemy_speed[10];

// Initialize enemy at a random empty grid position with random movement direction
void initEnemy(int i) {
    // Keep trying random positions until an empty cell (grid[y][x] == 0) is found
    while (true) {
        int x = rand() % (N - 2);
        int y = rand() % (M - 2);
        if (grid[y][x] == 0) {
            // Set enemy position in pixels (center of tile)
            enemy_x[i] = x * ts + ts;
            enemy_y[i] = y * ts + ts;
            break;
        }
    }
    // Assign random initial direction (±4 pixels per move)
    enemy_dx[i] = (rand() % 2 == 0 ? 1 : -1) * 4;
    enemy_dy[i] = (rand() % 2 == 0 ? 1 : -1) * 4;
    // Default to linear movement pattern
    enemy_pattern[i] = 0;
    // Initialize angle for circular movement
    enemy_angle[i] = 0;
    // Set base speed
    enemy_speed[i] = 4;
}

// Linear enemy movement: move in straight lines, bounce off walls or obstacles
void moveLinear(int i) {
    // Calculate next position based on direction and speed
    float next_x = enemy_x[i] + enemy_dx[i] * enemy_speed[i] / 4;
    float next_y = enemy_y[i] + enemy_dy[i] * enemy_speed[i] / 4;
    bool collisionX = false, collisionY = false;
    // Check for boundary collisions (left/right edges)
    if (next_x < ts || next_x >= (N - 1) * ts) {
        enemy_dx[i] = -enemy_dx[i]; // Reverse x-direction
        collisionX = true;
    }
    // Check for boundary collisions (top/bottom edges)
    if (next_y < ts || next_y >= (M - 1) * ts) {
        enemy_dy[i] = -enemy_dy[i]; // Reverse y-direction
        collisionY = true;
    }
    // Recalculate position after boundary collision
    if (collisionX) next_x = enemy_x[i] + enemy_dx[i] * enemy_speed[i] / 4;
    if (collisionY) next_y = enemy_y[i] + enemy_dy[i] * enemy_speed[i] / 4;
    // Check for collision with walls (grid value 1)
    if (grid[int(next_y) / ts][int(next_x) / ts] == 1) {
        // Check x-movement collision
        float temp_x = enemy_x[i] + enemy_dx[i] * enemy_speed[i] / 4;
        float temp_y = enemy_y[i];
        if (grid[int(temp_y) / ts][int(temp_x) / ts] == 1) enemy_dx[i] = -enemy_dx[i];
        // Check y-movement collision
        temp_x = enemy_x[i];
        temp_y = enemy_y[i] + enemy_dy[i] * enemy_speed[i] / 4;
        if (grid[int(temp_y) / ts][int(temp_x) / ts] == 1) enemy_dy[i] = -enemy_dy[i];
        // Handle corner case: reverse both directions if stuck
        if (!collisionX && !collisionY && enemy_dx[i] == (enemy_x[i] - next_x < 0 ? 1 : -1) * 4 && enemy_dy[i] == (enemy_y[i] - next_y < 0 ? 1 : -1) * 4) {
            enemy_dx[i] = -enemy_dx[i];
            enemy_dy[i] = -enemy_dy[i];
        }
        // Recalculate position after wall collision
        next_x = enemy_x[i] + enemy_dx[i] * enemy_speed[i] / 4;
        next_y = enemy_y[i] + enemy_dy[i] * enemy_speed[i] / 4;
    }
    // Update position if within bounds and no wall collision
    if (next_x >= ts && next_x < (N - 1) * ts && next_y >= ts && next_y < (M - 1) * ts && grid[int(next_y) / ts][int(next_x) / ts] != 1) {
        enemy_x[i] = next_x;
        enemy_y[i] = next_y;
    }
}

// Zigzag enemy movement: move horizontally with alternating vertical steps
void moveZigzag(int i) {
    // Static array to track zigzag step for each enemy
    static int zigzagStep[10] = {0};
    // Ensure horizontal direction is set
    if (enemy_dx[i] == 0) enemy_dx[i] = (rand() % 2 == 0 ? 1 : -1) * 4;
    // Calculate next position: horizontal move + alternating vertical step
    float next_x = enemy_x[i] + enemy_dx[i] * enemy_speed[i] / 4;
    float next_y = enemy_y[i] + (zigzagStep[i] % 2 == 0 ? 1 : -1) * enemy_speed[i] / 2;
    // Check horizontal collision (boundaries or walls)
    if (next_x < ts || next_x >= (N - 1) * ts || grid[int(enemy_y[i]) / ts][int(next_x) / ts] == 1) {
        enemy_dx[i] = -enemy_dx[i]; // Reverse horizontal direction
        next_x = enemy_x[i] + enemy_dx[i] * enemy_speed[i] / 4;
    }
    // Update x-position if valid
    if (next_x >= ts && next_x < (N - 1) * ts && grid[int(enemy_y[i]) / ts][int(next_x) / ts] != 1) enemy_x[i] = next_x;
    // Check vertical collision (boundaries or walls)
    if (next_y < ts || next_y >= (M - 1) * ts || grid[int(next_y) / ts][int(enemy_x[i]) / ts] == 1) {
        zigzagStep[i]++; // Increment step to alternate direction
        next_y = enemy_y[i] + (zigzagStep[i] % 2 == 0 ? 1 : -1) * enemy_speed[i] / 2;
    }
    // Update y-position if valid
    if (next_y >= ts && next_y < (M - 1) * ts && grid[int(next_y) / ts][int(enemy_x[i]) / ts] != 1) enemy_y[i] = next_y;
    else zigzagStep[i]++; // Increment step if vertical move is blocked
}

// Circular enemy movement: move in a circular path, adjust on collisions
void moveCircular(int i) {
    // Increment angle for circular motion
    static float angle_increment = 0.1f;
    // Initialize angle based on current direction
    if (enemy_angle[i] == 0 && enemy_dx[i] != 0) enemy_angle[i] = atan2(enemy_dy[i], enemy_dx[i]);
    float original_angle = enemy_angle[i];
    enemy_angle[i] += angle_increment;
    // Calculate next position using circular motion
    float next_x = enemy_x[i] + cos(enemy_angle[i]) * enemy_speed[i] * 0.5f;
    float next_y = enemy_y[i] + sin(enemy_angle[i]) * enemy_speed[i] * 0.5f;
    bool collision = false;
    // Check for boundary or wall collision
    if (next_x < ts || next_x >= (N - 1) * ts || next_y < ts || next_y >= (M - 1) * ts || grid[int(next_y) / ts][int(next_x) / ts] == 1) {
        collision = true;
        // Try opposite direction
        enemy_angle[i] = original_angle + 3.14159f;
        next_x = enemy_x[i] + cos(enemy_angle[i]) * enemy_speed[i] * 0.5f;
        next_y = enemy_y[i] + sin(enemy_angle[i]) * enemy_speed[i] * 0.5f;
        // Check if opposite direction is valid
        if (next_x < ts || next_x >= (N - 1) * ts || next_y < ts || next_y >= (M - 1) * ts || grid[int(next_y) / ts][int(next_x) / ts] == 1) {
            // Try four perpendicular directions
            for (int attempt = 1; attempt <= 4; attempt++) {
                enemy_angle[i] = original_angle + 3.14159f/2 * attempt;
                next_x = enemy_x[i] + cos(enemy_angle[i]) * enemy_speed[i] * 0.5f;
                next_y = enemy_y[i] + sin(enemy_angle[i]) * enemy_speed[i] * 0.5f;
                if (next_x >= ts && next_x < (N - 1) * ts && next_y >= ts && next_y < (M - 1) * ts && grid[int(next_y) / ts][int(next_x) / ts] != 1) {
                    collision = false;
                    break;
                }
            }
            // If still colliding, choose random angle
            if (collision) {
                enemy_angle[i] = (rand() % 628) / 100.0f;
                next_x = enemy_x[i] + cos(enemy_angle[i]) * enemy_speed[i] * 0.5f;
                next_y = enemy_y[i] + sin(enemy_angle[i]) * enemy_speed[i] * 0.5f;
                if (next_x >= ts && next_x < (N - 1) * ts && next_y >= ts && next_y < (M - 1) * ts && grid[int(next_y) / ts][int(next_x) / ts] != 1) collision = false;
            }
        } else collision = false;
    }
    // Update position if no collision
    if (!collision && next_x >= ts && next_x < (N - 1) * ts && next_y >= ts && next_y < (M - 1) * ts && grid[int(next_y) / ts][int(next_x) / ts] != 1) {
        enemy_x[i] = next_x;
        enemy_y[i] = next_y;
    }
}

// Update enemy movement pattern and initialize direction
void updateEnemyPattern(int i, int newPattern) {
    int oldPattern = enemy_pattern[i];
    enemy_pattern[i] = newPattern;
    // Initialize direction based on new pattern
    if (oldPattern != newPattern) {
        if (newPattern == 1) { // Zigzag
            if (enemy_dx[i] == 0) enemy_dx[i] = (rand() % 2 == 0 ? 1 : -1) * 4;
        } else if (newPattern == 2) { // Circular
            if (enemy_dx[i] != 0 || enemy_dy[i] != 0) enemy_angle[i] = atan2(enemy_dy[i], enemy_dx[i]);
            else enemy_angle[i] = (rand() % 628) / 100.0f;
        } else { // Linear
            if (enemy_dx[i] == 0 && enemy_dy[i] == 0) {
                enemy_dx[i] = (rand() % 2 == 0 ? 1 : -1) * 4;
                enemy_dy[i] = (rand() % 2 == 0 ? 1 : -1) * 4;
            }
        }
    }
}

// Move enemy based on its pattern
void moveEnemy(int i) {
    if (enemy_pattern[i] == 0) moveLinear(i);
    else if (enemy_pattern[i] == 1) moveZigzag(i);
    else moveCircular(i);
}

// Recursively mark empty areas for capture starting from (y, x)
void drop(int y, int x) {
    // Check bounds
    if (y < 0 || y >= M || x < 0 || x >= N) return;
    // Mark empty cell as temporary (-1)
    if (grid[y][x] == 0) grid[y][x] = -1;
    // Recurse in all four directions
    if (y > 0 && grid[y - 1][x] == 0) drop(y - 1, x);
    if (y < M - 1 && grid[y + 1][x] == 0) drop(y + 1, x);
    if (x > 0 && grid[y][x - 1] == 0) drop(y, x - 1);
    if (x < N - 1 && grid[y][x + 1] == 0) drop(y, x + 1);
}

// Update scoreboard with new score and time, maintaining top 5
void updateScoreboard(int score, float time) {
    // Add new score if fewer than 5 scores
    if (scoreCount < 5) {
        scores[scoreCount] = score;
        scoreTimes[scoreCount] = time;
        scoreCount++;
    } else if (score > scores[4]) { // Replace lowest score if new score is higher
        scores[4] = score;
        scoreTimes[4] = time;
    }
    // Sort scores in descending order
    for (int i = 0; i < scoreCount - 1; i++)
        for (int j = 0; j < scoreCount - i - 1; j++)
            if (scores[j] < scores[j + 1]) {
                int tempScore = scores[j];
                float tempTime = scoreTimes[j];
                scores[j] = scores[j + 1];
                scoreTimes[j] = scoreTimes[j + 1];
                scores[j + 1] = tempScore;
                scoreTimes[j + 1] = tempTime;
            }
    // Save scoreboard to file
    std::ofstream outFile("scoreboard.txt");
    for (int i = 0; i < scoreCount; i++) {
        outFile << scores[i] << " " << scoreTimes[i] << "\n";
    }
    outFile.close();
}

// Check if score qualifies for top 5
bool isNewHighScore(int score) {
    if (scoreCount < 5) return true;
    for (int i = 0; i < 5; i++) {
        if (score > scores[i]) return true;
    }
    return false;
}

// Reset game state for new game
void resetGame() {
    // Initialize grid: walls (1) on borders, empty (0) inside
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            grid[i][j] = (i == 0 || j == 0 || i == M - 1 || j == N - 1) ? 1 : 0;
    // Reset Player 1
    p1_x = 10; p1_y = 0; p1_dx = 0; p1_dy = 0; p1_moves = 0; p1_score = 0;
    p1_bonusCount = 0; p1_bonusThreshold = 10; p1_powerUps = 0; p1_moved = false;
    // Reset Player 2
    p2_x = N - 11; p2_y = M - 1; p2_dx = 0; p2_dy = 0; p2_moves = 0; p2_score = 0;
    p2_bonusCount = 0; p2_bonusThreshold = 10; p2_powerUps = 0; p2_moved = false;
    // Reset game time
    gameTime = 0;
    // Set enemy count based on level
    if (selectedLevel == 0) enemyCount = 2; // Easy
    else if (selectedLevel == 1) enemyCount = 4; // Medium
    else if (selectedLevel == 2) enemyCount = 6; // Hard
    else enemyCount = 2; // Continuous
    // Enable continuous mode for level 3
    continuousMode = (selectedLevel == 3);
    // Initialize all enemies
    for (int i = 0; i < 10; i++) {
        initEnemy(i);
    }
}

// Complete an enclosed area, update score and grid
void completeArea(int &x, int &y, int &dx, int &dy, int &score, int &bonusCount, int &bonusThreshold, int &powerUps, int trailValue) {
    // Check if player returned to a wall (grid[y][x] == 1) while moving
    if (grid[y][x] == 1 && (dx != 0 || dy != 0)) {
        dx = dy = 0; // Stop movement
        // Mark areas containing enemies as temporary (-1)
        for (int i = 0; i < enemyCount; i++)
            drop(enemy_y[i] / ts, enemy_x[i] / ts);
        int tilesCaptured = 0;
        // Convert non-temporary areas to walls (1), count captured tiles
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++)
                if (grid[i][j] == -1) {
                    grid[i][j] = 0; // Reset temporary areas
                } else if (grid[i][j] != trailValue && grid[i][j] != 1) {
                    grid[i][j] = 1; // Convert to wall
                    tilesCaptured++;
                }
        // Convert player trail to walls, count as captured
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++)
                if (grid[i][j] == trailValue) {
                    grid[i][j] = 1;
                    tilesCaptured++;
                }
        // Calculate score with bonus multiplier
        int multiplier = 1;
        if (tilesCaptured > bonusThreshold) {
            multiplier = (bonusCount >= 5 ? 4 : 2); // Double or quadruple points
            bonusCount++;
            if (bonusCount == 3) bonusThreshold = 5; // Lower threshold after 3 bonuses
        }
        score += tilesCaptured * multiplier;
        // Update power-ups based on score
        int prevPowerUps = powerUps;
        if (score >= 50) powerUps = 1 + (score - 50) / 30;
        powerUps = (powerUps > prevPowerUps ? powerUps : prevPowerUps);
    }
}

int main() {
    // Seed random number generator
    srand(time(0));
    // Create game window
    RenderWindow window(VideoMode(N * ts, M * ts), "Xonix Game!");
    window.setFramerateLimit(60);

    // Load font
    Font font;
    if (!font.loadFromFile("LemonMilk.otf")) return EXIT_FAILURE;

    // Load textures
    Texture t1, t2, t3, t4;
    t1.loadFromFile("images/tiles.png"); // Walls and trails
    t2.loadFromFile("images/gameover.png"); // Game over screen
    t3.loadFromFile("images/enemy.png"); // Enemy sprite
    t4.loadFromFile("images/tiles.png"); // Player 2 sprite
    // Create sprites
    Sprite sTile(t1), sGameover(t2), sEnemy(t3), sPlayer2(t4);
    sGameover.setPosition(100, 100);
    sEnemy.setOrigin(20, 20); // Center enemy sprite

    // Set up text objects for HUD, menu, and scoreboard
    Text hudText, menuText, scoreText;
    hudText.setFont(font);
    hudText.setCharacterSize(20);
    hudText.setFillColor(Color::White);
    hudText.setPosition(10, 10);
    menuText.setFont(font);
    menuText.setCharacterSize(30);
    menuText.setFillColor(Color::White);
    scoreText.setFont(font);
    scoreText.setCharacterSize(20);
    scoreText.setFillColor(Color::White);

    // Game control variables
    bool Game = true; // Game active flag
    float timer = 0, delay = 0.07; // Movement delay
    float enemyTimer = 0; // Tracks time for enemy speed/pattern updates
    float powerUpTimer = 0; // Power-up duration
    bool powerUpActive = false; // Power-up active flag
    bool paused = false; // Pause state
    int pauseSelection = 0; // Pause menu selection
    Clock clock; // Game clock

    // Load scoreboard from file
    std::ifstream inFile("scoreboard.txt");
    if (inFile.is_open()) {
        int s;
        float t;
        while (scoreCount < 5 && inFile >> s >> t) {
            scores[scoreCount] = s;
            scoreTimes[scoreCount] = t;
            scoreCount++;
        }
        inFile.close();
    }

    // Load sound effects
    sf::SoundBuffer moveBuffer, completeBuffer, gameOverBuffer;
    if (!moveBuffer.loadFromFile("playermoved.wav") || !completeBuffer.loadFromFile("completedmove.wav") || !gameOverBuffer.loadFromFile("gameover.wav")) return EXIT_FAILURE;
    sf::Sound moveSound(moveBuffer), completeSound(completeBuffer), gameOverSound(gameOverBuffer);

    // Initialize game
    resetGame();

    // Main game loop
    while (window.isOpen()) {
        // Update timers
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;
        if (Game && gameState == 1 && !paused) {
            gameTime += time;
            enemyTimer += time;
        }

        // Handle events
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
            if (e.type == Event::KeyPressed) {
                if (gameState == 0) { // Main menu
                    if (e.key.code == Keyboard::Up && selectedLevel > 0) selectedLevel--;
                    if (e.key.code == Keyboard::Down && selectedLevel < 5) selectedLevel++;
                    if (e.key.code == Keyboard::Left) twoPlayerMode = false;
                    if (e.key.code == Keyboard::Right) twoPlayerMode = true;
                    if (e.key.code == Keyboard::Return) {
                        if (selectedLevel == 4) gameState = 3; // Show scoreboard
                        else if (selectedLevel == 5) window.close(); // Exit
                        else {
                            gameState = 1; // Start game
                            resetGame();
                            Game = true;
                            paused = false;
                        }
                    }
                } else if (gameState == 2) { // Game over
                    if (e.key.code == Keyboard::R) {
                        gameState = 1;
                        resetGame();
                        Game = true;
                        paused = false;
                    }
                    if (e.key.code == Keyboard::M) gameState = 0;
                    if (e.key.code == Keyboard::Q) window.close();
                } else if (gameState == 3) { // Scoreboard
                    if (e.key.code == Keyboard::Escape) gameState = 0;
                } else if (gameState == 1 && Game) { // Playing
                    if (e.key.code == Keyboard::Escape) {
                        paused = !paused;
                        pauseSelection = 0;
                    }
                    if (paused) {
                        if (e.key.code == Keyboard::Up && pauseSelection > 0) pauseSelection--;
                        if (e.key.code == Keyboard::Down && pauseSelection < 2) pauseSelection++;
                        if (e.key.code == Keyboard::Return) {
                            if (pauseSelection == 0) paused = false;
                            else if (pauseSelection == 1) {
                                gameState = 0;
                                paused = false;
                            } else if (pauseSelection == 2) window.close();
                        }
                    } else {
                        // Activate power-ups
                        if (e.key.code == Keyboard::P && p1_powerUps > 0) {
                            p1_powerUps--;
                            powerUpActive = true;
                            powerUpTimer = 3;
                            if (twoPlayerMode) p2_dx = p2_dy = 0; // Freeze P2
                        }
                        if (e.key.code == Keyboard::O && twoPlayerMode && p2_powerUps > 0) {
                            p2_powerUps--;
                            powerUpActive = true;
                            powerUpTimer = 3;
                            p1_dx = p1_dy = 0; // Freeze P1
                        }
                    }
                }
            }
        }

        // Main menu rendering
        if (gameState == 0) {
            window.clear(sf::Color(96, 157, 255));
            std::stringstream menu;
            menu << "Xonix Game\n\n"
                 << (selectedLevel == 0 ? "> " : "  ") << "Easy\n"
                 << (selectedLevel == 1 ? "> " : "  ") << "Medium\n"
                 << (selectedLevel == 2 ? "> " : "  ") << "Hard\n"
                 << (selectedLevel == 3 ? "> " : "  ") << "Continuous\n"
                 << (selectedLevel == 4 ? "> " : "  ") << "Scoreboard\n"
                 << (selectedLevel == 5 ? "> " : "  ") << "Exit\n\n" 
                 << "Mode: " << (twoPlayerMode ? "Two Player" : "Single Player") << "\n"
                 << "Use Arrows to select, Enter to confirm"; 
            menuText.setString(menu.str());
            menuText.setPosition(100, 50);
            window.draw(menuText);
            window.display();
            continue;
        } else if (gameState == 3) { // Scoreboard rendering
            window.clear(sf::Color(100, 60, 30)); // Muted orange background for scoreboard
            std::stringstream score;
            score << "Scoreboard\n\n";
            for (int i = 0; i < scoreCount; i++) {
                score << (i + 1) << ". Score: " << scores[i] << ", Time: " << int(scoreTimes[i]) << "s\n";
            }
            scoreText.setString(score.str());
            scoreText.setPosition(100, 50);
            window.draw(scoreText);
            window.display();
            continue;
        } else if (gameState == 2) { // Game over rendering
            window.clear(sf::Color(100, 60, 30)); // Muted orange background for game over screen
            // Draw grid
            for (int i = 0; i < M; i++)
                for (int j = 0; j < N; j++) {
                    if (grid[i][j] == 0) continue;
                    if (grid[i][j] == 1) sTile.setTextureRect(IntRect(0, 0, ts, ts));
                    if (grid[i][j] == 2) sTile.setTextureRect(IntRect(54, 0, ts, ts));
                    if (grid[i][j] == 3) sTile.setTextureRect(IntRect(36, 0, ts, ts));
                    sTile.setPosition(j * ts, i * ts);
                    window.draw(sTile);
                }
            // Draw Player 1
            sTile.setTextureRect(IntRect(36, 0, ts, ts));
            sTile.setPosition(p1_x * ts, p1_y * ts);
            window.draw(sTile);
            // Draw Player 2
            if (twoPlayerMode) {
                sPlayer2.setTextureRect(IntRect(18, 0, ts, ts));
                sPlayer2.setPosition(p2_x * ts, p2_y * ts);
                window.draw(sPlayer2);
            }
            // Draw enemies
            for (int i = 0; i < enemyCount; i++) {
                sEnemy.setPosition(enemy_x[i], enemy_y[i]);
                window.draw(sEnemy);
            }
            window.draw(sGameover);
            if (!gameOverSound.getStatus()) gameOverSound.play();
            // Display scores and options
            std::stringstream end;
            end << "Game Over!\n\n";
            bool p1HighScore = isNewHighScore(p1_score);
            end << "P1 Score: " << p1_score << (p1HighScore ? " (New High Score!)" : "") << "\n"
                << "P1 Moves: " << p1_moves << "\n";
            if (twoPlayerMode) {
                bool p2HighScore = isNewHighScore(p2_score);
                end << "P2 Score: " << p2_score << (p2HighScore ? " (New High Score!)" : "") << "\n"
                    << "P2 Moves: " << p2_moves << "\n"
                    << "Winner: " << (p1_score > p2_score ? "Player 1" : p1_score < p2_score ? "Player 2" : "Tie") << "\n";
            }
            end << "\nR Restart\nM Menu\nQ Quit";
            menuText.setString(end.str());
            menuText.setPosition(100, 50);
            window.draw(menuText);
            window.display();
            continue;
        }

        // Pause menu rendering
        if (gameState == 1 && paused) {
            window.clear(sf::Color(96, 157, 255)); 
            // Draw grid
            for (int i = 0; i < M; i++)
                for (int j = 0; j < N; j++) {
                    if (grid[i][j] == 0) continue;
                    if (grid[i][j] == 1) sTile.setTextureRect(IntRect(0, 0, ts, ts));
                    if (grid[i][j] == 2) sTile.setTextureRect(IntRect(54, 0, ts, ts));
                    if (grid[i][j] == 3) sTile.setTextureRect(IntRect(36, 0, ts, ts));
                    sTile.setPosition(j * ts, i * ts);
                    window.draw(sTile);
                }
            // Draw Player 1
            sTile.setTextureRect(IntRect(36, 0, ts, ts));
            sTile.setPosition(p1_x * ts, p1_y * ts);
            window.draw(sTile);
            // Draw Player 2
            if (twoPlayerMode) {
                sPlayer2.setTextureRect(IntRect(18, 0, ts, ts));
                sPlayer2.setPosition(p2_x * ts, p2_y * ts);
                window.draw(sPlayer2);
            }
            // Draw enemies
            for (int i = 0; i < enemyCount; i++) {
                sEnemy.setPosition(enemy_x[i], enemy_y[i]);
                window.draw(sEnemy);
            }
            // Display pause menu
            std::stringstream pauseMenu;
            pauseMenu << "Paused\n\n"
                      << (pauseSelection == 0 ? "> " : "  ") << "Resume\n"
                      << (pauseSelection == 1 ? "> " : "  ") << "Main Menu\n"
                      << (pauseSelection == 2 ? "> " : "  ") << "Exit\n\n"
                      << "Use Up/Down to select, Enter to confirm";
            menuText.setString(pauseMenu.str());
            menuText.setPosition(100, 50);
            window.draw(menuText);
            window.display();
            continue;
        }

        // Handle game over
        if (!Game) {
            updateScoreboard(twoPlayerMode ? (p1_score > p2_score ? p1_score : p2_score) : p1_score, gameTime);
            gameState = 2;
            continue;
        }

        // Update power-up timer
        if (powerUpActive) {
            powerUpTimer -= time;
            if (powerUpTimer <= 0) powerUpActive = false;
        }

        // Increase enemy speed and add enemies in continuous mode
        if (enemyTimer >= 20) {
            for (int i = 0; i < enemyCount; i++) enemy_speed[i] += 0.5;
            if (continuousMode && enemyCount < 10) {
                initEnemy(enemyCount);
                initEnemy(enemyCount + 1);
                enemyCount += 2;
            }
            enemyTimer = 0;
        }

        // Change enemy patterns after 30 seconds
        if (gameTime >= 30) {
            for (int i = 0; i < enemyCount; i += 2) {
                enemy_pattern[i] = 1; // Zigzag
                if (i + 1 < enemyCount) enemy_pattern[i + 1] = 2; // Circular
            }
        }

        // Reset movement flags
        p1_moved = false;
        p2_moved = false;

        // Handle Player 1 input
        if (Keyboard::isKeyPressed(Keyboard::Left)) { p1_dx = -1; p1_dy = 0; p1_moved = true; }
        if (Keyboard::isKeyPressed(Keyboard::Right)) { p1_dx = 1; p1_dy = 0; p1_moved = true; }
        if (Keyboard::isKeyPressed(Keyboard::Up)) { p1_dx = 0; p1_dy = -1; p1_moved = true; }
        if (Keyboard::isKeyPressed(Keyboard::Down)) { p1_dx = 0; p1_dy = 1; p1_moved = true; }
        // Handle Player 2 input
        if (twoPlayerMode) {
            if (Keyboard::isKeyPressed(Keyboard::A)) { p2_dx = -1; p2_dy = 0; p2_moved = true; }
            if (Keyboard::isKeyPressed(Keyboard::D)) { p2_dx = 1; p2_dy = 0; p2_moved = true; }
            if (Keyboard::isKeyPressed(Keyboard::W)) { p2_dx = 0; p2_dy = -1; p2_moved = true; }
            if (Keyboard::isKeyPressed(Keyboard::S)) { p2_dx = 0; p2_dy = 1; p2_moved = true; }
        }

        // Update player positions
        if (timer > delay) {
            if (p1_dx != 0 || p1_dy != 0) {
                p1_x += p1_dx;
                p1_y += p1_dy;
                // Keep player within bounds
                if (p1_x < 0) p1_x = 0; if (p1_x > N - 1) p1_x = N - 1;
                if (p1_y < 0) p1_y = 0; if (p1_y > M - 1) p1_y = M - 1;
                // Game over if player hits own or opponent's trail
                if (grid[p1_y][p1_x] == 2 || grid[p1_y][p1_x] == 3) Game = false;
                // Leave trail in empty space
                if (grid[p1_y][p1_x] == 0) grid[p1_y][p1_x] = 2;
                if (p1_moved) p1_moves++;
                // Game over if players collide
                if (twoPlayerMode && p1_x == p2_x && p1_y == p2_y) Game = false;
                if (twoPlayerMode && grid[p1_y][p1_x] == 3 && (p1_dx != 0 || p1_dy != 0)) Game = false;
            }
            if (twoPlayerMode && (p2_dx != 0 || p2_dy != 0)) {
                p2_x += p2_dx;
                p2_y += p2_dy;
                // Keep player within bounds
                if (p2_x < 0) p2_x = 0; if (p2_x > N - 1) p2_x = N - 1;
                if (p2_y < 0) p2_y = 0; if (p2_y > M - 1) p2_y = M - 1;
                // Game over if player hits own or opponent's trail
                if (grid[p2_y][p2_x] == 2 || grid[p2_y][p2_x] == 3) Game = false;
                // Leave trail in empty space
                if (grid[p2_y][p2_x] == 0) grid[p2_y][p2_x] = 3;
                if (p2_moved) p2_moves++;
                if (grid[p2_y][p2_x] == 2 && (p2_dx != 0 || p2_dy != 0)) Game = false;
            }
            timer = 0;
        }

        // Move enemies if no power-up is active
        if (!powerUpActive) {
            for (int i = 0; i < enemyCount; i++) moveEnemy(i);
        }

        // Check for area completion
        completeArea(p1_x, p1_y, p1_dx, p1_dy, p1_score, p1_bonusCount, p1_bonusThreshold, p1_powerUps, 2);
        if (twoPlayerMode)
            completeArea(p2_x, p2_y, p2_dx, p2_dy, p2_score, p2_bonusCount, p2_bonusThreshold, p2_powerUps, 3);

        // Game over if enemy hits player trail
        for (int i = 0; i < enemyCount; i++)
            if (grid[enemy_y[i] / ts][enemy_x[i] / ts] == 2 || grid[enemy_y[i] / ts][enemy_x[i] / ts] == 3)
                Game = false;

        // Set fixed deep blue background for gameplay
        window.clear(sf::Color(11, 15, 26));

        // Draw grid
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++) {
                if (grid[i][j] == 0) continue;
                if (grid[i][j] == 1) sTile.setTextureRect(IntRect(0, 0, ts, ts));
                if (grid[i][j] == 2) sTile.setTextureRect(IntRect(54, 0, ts, ts));
                if (grid[i][j] == 3) sTile.setTextureRect(IntRect(36, 0, ts, ts));
                sTile.setPosition(j * ts, i * ts);
                window.draw(sTile);
            }
        // Draw Player 1
        sTile.setTextureRect(IntRect(36, 0, ts, ts));
        sTile.setPosition(p1_x * ts, p1_y * ts);
        window.draw(sTile);
        // Draw Player 2
        if (twoPlayerMode) {
            sPlayer2.setTextureRect(IntRect(18, 0, ts, ts));
            sPlayer2.setPosition(p2_x * ts, p2_y * ts);
            window.draw(sPlayer2);
        }
        // Draw rotating enemies
        sEnemy.rotate(10);
        for (int i = 0; i < enemyCount; i++) {
            sEnemy.setPosition(enemy_x[i], enemy_y[i]);
            window.draw(sEnemy);
        }
        // Play sound effects
        if (p1_moved && !moveSound.getStatus()) moveSound.play();
        if (grid[p1_y][p1_x] == 1 && (p1_dx != 0 || p1_dy != 0) && !completeSound.getStatus()) completeSound.play();
        // Display HUD
        std::stringstream hud;
        hud << "Time: " << int(gameTime) << "s\n"
            << "P1: " << p1_score << " (" << p1_moves << " moves, " << p1_powerUps << " power-ups)\n";
        if (twoPlayerMode)
            hud << "P2: " << p2_score << " (" << p2_moves << " moves, " << p2_powerUps << " power-ups)";
        hudText.setString(hud.str());
        window.draw(hudText);
        window.display();
    }
    return 0;
}