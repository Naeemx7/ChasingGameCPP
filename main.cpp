/**
 * Cat and Mouse - The Grand Chase!
 *
 * Copyright (c) 2025 Mohamed Naeem
 *
 * This software is licensed under the MIT License.
 * See the LICENSE file for details.
 *
 * ---
 *
 * A simple 2D maze game built with OpenGL and C++.
 * The player (mouse) must collect all the cheese in a level
 * while avoiding the cat, which uses a BFS pathfinding algorithm
 * to hunt the player. The game features multiple levels, power-ups,
 * and a progressively increasing difficulty.
 *
 * Author: Mohamed Naeem
 * Computer Science, 3rd Year
 *
 * This project serves as a practical application of Computer Graphics,
 * game state management, and basic AI in C++.
 */

#include <GL/glut.h>
#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <cmath>
#include <algorithm>

// --- Game & Window Configuration ---
const int ROWS = 23;
const int COLS = 23;
const int CELL_SIZE = 25;
const int WINDOW_WIDTH = COLS * CELL_SIZE;
const int WINDOW_HEIGHT = ROWS * CELL_SIZE;
const int TUNNEL_ROW_INDEX = 11; // The Y-index for the horizontal tunnel.

// --- Maze Tile Definitions ---
const int TILE_WALL = 1;
const int TILE_PATH = 0;
const int TILE_BLOCKED = 3; // Unused in current logic, but available.
const int TILE_SLOW_POWERUP = 4;

// --- Gameplay Constants ---
const int PLAYER_START_X = 1;
const int PLAYER_START_Y = 1;
const int CAT_START_X = 11;
const int CAT_START_Y = 11;
const int MAX_LEVELS = 3;
const int NUM_CHEESE_TO_PLACE = 12;
const int NUM_POWERUPS_PER_LEVEL = 1;

// --- Game State Management ---
enum GameState { INTRO, START_MENU, PLAYING, PAUSED, GAME_OVER, GAME_WON_LEVEL, GAME_WON_FINAL };
GameState currentGameState = INTRO;

// --- Dynamic Game Variables ---
int playerX = PLAYER_START_X;
int playerY = PLAYER_START_Y;
int catX = CAT_START_X;
int catY = CAT_START_Y;
int maze[ROWS][COLS];
int currentLevel = 1;
int score = 0;
int totalScore = 0;
int initialCheeseCount = 0;
std::vector<std::pair<int, int>> cheeseLocations;

// Power-up structure and storage
struct Powerup {
    int x, y;
    int type = TILE_SLOW_POWERUP;
    float sparklePhase = 0.0f;
};
std::vector<Powerup> powerupLocations;
bool isCatSlowed = false;
int catSlowDurationTimer = 0;
const int CAT_SLOW_DURATION_MS = 5000;

// Cat speed logic
const int INITIAL_CAT_DELAY_MS = 350;
const int MIN_CAT_DELAY_MS = 150;
const int DELAY_REDUCTION_PER_CHEESE = (NUM_CHEESE_TO_PLACE > 1) ? ((INITIAL_CAT_DELAY_MS - MIN_CAT_DELAY_MS) / (NUM_CHEESE_TO_PLACE - 1)) : 0;
int currentCatDelay = INITIAL_CAT_DELAY_MS;
int normalCatDelayBeforeSlowdown = 0;

// Timer and state management variables
int lastTickTime = 0;
bool timerActive = false;
int resetIdentifier = 0;

// --- Drawing & Style Constants ---
const double TWICE_PI = 6.283185307179586;
const float INNER_WALL_RADIUS = 7.0f;
const float OUTLINE_WIDTH = 2.0f;
const float OUTER_WALL_RADIUS = INNER_WALL_RADIUS + OUTLINE_WIDTH;
const float OUTLINE_COLOR_R = 0.0f; const float OUTLINE_COLOR_G = 0.0f; const float OUTLINE_COLOR_B = 0.5f;
const float FILL_COLOR_R = 0.0f; const float FILL_COLOR_G = 0.0f; const float FILL_COLOR_B = 1.0f;
const float POWERUP_COLOR_R = 0.2f; const float POWERUP_COLOR_G = 0.6f; const float POWERUP_COLOR_B = 1.0f;
const float CHEESE_SCALE_FACTOR = 0.7f;

// --- Function Declarations ---
void initMaze(int level);
void initLevelData();
void resetGame();
void nextLevel();
void processPlayerMove(int nextX, int nextY);
void drawFilledCircle(float cx, float cy, float radius, float r, float g, float b);
void drawConnectingRect(float x1, float y1, float x2, float y2, float radius, float r, float g, float b);
void drawFilledelipse(GLfloat x, GLfloat y, GLfloat radiusX, GLfloat radiusY);
void drawCustomCat(int gridX, int gridY, float cellSize);
void drawCustomMouse(int gridX, int gridY, float cellSize);
void drawCustomCheese(float drawX, float drawY, float drawSize);
void drawPowerup(float drawX, float drawY, float size, float sparklePhase);
void display();
void moveCat();
void catTimer(int value);
void keyboard(unsigned char key, int x, int y);
void specialKeyboard(int key, int x, int y);
void initOpenGL();
void idle();
void reshape(int w, int h);
int getTextWidth(const std::string& text, void* font);
void renderTextAt(float x, float y, const std::string& text, void* font, float r, float g, float b);
void renderCenteredText(float cx, float y, const std::string& text, void* font, float r, float g, float b);


// -----------------------------------------------------------------------------
// MAZE AND LEVEL INITIALIZATION
// -----------------------------------------------------------------------------

/**
 * @brief Selects and loads a maze layout based on the current level.
 * @param level The level number to load the maze for.
 */
