# Strategic Turn-Based Game

## Project Overview
This is a turn-based strategic game developed as a final project for the PAA course (Academic Year 2024/2025) at the university. The game is implemented in Unreal Engine 5.4 using C++ and follows a detailed set of specifications for a 1vs1 (Human Player vs AI) grid-based strategy game.

## Game Specifications

### Grid and Game Board
- 25x25 grid-based game board
- Square cells slightly larger than game units
- Top-down 2D view
- Configurable obstacle percentage on the grid

### Game Units

#### Sniper
- Movement: Max 3 cells
- Attack Type: Long-range attack
- Attack Range: Max 10 cells
- Damage: 4-8 HP
- Health: 20 HP

#### Brawler
- Movement: Max 6 cells
- Attack Type: Close-range attack
- Attack Range: 1 cell
- Damage: 1-6 HP
- Health: 40 HP

### Game Phases
1. Both players are assigned 2 game units (one of each type)
2. Coin toss to determine first player
3. Players alternately place their units
4. Winner of the coin toss starts the first turn
5. Players take turns until the game ends

### Turn Mechanics
- Select a unit
- Options:
  - Move and then attack (if attack range permits)
  - Attack without moving
  - Move without attacking

### Win Conditions
- Defeat all opponent's units
- Game ends when one player loses all units

## Project Requirements Implemented
- ✅ Compiles correctly with well-commented and structured C++ code
- ✅ Correct initial grid visualization
- ✅ Game unit placement mechanism
- ✅ AI with random actions (movement and attack)
- ✅ Turn-based gameplay with win conditions
- ✅ Game state interface (current turn, unit health)
- ✅ Movement range suggestions
- ✅ Counterattack damage mechanism
- ✅ Move history logging
- ✅ AI with optimized movement algorithms (A*)

## Screenshot

# Game Start
![start](https://github.com/user-attachments/assets/dd34540f-7be6-4894-bd54-95a06d6c1bda)

# CoinFlip - Hard Mode
![image](https://github.com/user-attachments/assets/d4feaacc-0c9d-4569-bd58-7971eeecb101)

# SetupPhase - Hard Mode
![image](https://github.com/user-attachments/assets/b270fa03-cb30-48b0-98a6-2b0ba5f8788e)

# Movemente And Attack Widget - Hard Mode
![image](https://github.com/user-attachments/assets/2343d584-a7cd-4e74-a60d-a040c8a230cc)

# Movemente Range - Hard Mode
![image](https://github.com/user-attachments/assets/6e69fea2-7276-4def-9c92-821332c4406a)

# Game Over - Hard Mode
![image](https://github.com/user-attachments/assets/86b6362f-fc9d-41a6-a226-30a0647ce9cb)

# CoinFlip - Easy Mode
![image](https://github.com/user-attachments/assets/5ef0fba1-8f52-45f2-a389-548a061f8a4a)
