#ifndef ENEMY_PATTERNS_H
#define ENEMY_PATTERNS_H

// Circular movement pattern
void circularMovement(int& x, int& y, int centerX, int centerY, float radius, float& angle);

// Zigzag movement pattern
void zigzagMovement(int& x, int& y, int startX, int& direction, float speed);

// Spiral movement pattern
void spiralMovement(int& x, int& y, int centerX, int centerY, float& radius, float& angle);

// Figure-eight movement pattern
void figureEightMovement(int& x, int& y, int centerX, int centerY, float& angle);

#endif // ENEMY_PATTERNS_H 