void initMaze(int level) {
    int layout1[ROWS][COLS] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
        {1,0,1,0,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,0,1},
        {1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
        {1,1,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,1},
        {1,0,0,0,1,0,1,0,1,0,1,3,1,0,1,0,1,0,1,0,0,0,1},
        {1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1},
        {1,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1},
        {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1},
        {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
        {1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,0,1},
        {0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
        {1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1},
        {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
        {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1},
        {1,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1},
        {1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1},
        {1,0,0,0,1,0,1,0,1,0,1,3,1,0,1,0,1,0,1,0,0,0,1},
        {1,1,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,1},
        {1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
        {1,0,1,0,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };
    int layout2[ROWS][COLS] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,0,1,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1},
        {1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,0,1,0,1,1,0,1},
        {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
        {1,0,1,1,1,0,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1},
        {1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1,0,1,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1,0,1,1,1,1,1},
        {1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1},
        {1,0,1,0,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1,0,1},
        {1,0,1,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,1,0,1},
        {1,0,1,1,1,0,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
        {1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,0,1,0,1,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0,0,1},
        {1,0,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,0,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };
    int layout3[ROWS][COLS] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1},
        {1,0,0,0,1,0,0,0,1,0,1,0,1,0,1,1,1,0,1,0,0,0,1},
        {1,0,1,1,1,1,1,0,1,0,1,0,1,0,0,0,1,0,1,1,1,0,1},
        {1,0,0,0,1,0,0,0,1,0,0,0,1,1,1,0,1,0,0,0,1,0,1},
        {1,1,1,0,1,0,1,1,1,0,1,0,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1},
        {1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1},
        {1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1},
        {0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
        {1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1},
        {1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1},
        {1,0,0,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,1},
        {1,1,1,0,1,0,1,1,1,0,1,0,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,0,0,1,0,0,0,1,0,0,0,1,1,1,0,1,0,0,0,1,0,1},
        {1,0,1,1,1,1,1,0,1,0,1,0,1,0,0,0,1,0,1,1,1,0,1},
        {1,0,0,0,1,0,0,0,1,0,1,0,1,0,1,1,1,0,1,0,0,0,1},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1},
        {1,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };

    int (*selectedLayout)[COLS];
    switch (level) {
        case 2: selectedLayout = layout2; break;
        case 3: selectedLayout = layout3; break;
        default: selectedLayout = layout1;
    }
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            maze[y][x] = selectedLayout[y][x];
        }
    }
}

/**
 * @brief Populates the maze with cheese and power-ups for a new level.
 */
void initLevelData() {
    score = 0;
    currentCatDelay = INITIAL_CAT_DELAY_MS;
    cheeseLocations.clear();
    powerupLocations.clear();
    const int initialPlayerX = PLAYER_START_X;
    const int initialPlayerY = PLAYER_START_Y;
    const int initialCatX = CAT_START_X;
    const int initialCatY = CAT_START_Y;
    int placedCheese = 0;
    int placedPowerups = 0;
    int attempts = 0;
    const int maxAttempts = ROWS * COLS * 10;
    while (placedCheese < NUM_CHEESE_TO_PLACE && attempts < maxAttempts) {
        attempts++;
        int rx = rand() % COLS;
        int ry = rand() % ROWS;
        if (maze[ry][rx] == TILE_PATH && !(rx == initialPlayerX && ry == initialPlayerY) && !(rx == initialCatX && ry == initialCatY)) {
            bool alreadyExists = false;
            for (const auto& loc : cheeseLocations) {
                if (loc.first == rx && loc.second == ry) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                cheeseLocations.push_back({rx, ry});
                placedCheese++;
            }
        }
    }
    attempts = 0;
    while (placedPowerups < NUM_POWERUPS_PER_LEVEL && attempts < maxAttempts) {
         attempts++;
         int rx = rand() % COLS;
         int ry = rand() % ROWS;
         if (maze[ry][rx] == TILE_PATH && !(rx == initialPlayerX && ry == initialPlayerY) && !(rx == initialCatX && ry == initialCatY)) {
            bool cheeseExists = false;
            for (const auto& loc : cheeseLocations) {
                if (loc.first == rx && loc.second == ry) {
                    cheeseExists = true;
                    break;
                }
            }
            bool powerupExists = false;
            for (const auto& p : powerupLocations) {
                if (p.x == rx && p.y == ry) {
                    powerupExists = true;
                    break;
                }
            }
            if (!cheeseExists && !powerupExists) {
                powerupLocations.push_back({rx, ry, TILE_SLOW_POWERUP});
                placedPowerups++;
            }
         }
    }
    initialCheeseCount = cheeseLocations.size();
    if (placedCheese < NUM_CHEESE_TO_PLACE) std::cout << "Warning: Could only place " << placedCheese << " cheese.\n";
    if (placedPowerups < NUM_POWERUPS_PER_LEVEL) std::cout << "Warning: Could only place " << placedPowerups << " powerups.\n";
    if (initialCheeseCount > 0 || placedPowerups > 0) std::cout << "Level " << currentLevel << " started. Collect " << initialCheeseCount << " cheese! Cat Delay: " << currentCatDelay << "ms\n";
    else std::cout << "Warning: No items placed for level " << currentLevel << ".\n";
    playerX = PLAYER_START_X;
    playerY = PLAYER_START_Y;
    catX = CAT_START_X;
    catY = CAT_START_Y;
    isCatSlowed = false;
    catSlowDurationTimer = 0;
    normalCatDelayBeforeSlowdown = currentCatDelay;
}


// -----------------------------------------------------------------------------
// GAME STATE AND FLOW CONTROL
// -----------------------------------------------------------------------------

/**
 * @brief Resets the game to its initial state (Level 1, score 0).
 */
void resetGame() {
    std::cout << "--- Game Reset! ---\n";
    resetIdentifier++;
    timerActive = false;
    currentLevel = 1;
    totalScore = 0;
    initMaze(currentLevel);
    initLevelData();
    currentGameState = PLAYING;
    timerActive = true;
    lastTickTime = glutGet(GLUT_ELAPSED_TIME);
    catTimer(resetIdentifier);
    glutPostRedisplay();
}

/**
 * @brief Advances the game to the next level or triggers the win condition.
 */
void nextLevel() {
     totalScore += score;
     score = 0;
     currentLevel++;
     timerActive = false;
     if (currentLevel > MAX_LEVELS) {
         currentGameState = GAME_WON_FINAL;
         std::cout << "************************************\n*   You beat all levels! YOU WIN!  *\n*      Final Score: " << totalScore <<"           *\n************************************\n";
     } else {
         std::cout << "************************************\n*      Level Complete!             *\n*      Proceeding to Level " << currentLevel << "       *\n************************************\n";
         currentGameState = GAME_WON_LEVEL;
         glutTimerFunc(2000, [](int v_current_level_for_lambda){
              resetIdentifier++;
              initMaze(v_current_level_for_lambda);
              initLevelData();
              currentGameState = PLAYING;
              timerActive = true;
              lastTickTime = glutGet(GLUT_ELAPSED_TIME);
              catTimer(resetIdentifier);
              glutPostRedisplay();
         }, currentLevel);
     }
     glutPostRedisplay();
}


// -----------------------------------------------------------------------------
// DRAWING AND RENDERING
// -----------------------------------------------------------------------------

// --- Primitive Drawing Functions ---
void drawFilledCircle(float cx, float cy, float radius, float r, float g, float b) {
    int num_segments = 80; glColor3f(r, g, b); glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++) { float angle = i * TWICE_PI / num_segments; glVertex2f(cx + (cos(angle) * radius), cy + (sin(angle) * radius)); }
    glEnd();
}
void drawConnectingRect(float x1, float y1, float x2, float y2, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    if (abs(x1 - x2) > abs(y1 - y2)) { glBegin(GL_QUADS); glVertex2f(x1, y1 - radius); glVertex2f(x2, y2 - radius); glVertex2f(x2, y2 + radius); glVertex2f(x1, y1 + radius); glEnd(); }
    else { glBegin(GL_QUADS); glVertex2f(x1 - radius, y1); glVertex2f(x2 - radius, y2); glVertex2f(x2 + radius, y2); glVertex2f(x1 + radius, y1); glEnd(); }
}
void drawFilledelipse(GLfloat x, GLfloat y, GLfloat radiusX, GLfloat radiusY) {
    int triangleAmount = 100; glBegin(GL_TRIANGLE_FAN); glVertex2f(x, y);
    for (int i = 0; i <= triangleAmount; i++) { glVertex2f( x + (radiusX * cos(i * TWICE_PI / triangleAmount)), y + (radiusY * sin(i * TWICE_PI / triangleAmount))); }
    glEnd();
}

// --- Custom Sprite Drawing Functions ---
void drawCustomCat(int gridX, int gridY, float cellSize) {
    glPushMatrix();
    float cX = (gridX + 0.5f) * cellSize; float cY = (gridY + 0.5f) * cellSize;
    glTranslatef(cX, cY, 0.0f);
    float oS = 180.0f; float sF = cellSize / oS; glScalef(sF, sF, 1.0f); glTranslatef(-230.0f, -250.0f, 0.0f);
    glColor3f(1.00f,0.47f,0.00f); drawFilledelipse(232.7689f, 172.0119f, 69.3266f, 69.3266f); glColor3f(0.90f,0.38f,0.00f); glBegin(GL_TRIANGLES); glVertex2f(252.96f,110.04f); glVertex2f(290.41f,135.72f); glVertex2f(310.89f,65.695f); glEnd(); glBegin(GL_TRIANGLES); glVertex2f(174.93f,136.42f); glVertex2f(210.51f,108.80f); glVertex2f(150.53f,68.27f); glEnd();
    glColor3f(0.95f,0.66f,0.66f); glBegin(GL_TRIANGLES); glVertex2f(241.56f,174.11f); glVertex2f(219.05f,177.14f); glVertex2f(233.345f,198.14f); glEnd(); glColor3f(1.00f,0.47f,0.00f); glBegin(GL_TRIANGLES); glVertex2f(156.53f,378.37f); glVertex2f(302.85f,380.62f); glVertex2f(231.94f,233.175f); glEnd();
    glColor3f(0.78f,0.27f,0.00f); glBegin(GL_TRIANGLES); glVertex2f(236.06f,387.65f); glVertex2f(292.63f,387.65f); glVertex2f(264.345f,331.08f); glEnd(); glBegin(GL_TRIANGLES); glVertex2f(162.35f,389.14f); glVertex2f(218.43f,389.14f); glVertex2f(190.39f,333.07f); glEnd();
    glColor3f(1.00f,0.47f,0.00f); glBegin(GL_TRIANGLES); glVertex2f(271.27f,333.98f); glVertex2f(287.44f,380.18f); glVertex2f(374.69f,323.73f); glEnd(); glColor3f(0.96f,0.96f,0.96f); drawFilledelipse(249.950f, 156.324f, 16.320f, 16.320f); drawFilledelipse(204.133f, 158.316f, 15.334f, 15.334f);
    glColor3f(0.18f,0.76f,0.49f); drawFilledelipse(250.209f, 156.725f, 8.064f, 16.719f); drawFilledelipse(204.210f, 158.453f, 7.890f, 16.552f); glColor3f(0.00f,0.00f,0.00f); drawFilledelipse(248.246f, 157.819f, 4.104f, 6.125f); drawFilledelipse(204.976f, 160.147f, 4.230f, 6.271f);
    glColor3f(1.00f,0.64f,0.28f); glBegin(GL_TRIANGLES); glVertex2f(341.13f,320.54f); glVertex2f(351.03f,347.20f); glVertex2f(372.74f,323.96f); glEnd(); glColor3f(0.88f,0.11f,0.14f); glBegin(GL_TRIANGLES); glVertex2f(200.20f,244.58f); glVertex2f(199.80f,281.38f); glVertex2f(236.805f,263.38f); glEnd(); glBegin(GL_TRIANGLES); glVertex2f(253.96f,281.06f); glVertex2f(253.87f,243.84f); glVertex2f(219.355f,262.53f); glEnd();
    glPopMatrix();
}
void drawCustomMouse(int gridX, int gridY, float cellSize) {
    glPushMatrix(); float cX = (gridX + 0.5f) * cellSize; float cY = (gridY + 0.5f) * cellSize; glTranslatef(cX, cY, 0.0f);
    float oS = 300.0f; float sF = cellSize / oS; glScalef(sF, sF, 1.0f); float oCX = 250.0f; float oCY = 200.0f; glTranslatef(-oCX, -oCY, 0.0f);
    glColor3f(0.07f,0.07f,0.07f); glBegin(GL_TRIANGLES); glVertex2f(162.35f,333.86f); glVertex2f(341.44f,333.86f); glVertex2f(251.89499999999998f,154.78f); glEnd(); glColor3f(0.90f,0.90f,0.90f); glBegin(GL_TRIANGLES); glVertex2f(167.29f,330.87f); glVertex2f(336.42f,330.87f); glVertex2f(251.85500000000002f,161.75f); glEnd();
    glColor3f(0.00f,0.00f,0.00f); drawFilledelipse(189.34462151394422f, 84.76294820717129f, 50.0f, 50.0f); glColor3f(0.07f,0.07f,0.07f); drawFilledelipse(307.87051792828686f, 101.69521912350598f, 50.0f, 50.0f);
    glColor3f(0.90f,0.90f,0.90f); drawFilledelipse(190.2390438247012f, 84.91035856573703f, 47.828488f, 47.088872f); glColor3f(0.90f,0.90f,0.90f); drawFilledelipse(306.9472111553786f, 102.31573705179287f, 47.113526f, 47.113526f);
    glColor3f(0.07f,0.07f,0.07f); glBegin(GL_TRIANGLES); glVertex2f(153.39f,189.44f); glVertex2f(344.62f,189.44f); glVertex2f(249.005f,60.16f); glEnd(); glColor3f(0.90f,0.90f,0.90f); glBegin(GL_TRIANGLES); glVertex2f(160.36f,185.26f); glVertex2f(336.80f,185.26f); glVertex2f(248.58f,65.88f); glEnd();
    glColor3f(0.07f,0.07f,0.07f); drawFilledelipse(158.56573705179278f, 185.4581673306773f, 19.525857f, 19.525857f); glColor3f(0.81f,0.30f,0.82f); drawFilledelipse(158.56573705179278f, 185.45816733067736f, 16.567394f, 16.567394f);
    glColor3f(0.00f,0.00f,0.00f); drawFilledelipse(203.6354581673307f, 122.95816733067728f, 17.800087f, 17.800087f); glColor3f(0.00f,0.00f,0.00f); drawFilledelipse(254.43227091633463f, 122.9581673306773f, 18.786241f, 18.786241f);
    glColor3f(1.00f,1.00f,1.00f); drawFilledelipse(202.68611090230274f, 123.21030344032678f, 15.282292f, 15.091263f); glColor3f(1.00f,1.00f,1.00f); drawFilledelipse(252.9912816267398f, 122.7059295286387f, 15.781672f, 15.578038f);
    glColor3f(0.00f,0.00f,0.00f); drawFilledelipse(206.92647570684872f, 125.25528639496653f, 5.180062f, 5.180062f); glColor3f(0.00f,0.00f,0.00f); drawFilledelipse(247.9675433326319f, 125.93683913748762f, 5.482440f, 5.482440f);
    glColor3f(0.00f,0.00f,0.00f); glBegin(GL_QUADS); glVertex2f(310.8885560144597f,325.8287950582844f); glVertex2f(272.31075697211134f,325.8287950582844f); glVertex2f(272.31075697211134f,352.39519146839973f); glVertex2f(310.8885560144597f,352.39519146839973f); glEnd(); glColor3f(0.00f,0.00f,0.00f); glBegin(GL_QUADS); glVertex2f(230.07968127490042f,326.6932270916334f); glVertex2f(191.2350597609562f,326.6932270916334f); glVertex2f(191.2350597609562f,353.585657370518f); glVertex2f(230.07968127490042f,353.585657370518f); glEnd();
    glColor3f(0.90f,0.90f,0.90f); glBegin(GL_QUADS); glVertex2f(307.6213555793895f,328.77290883825026f); glVertex2f(275.94871084890787f,328.60725693066405f); glVertex2f(275.8382762438504f,349.7223534176518f); glVertex2f(307.510920974332f,349.888005325238f); glEnd(); glColor3f(0.90f,0.90f,0.90f); glBegin(GL_QUADS); glVertex2f(226.0956175298804f,330.67729083665347f); glVertex2f(194.22310756972107f,330.67729083665347f); glVertex2f(194.22310756972107f,350.5976095617531f); glVertex2f(226.0956175298804f,350.5976095617531f); glEnd();
    glColor3f(0.07f,0.07f,0.07f); drawFilledelipse(213.74542556597365f, 347.39233073767116f, 1.578249f, 5.933972f); glColor3f(0.07f,0.07f,0.07f); drawFilledelipse(202.49779565683198f, 345.9240904775491f, 1.542416f, 5.360665f); glColor3f(0.07f,0.07f,0.07f); drawFilledelipse(296.31404619856903f, 343.5271936089276f, 1.478544f, 5.820971f); glColor3f(0.07f,0.07f,0.07f); drawFilledelipse(285.3606729855411f, 344.0237891658534f, 1.477145f, 6.315447f);
    glColor3f(0.07f,0.07f,0.07f); glBegin(GL_QUADS); glVertex2f(260.9561752988048f,209.00966201047677f); glVertex2f(238.20149336800927f,209.00966201047677f); glVertex2f(238.20149336800927f,223.9083665338645f); glVertex2f(260.9561752988048f,223.9083665338645f); glEnd(); glColor3f(0.07f,0.07f,0.07f); glBegin(GL_QUADS); glVertex2f(257.9681274900398f,221.51394422310756f); glVertex2f(241.03585657370513f,221.51394422310756f); glVertex2f(241.03585657370513f,275.8964143426295f); glVertex2f(257.9681274900398f,275.8964143426295f); glEnd();
    glColor3f(0.07f,0.07f,0.07f); glBegin(GL_TRIANGLES); glVertex2f(259.67f,274.34f); glVertex2f(239.29f,274.56f); glVertex2f(249.7f,294.825f); glEnd(); glColor3f(1.00f,1.00f,1.00f); glBegin(GL_QUADS); glVertex2f(257.71157368019675f,210.99185282412623f); glVertex2f(240.98235187084202f,210.99185282412623f); glVertex2f(240.98235187084202f,222.2895091109632f); glVertex2f(257.71157368019675f,222.2895091109632f); glEnd();
    glColor3f(0.81f,0.30f,0.82f); glBegin(GL_QUADS); glVertex2f(254.58167938597177f,222.32933355825492f); glVertex2f(244.3156875450216f,222.32933355825492f); glVertex2f(244.3156875450216f,279.2164623974804f); glVertex2f(254.58167938597177f,279.2164623974804f); glEnd(); glColor3f(0.00f,0.00f,0.00f); glBegin(GL_TRIANGLES); glVertex2f(259.67f,273.99f); glVertex2f(240.09f,273.92f); glVertex2f(249.81f,293.54499999999996f); glEnd();
    glColor3f(1.00f,1.00f,1.00f); drawFilledelipse(248.45266109692034f, 223.06097618311017f, 3.365244f, 3.365244f); glColor3f(1.00f,1.00f,1.00f); drawFilledelipse(250.26729267778978f, 234.97473632933173f, 4.543656f, 4.543656f); glColor3f(1.00f,1.00f,1.00f); drawFilledelipse(251.081391011246f, 249.0880387025031f, 2.715373f, 2.715373f); glColor3f(1.00f,1.00f,1.00f); drawFilledelipse(247.75954390486368f, 257.561048114541f, 3.207542f, 3.207542f);
    glColor3f(1.00f,1.00f,1.00f); glBegin(GL_TRIANGLES); glVertex2f(258.70f,273.66f); glVertex2f(240.98f,273.58f); glVertex2f(249.765f,291.33500000000004f); glEnd();
    glPopMatrix();
}
void drawCustomCheese(float drawX, float drawY, float drawSize) {
    glPushMatrix();
    glTranslatef(drawX, drawY, 0.0f);
    float originalApproxWidth = 400.0f; float scaleFactor = drawSize / originalApproxWidth;
    glScalef(scaleFactor, scaleFactor, 1.0f);
    float originalCenterX = 250.0f; float originalCenterY = 250.0f;
    glTranslatef(-originalCenterX, -originalCenterY, 0.0f);
    glColor3f(0.99f, 0.76f, 0.11f); glBegin(GL_QUADS); glVertex2f(432.978f, 252.265f); glVertex2f(124.052f, 252.689f); glVertex2f(124.231f, 383.511f); glVertex2f(433.158f, 383.087f); glEnd();
    glBegin(GL_TRIANGLES); glVertex2f(56.47f, 325.76f); glVertex2f(431.64f, 325.76f); glVertex2f(244.055f, 116.42f); glEnd();
    glBegin(GL_QUADS); glVertex2f(174.081f, 324.824f); glVertex2f(58.234f, 324.884f); glVertex2f(58.264f, 383.339f); glVertex2f(174.111f, 383.280f); glEnd();
    glBegin(GL_TRIANGLES); glVertex2f(105.93f, 270.02f); glVertex2f(432.89f, 255.18f); glVertex2f(261.235f, 82.59f); glEnd();
    glColor3f(0.97f, 0.60f, 0.0f); drawFilledelipse(283.188f, 186.741f, 22.877f, 22.877f);
    drawFilledelipse(198.921f, 248.636f, 16.232f, 16.232f); drawFilledelipse(162.132f, 325.197f, 19.185f, 19.185f);
    drawFilledelipse(266.534f, 294.374f, 31.983f, 31.983f); drawFilledelipse(351.049f, 251.619f, 22.139f, 22.139f);
    drawFilledelipse(360.246f, 327.434f, 13.525f, 13.525f); drawFilledelipse(352.534f, 250.909f, 24.573f, 24.573f);
    glColor3f(0.97f, 0.89f, 0.36f); glBegin(GL_QUADS); glVertex2f(433.989f, 373.506f); glVertex2f(57.249f, 373.506f); glVertex2f(57.249f, 388.924f); glVertex2f(433.989f, 388.924f); glEnd();
    glColor3f(0.99f, 0.76f, 0.11f); glBegin(GL_QUADS); glVertex2f(134.178f, 321.525f); glVertex2f(61.476f, 321.525f); glVertex2f(61.476f, 331.550f); glVertex2f(134.178f, 331.550f); glEnd();
    drawFilledelipse(61.732f, 324.959f, 2.587f, 2.587f); glBegin(GL_TRIANGLES); glVertex2f(357.51f, 301.30f); glVertex2f(431.26f, 252.31f); glVertex2f(306.17f, 144.0f); glEnd();
    glPopMatrix();
}
void drawPowerup(float drawX, float drawY, float size, float sparklePhase) {
    glPushMatrix();
    glTranslatef(drawX, drawY, 0.0f);
    glColor3f(POWERUP_COLOR_R, POWERUP_COLOR_G, POWERUP_COLOR_B);
    float radius = size * 0.4f;
    drawFilledCircle(0, 0, radius, POWERUP_COLOR_R, POWERUP_COLOR_G, POWERUP_COLOR_B);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f * (0.5f + 0.5f * sin(sparklePhase)));
    float sparkleRadius = radius * (0.6f + 0.2f * sin(sparklePhase));
    glEnable(GL_BLEND);
    drawFilledCircle(0, 0, sparkleRadius, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
    glPopMatrix();
}

// --- Text Rendering Utilities ---
int getTextWidth(const std::string& text, void* font) {
    int width = 0;
    for (char c : text) { width += glutBitmapWidth(font, c); }
    return width;
}
void renderTextAt(float x, float y, const std::string& text, void* font, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) { glutBitmapCharacter(font, c); }
}
void renderCenteredText(float cx, float y, const std::string& text, void* font, float r, float g, float b) {
    int textWidth = getTextWidth(text, font);
    renderTextAt(cx - (textWidth / 2.0f), y, text, font, r, g, b);
}

/**
 * @brief The main display callback function, responsible for all rendering.
 * It acts as a state machine, drawing different scenes based on the currentGameState.
 */
void display() {
    // Set the background color and clear the buffer
    glClearColor(0.05f, 0.05f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up a 2D orthographic projection matching the window dimensions.
    // The Y-axis is inverted (0 is at the top) to match a 2D array indexing scheme.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // The rendering process is layered:
    // 1. Maze Walls
    // 2. Heads-Up Display (HUD)
    // 3. Game Objects (Player, Cat, Items)
    // 4. Full-screen overlays (Menus, Pause Screen)

    // --- 1. Draw Maze Walls ---
    if (currentGameState == PLAYING || currentGameState == PAUSED) {
        for (int y = 0; y < ROWS; ++y) { for (int x = 0; x < COLS; ++x) { if (maze[y][x] == TILE_WALL) {
            float cX = (x + 0.5f) * CELL_SIZE, cY = (y + 0.5f) * CELL_SIZE;
            drawFilledCircle(cX, cY, OUTER_WALL_RADIUS, OUTLINE_COLOR_R, OUTLINE_COLOR_G, OUTLINE_COLOR_B);
            if (x + 1 < COLS && maze[y][x + 1] == TILE_WALL) { if (!(y == TUNNEL_ROW_INDEX && x == COLS - 1 && maze[y][0] == TILE_PATH)) drawConnectingRect(cX, cY, cX + CELL_SIZE, cY, OUTER_WALL_RADIUS, OUTLINE_COLOR_R, OUTLINE_COLOR_G, OUTLINE_COLOR_B); }
            if (y == TUNNEL_ROW_INDEX && x == COLS - 1 && maze[y][0] == TILE_WALL) drawConnectingRect(cX, cY, cX + CELL_SIZE, cY, OUTER_WALL_RADIUS, OUTLINE_COLOR_R, OUTLINE_COLOR_G, OUTLINE_COLOR_B);
            if (y + 1 < ROWS && maze[y + 1][x] == TILE_WALL) drawConnectingRect(cX, cY, cX, cY + CELL_SIZE, OUTER_WALL_RADIUS, OUTLINE_COLOR_R, OUTLINE_COLOR_G, OUTLINE_COLOR_B);
        } } }
        for (int y = 0; y < ROWS; ++y) { for (int x = 0; x < COLS; ++x) { if (maze[y][x] == TILE_WALL) {
            float cX = (x + 0.5f) * CELL_SIZE, cY = (y + 0.5f) * CELL_SIZE;
            drawFilledCircle(cX, cY, INNER_WALL_RADIUS, FILL_COLOR_R, FILL_COLOR_G, FILL_COLOR_B);
            if (x + 1 < COLS && maze[y][x + 1] == TILE_WALL) { if (!(y == TUNNEL_ROW_INDEX && x == COLS - 1 && maze[y][0] == TILE_PATH)) drawConnectingRect(cX, cY, cX + CELL_SIZE, cY, INNER_WALL_RADIUS, FILL_COLOR_R, FILL_COLOR_G, FILL_COLOR_B); }
            if (y == TUNNEL_ROW_INDEX && x == COLS - 1 && maze[y][0] == TILE_WALL) drawConnectingRect(cX, cY, cX + CELL_SIZE, cY, INNER_WALL_RADIUS, FILL_COLOR_R, FILL_COLOR_G, FILL_COLOR_B);
            if (y + 1 < ROWS && maze[y + 1][x] == TILE_WALL) drawConnectingRect(cX, cY, cX, cY + CELL_SIZE, INNER_WALL_RADIUS, FILL_COLOR_R, FILL_COLOR_G, FILL_COLOR_B);
        } } }
    }

    // --- 2. Draw In-Game HUD ---
    if (currentGameState == PLAYING) {
        // Vertical position for the text, placing it within the top row of maze cells.
        const float textY = 21.0f;
        void* font = GLUT_BITMAP_HELVETICA_18;

        std::stringstream ss_left;
        ss_left << "Level: " << currentLevel << "   Total Score: " << totalScore + score;
        renderTextAt(CELL_SIZE, textY, ss_left.str(), font, 1.0f, 1.0f, 1.0f);

        std::stringstream ss_right;
        ss_right << "Cheese Left: " << cheeseLocations.size();
        int rightTextWidth = getTextWidth(ss_right.str(), font);
        renderTextAt(WINDOW_WIDTH - rightTextWidth - CELL_SIZE, textY, ss_right.str(), font, 1.0f, 1.0f, 0.0f);

        if (isCatSlowed) {
            renderCenteredText(WINDOW_WIDTH / 2.0f, textY, "SLOWED!", font, 0.5f, 0.8f, 1.0f);
        }
    }

    // --- 3. Draw Game Objects (Characters and Items) ---
    if (currentGameState == PLAYING || currentGameState == PAUSED) {
        for (const auto& loc : cheeseLocations) { float cDX = (loc.first + 0.5f) * CELL_SIZE; float cDY = (loc.second + 0.5f) * CELL_SIZE; drawCustomCheese(cDX, cDY, CELL_SIZE * CHEESE_SCALE_FACTOR); }
        for (auto& p : powerupLocations) { float pDX = (p.x + 0.5f) * CELL_SIZE; float pDY = (p.y + 0.5f) * CELL_SIZE; drawPowerup(pDX, pDY, CELL_SIZE * CHEESE_SCALE_FACTOR, p.sparklePhase); }
        drawCustomMouse(playerX, playerY, CELL_SIZE);
        drawCustomCat(catX, catY, CELL_SIZE);
    }

    // --- 4. Draw Full-Screen Overlays (Menus) ---
    if (currentGameState == PAUSED) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f); // Semi-transparent black overlay
        glEnable(GL_BLEND);
        glRectf(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glDisable(GL_BLEND);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.45f, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24, 1.0f, 1.0f, 1.0f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.52f, "Press 'P' to Resume", GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f);
    } else if (currentGameState == GAME_OVER || currentGameState == GAME_WON_LEVEL || currentGameState == GAME_WON_FINAL) {
        float r, g, b;
        if (currentGameState == GAME_OVER) { r = 0.6f; g = 0.0f; b = 0.0f; } // Dark red for game over
        else { r = 0.0f; g = 0.5f; b = 0.1f; } // Dark green for win
        glColor4f(r, g, b, 0.75f);
        glEnable(GL_BLEND);
        glRectf(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glDisable(GL_BLEND);
        std::string message, score_message, action_message;
        if (currentGameState == GAME_OVER) { message = "GAME OVER!"; score_message = "Final Score: " + std::to_string(totalScore); action_message = "Press 'R' to Restart"; }
        else if (currentGameState == GAME_WON_FINAL) { message = "YOU BEAT THE GAME!"; score_message = "Grand Total Score: " + std::to_string(totalScore); action_message = "Press ESC to Quit";}
        else { message = "LEVEL " + std::to_string(currentLevel-1) + " COMPLETE!"; score_message = "Total Score: " + std::to_string(totalScore); action_message = "Loading next level..."; }
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.40f, message, GLUT_BITMAP_TIMES_ROMAN_24, 1.0f, 1.0f, 1.0f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.50f, score_message, GLUT_BITMAP_HELVETICA_18, 0.9f, 0.9f, 0.9f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.58f, action_message, GLUT_BITMAP_HELVETICA_18, 0.9f, 0.9f, 0.9f);
    } else if (currentGameState == INTRO) {
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.45f, "A Game By", GLUT_BITMAP_HELVETICA_18, 0.8f, 0.8f, 1.0f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.55f, "Mohamed Naeem", GLUT_BITMAP_TIMES_ROMAN_24, 1.0f, 1.0f, 1.0f);
    } else if (currentGameState == START_MENU) {
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.15f, "Cat and Mouse - The Grand Chase!", GLUT_BITMAP_TIMES_ROMAN_24, 1.0f, 1.0f, 1.0f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.28f, "Press ENTER to Start", GLUT_BITMAP_HELVETICA_18, 0.8f, 1.0f, 0.8f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.40f, "--- INSTRUCTIONS ---", GLUT_BITMAP_HELVETICA_18, 0.7f, 0.7f, 0.9f);
        float y_pos = WINDOW_HEIGHT * 0.48f;
        renderCenteredText(WINDOW_WIDTH / 2.0f, y_pos, "WASD or Arrow Keys to Move", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f); y_pos += 25;
        renderCenteredText(WINDOW_WIDTH / 2.0f, y_pos, "P to Pause / Resume", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f); y_pos += 25;
        renderCenteredText(WINDOW_WIDTH / 2.0f, y_pos, "R to Reset Game", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f); y_pos += 25;
        renderCenteredText(WINDOW_WIDTH / 2.0f, y_pos, "ESC to Quit", GLUT_BITMAP_HELVETICA_12, 1.0f, 1.0f, 1.0f); y_pos += 35;
        renderCenteredText(WINDOW_WIDTH / 2.0f, y_pos, "Collect all the cheese to advance, Avoid the cat it gets faster !", GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f); y_pos += 20;
        renderCenteredText(WINDOW_WIDTH / 2.0f, y_pos, "Blue items will temporarily slow the cat down.", GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f);

        // Developer and GitHub credits at the bottom of the start menu
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT - 50, "Game by Mohamed Naeem", GLUT_BITMAP_9_BY_15, 0.6f, 0.6f, 0.8f);
        renderCenteredText(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT - 35, "GitHub: Naeemx7", GLUT_BITMAP_9_BY_15, 0.6f, 0.6f, 0.8f);
    }

    glutSwapBuffers();
}


