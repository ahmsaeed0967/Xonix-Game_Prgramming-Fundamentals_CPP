# 🕹️ Xonix Arcade Game — C++ & SFML

A fully playable recreation of the classic Xonix arcade game, built as my **Programming Fundamentals** semester project at FAST NUCES. No OOP, no game engine — just structured C++, procedural logic, and a 2D array as the game world.

---

## 🎮 What Is Xonix?

The objective is straightforward: carve out territory by drawing closed paths across the grid while avoiding enemies. Leave the safe border zone, trace a trail through open space, and return to safety — the enclosed area locks in as captured territory. Take a hit to your trail from an enemy, and it's game over.

---

## 🚀 Features

- **Three difficulty levels** — Easy (2 enemies), Medium (4), Hard (6) — plus a **Continuous mode** that spawns two new enemies every 20 seconds and increases their speed over time.
- **Two-player mode** — both players share the same grid, with separate trails and scores. Crossing the opponent's trail ends the game.
- **Three enemy movement patterns** — enemies start with linear (bouncing) movement, then switch to zigzag and circular paths after 30 seconds of gameplay.
- **Scoring with bonus multipliers** — capturing large areas in one move triggers a 2× or 4× point multiplier. Power-ups unlock as your score grows and can be activated mid-game to temporarily freeze enemy movement.
- **Persistent scoreboard** — top 5 scores and their times are saved to `scoreboard.txt` using file I/O and loaded on startup.
- **Pause menu** — pause mid-game to resume, return to the main menu, or exit.
- **Sound effects** — audio feedback for movement, area completion, and game over.

---

## 📐 Core Logic (How It Actually Works)

### 1. The Grid
The entire game world is a `25 × 40` integer array. Each cell holds one of five values:

| Value    | Meaning                     |
|----------|-----------------------------|
| `0`      | Open/uncaptured space       |
| `1`      | Wall / captured territory   |
| `2`      | Player 1's active trail     |
| `3`      | Player 2's active trail     |
| `-1`     | Temporary flood-fill marker |

### 2. Territory Capture — Flood Fill
When a player returns to a wall tile, the game runs a recursive flood fill (`drop()`) starting from each enemy's current position. This marks all open space reachable by enemies as `-1`. After the fill:
- Cells still at `0` (not reachable by enemies) get converted to `1` — captured.
- Cells marked `-1` get reset to `0` — left open.
- The player's trail cells (`2` or `3`) also convert to walls.

This is what makes the area-capture mechanic work correctly: enemies always stay in uncaptured space.

### 3. Enemy Movement
Three movement patterns are implemented as separate functions and can change mid-game:

- **Linear** — enemies move diagonally and reflect off walls by inverting `dx` or `dy` on collision.
- **Zigzag** — horizontal movement combined with alternating vertical steps; direction toggles via a step counter.
- **Circular** — position is calculated using `cos(angle)` and `sin(angle)`. On collision, the code tries the opposite direction, then four perpendicular angles, then a random angle as a fallback.

Enemy speed increases by `0.5` every 20 seconds of gameplay.

### 4. Scoring
Captured tiles are counted after each successful area close. If the count exceeds the current bonus threshold, a multiplier applies (2× normally, 4× after 5 bonus captures). The threshold drops from 10 to 5 tiles after 3 consecutive bonus captures, rewarding sustained aggressive play.

---

## 🛠️ Tech Stack

- **Language:** C++ — arrays, loops, functions, recursion, file streams
- **Library:** [SFML](https://www.sfml-dev.org/) — window management, sprite rendering, audio

---

## 📂 Project Structure

```text
XonixGame/
├── main.cpp                # Full game logic and rendering loop
├── images/
│   ├── tiles.png           # Wall, trail, and player sprites
│   ├── enemy.png           # Enemy sprite
│   └── gameover.png        # Game over overlay
├── LemonMilk.otf           # Game font
├── playermoved.wav         # Sound: player movement
├── completedmove.wav       # Sound: area captured
├── gameover.wav            # Sound: game over
└── scoreboard.txt          # Auto-generated: persisted top 5 scores
```
## 🎮 Game Controls

| Action | Player 1 (🕹️ Solo / Left) | Player 2 (🕹️ Co-Op / Right) |
| :--- | :---: | :---: |
| **Move / Navigate** | <kbd>▲</kbd> <kbd>▼</kbd> <kbd>◄</kbd> <kbd>►</kbd> (Arrow Keys) | <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> |
| **Activate Power-Up** | <kbd>P</kbd> | <kbd>O</kbd> |
| **Pause Menu** | <kbd>Escape</kbd> | — |

## ⚙️ Build & Run (Linux & Ubuntu Setup)

This project uses CMake configuration for reliable linking. Follow these simple steps in your terminal to build and run the game:

###  ✅ Install Dependencies, then compile and run
```bash
# Update packages and install compiler essentials + SFML development kits
sudo apt update
sudo apt install cmake build-essential libsfml-dev

### - Compile & Execute via CMake

1️⃣ Generate and enter the build directory:
mkdir build
cd build

2️⃣ Configure the project configurations and compile the files:
 cmake ..
 make

3️⃣ Execute the generated game engine binary directly from your terminal:
./XonixGame
```
## 📌 Context 

> 🏫 **Institutional Framework:** This repository features a semester project developed for the **Programming Fundamentals (PF)** course at **FAST NUCES, Islamabad Campus**.

---

### 📐 1. What Was Provided (The Boilerplate Skeleton)
To ensure equal footing, the university provided us with a basic architectural starter kit that included:
- **The Core Grid:** A raw `25 × 40` structured integer matrix managing fundamental pixel coordinate bounds.
- **The Engine Loop:** A lightweight, non-modular game loop rendering minimal SFML window bindings.
- **Elementary Physics:** A basic, single-mode linear bouncing vector for standard coordinate boundary collision.

---