// -----------------------------------------------------------------------------
// GAME LOGIC AND AI
// -----------------------------------------------------------------------------

/**
 * @brief Uses Breadth-First Search (BFS) to find the shortest path from the cat to the player.
 * It then moves the cat one step along this path.
 */
void moveCat() {
    std::queue<std::pair<int, int>> q;
    int parent[ROWS][COLS][2];
    bool visited[ROWS][COLS] = {false};
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            parent[y][x][0] = parent[y][x][1] = -1;
        }
    }

    q.push({catX, catY});
    visited[catY][catX] = true;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    int targetX = -1, targetY = -1;
    bool found = false;

    // Standard BFS algorithm to find the player
    while (!q.empty() && !found) {
        std::pair<int, int> current = q.front();
        q.pop();
        int x_bfs = current.first;
        int y_bfs = current.second;

        if (x_bfs == playerX && y_bfs == playerY) {
            targetX = x_bfs;
            targetY = y_bfs;
            found = true;
            break;
        }

        for (int i = 0; i < 4 && !found; ++i) {
            int nx = x_bfs + dx[i];
            int ny = y_bfs + dy[i];

            if (ny == TUNNEL_ROW_INDEX) {
                if (nx < 0) nx = COLS - 1;
                else if (nx >= COLS) nx = 0;
            }

            if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && maze[ny][nx] == TILE_PATH && !visited[ny][nx]) {
                visited[ny][nx] = true;
                parent[ny][nx][0] = x_bfs;
                parent[ny][nx][1] = y_bfs;
                q.push({nx, ny});
                if (nx == playerX && ny == playerY) {
                    targetX = nx;
                    targetY = ny;
                    found = true;
                }
            }
        }
    }

    if (!found) return;

    // Backtrack from the player to find the cat's next move
    int cx = targetX;
    int cy = targetY;
    int nextStepX = -1, nextStepY = -1;
    while(true){
        if (cx == catX && cy == catY) break;
        int px = parent[cy][cx][0];
        int py = parent[cy][cx][1];
        if (px == -1 && py == -1) break;
        if (px == catX && py == catY) {
            nextStepX = cx;
            nextStepY = cy;
            break;
        }
        cx = px;
        cy = py;
    }

    // Update the cat's position if a valid step was found
    if (nextStepX != -1) {
        catX = nextStepX;
        catY = nextStepY;
    }

    // Check for collision with the player
    if (catX == playerX && catY == playerY && currentGameState == PLAYING) {
        std::cout << "Caught by the cat! Game Over. Current Level Score: " << score << std::endl;
        totalScore += score;
        score = 0;
        currentGameState = GAME_OVER;
        timerActive = false;
        std::cout << "Final Total Score: " << totalScore << std::endl;
    }
}

/**
 * @brief Timer callback that triggers the cat's movement periodically.
 * @param value A unique identifier to prevent multiple timers from running after a reset.
 */
void catTimer(int value) {
    if (value != resetIdentifier && currentGameState != START_MENU) {
        return;
    }
    if (currentGameState == PLAYING && timerActive) {
        if (!isCatSlowed) {
            moveCat();
        }
        glutTimerFunc(currentCatDelay, catTimer, resetIdentifier);
    }
}


// -----------------------------------------------------------------------------
// USER INPUT AND SYSTEM CALLBACKS
// -----------------------------------------------------------------------------

/**
 * @brief Handles all keyboard input from the user (ASCII characters).
 * @param key The ASCII value of the key pressed.
 * @param x_param Mouse X position (unused).
 * @param y_param Mouse Y position (unused).
 */
void keyboard(unsigned char key, int x_param, int y_param) {
    // State machine for keyboard input
    if (currentGameState == INTRO) {
        currentGameState = START_MENU;
        glutPostRedisplay();
        return;
    }
    if (currentGameState == START_MENU) {
        if (key == 13) { resetGame(); } // Enter key
        else if (key == 27) { exit(0); } // Escape key
        return;
    }
    if (currentGameState == GAME_OVER || currentGameState == GAME_WON_FINAL || currentGameState == GAME_WON_LEVEL) {
        if ((key == 'r' || key == 'R') && currentGameState != GAME_WON_FINAL) { resetGame(); }
        else if (key == 27) { exit(0); }
        return;
    }

    // General controls (available in multiple states)
    if (key == 'p' || key == 'P') {
         if (currentGameState == PLAYING) {
             currentGameState = PAUSED;
             timerActive = false;
             std::cout << "Game Paused.\n";
             glutPostRedisplay();
         } else if (currentGameState == PAUSED) {
             currentGameState = PLAYING;
             timerActive = true;
             lastTickTime = glutGet(GLUT_ELAPSED_TIME);
             resetIdentifier++;
             catTimer(resetIdentifier);
             std::cout << "Game Resumed.\n";
             glutPostRedisplay();
         }
         return;
    }
    if (key == 27) exit(0);

    // Gameplay controls (only active when playing)
    if (currentGameState != PLAYING) return;
    if (key == 'r' || key == 'R') {
        resetGame();
        return;
    }

    int nextX = playerX, nextY = playerY;
    switch (key) {
        case 'w': case 'W': nextY--; break;
        case 's': case 'S': nextY++; break;
        case 'a': case 'A': nextX--; break;
        case 'd': case 'D': nextX++; break;
        default: return;
    }
    processPlayerMove(nextX, nextY);
}

/**
 * @brief Handles special keyboard input (e.g., arrow keys).
 * @param key The GLUT constant for the special key pressed.
 * @param x Mouse X position (unused).
 * @param y Mouse Y position (unused).
 */
void specialKeyboard(int key, int x, int y) {
    if (currentGameState != PLAYING) return;

    int nextX = playerX, nextY = playerY;
    switch (key) {
        case GLUT_KEY_UP:    nextY--; break;
        case GLUT_KEY_DOWN:  nextY++; break;
        case GLUT_KEY_LEFT:  nextX--; break;
        case GLUT_KEY_RIGHT: nextX++; break;
        default: return;
    }
    processPlayerMove(nextX, nextY);
}

/**
 * @brief Centralized logic to handle player movement and collisions.
 * This is called by both keyboard() and specialKeyboard() to avoid code duplication.
 * @param nextX The proposed new X coordinate for the player.
 * @param nextY The proposed new Y coordinate for the player.
 */
void processPlayerMove(int nextX, int nextY) {
    // Handle tunnel wrapping
    if (nextY == TUNNEL_ROW_INDEX) {
        if (nextX < 0) nextX = COLS - 1;
        else if (nextX >= COLS) nextX = 0;
    }

    // Check if the next move is valid (a path tile)
    if (nextX >= 0 && nextX < COLS && nextY >= 0 && nextY < ROWS && maze[nextY][nextX] == TILE_PATH) {
        playerX = nextX;
        playerY = nextY;

        // Check for collision with cheese
        for (auto it = cheeseLocations.begin(); it != cheeseLocations.end(); ) {
            if (it->first == playerX && it->second == playerY) {
                it = cheeseLocations.erase(it);
                score++;
                std::cout << "Collected Cheese! Level Score: " << score << " (Current Total: " << totalScore + score << ")" << std::endl;

                // Increase cat speed as cheese is collected (non-linear scaling)
                if (!isCatSlowed && initialCheeseCount > 0) {
                    float progress = (float)score / initialCheeseCount;
                    currentCatDelay = MIN_CAT_DELAY_MS + (int)((INITIAL_CAT_DELAY_MS - MIN_CAT_DELAY_MS) * (1.0f - sqrt(progress)));
                    currentCatDelay = std::max(MIN_CAT_DELAY_MS, currentCatDelay);
                    normalCatDelayBeforeSlowdown = currentCatDelay;
                    std::cout << "Cat speed adjusted! New delay: " << currentCatDelay << "ms\n";
                }

                if (cheeseLocations.empty()) {
                    nextLevel();
                    return; // Exit to prevent further processing this frame
                }
                break;
            } else {
                ++it;
            }
        }

        // Check for collision with power-ups
        for (auto it = powerupLocations.begin(); it != powerupLocations.end(); ) {
            if(it->x == playerX && it->y == playerY) {
                if (it->type == TILE_SLOW_POWERUP && !isCatSlowed) {
                    std::cout << "Powerup Collected: Cat Slowdown!\n";
                    isCatSlowed = true;
                    catSlowDurationTimer = CAT_SLOW_DURATION_MS;
                    normalCatDelayBeforeSlowdown = currentCatDelay;
                    currentCatDelay = std::max(currentCatDelay, INITIAL_CAT_DELAY_MS + 100);
                    std::cout << "Cat slowed! Delay: " << currentCatDelay << "ms\n";
                    it = powerupLocations.erase(it);
                    break;
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
        glutPostRedisplay();
    }
}

/**
 * @brief Sets up initial OpenGL states like blending and anti-aliasing.
 */
void initOpenGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * @brief Idle callback for time-based updates, such as animations and timers.
 */
void idle() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = currentTime - lastTickTime;

    if (deltaTime > 0) {
        if (currentGameState == PLAYING || currentGameState == PAUSED) {
            // Animate power-up sparkle effect
            for(auto& p : powerupLocations) {
                p.sparklePhase += deltaTime * 0.005f;
                if (p.sparklePhase > TWICE_PI) p.sparklePhase -= TWICE_PI;
            }
            // Decrement the cat slowdown timer
            if(isCatSlowed && currentGameState == PLAYING) {
                catSlowDurationTimer -= deltaTime;
                if(catSlowDurationTimer <= 0) {
                    isCatSlowed = false;
                    // Restore cat speed to its normal value for the current progress
                    if(initialCheeseCount > 0) {
                        float progress = (float)score / initialCheeseCount;
                        currentCatDelay = MIN_CAT_DELAY_MS + (int)((INITIAL_CAT_DELAY_MS - MIN_CAT_DELAY_MS) * (1.0f - sqrt(progress)));
                        currentCatDelay = std::max(MIN_CAT_DELAY_MS, currentCatDelay);
                    } else {
                        currentCatDelay = normalCatDelayBeforeSlowdown;
                    }
                    std::cout << "Cat slowdown ended! Delay restored to: " << currentCatDelay << "ms\n";
                }
            }
        }
        lastTickTime = currentTime;
    }
    glutPostRedisplay();
}

/**
 * @brief Reshape callback that maintains the game's aspect ratio.
 * This function adds black bars (letterboxing/pillarboxing) if the window
 * is resized to a non-proportional shape.
 * @param w The new window width.
 * @param h The new window height.
 */
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float gameAspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float windowAspect = (float)w / (float)h;

    int newViewportW, newViewportH, newViewportX, newViewportY;

    if (windowAspect > gameAspect) {
        // Window is wider than the game (pillarbox)
        newViewportH = h;
        newViewportW = (int)(h * gameAspect);
        newViewportX = (w - newViewportW) / 2;
        newViewportY = 0;
    } else {
        // Window is taller than the game (letterbox)
        newViewportW = w;
        newViewportH = (int)(w / gameAspect);
        newViewportX = 0;
        newViewportY = (h - newViewportH) / 2;
    }
    glViewport(newViewportX, newViewportY, newViewportW, newViewportH);
    glutPostRedisplay();
}


// -----------------------------------------------------------------------------
// MAIN FUNCTION
// -----------------------------------------------------------------------------

int main(int argc, char** argv) {
    srand(time(0)); // Seed the random number generator
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA | GLUT_MULTISAMPLE);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Cat and Mouse - The Grand Chase!");

    // A timer to automatically transition from the intro screen to the start menu
    glutTimerFunc(3500, [](int val){
        if (currentGameState == INTRO) {
            currentGameState = START_MENU;
        }
    }, 0);

    // Initialize OpenGL, the maze, and register callbacks
    initOpenGL();
    initMaze(currentLevel);
    initLevelData();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard); // Register the handler for arrow keys
    glutIdleFunc(idle);
    lastTickTime = glutGet(GLUT_ELAPSED_TIME);

    // Check for and report MSAA (anti-aliasing) status
    GLint buffers; GLint samples;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
    glGetIntegerv(GL_SAMPLES, &samples);
    if (buffers > 0 && samples > 0) std::cout << "MSAA Enabled: Buffers=" << buffers << ", Samples=" << samples << std::endl;
    else std::cout << "MSAA Not Available/Enabled." << std::endl;

    // Print controls to the console for the user
    std::cout << "\n--- Controls ---\nWASD or Arrow Keys: Move\nP: Pause/Resume\nR: Reset Game\nESC: Quit\nEnter: Start Game\n----------------\n";

    glutMainLoop();
    return 0;
}
