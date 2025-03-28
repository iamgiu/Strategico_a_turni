// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


// Enum for types of units
UENUM()
enum class EPieceUnit : uint8
{
	NONE,	
	SNIPER,
	BRAWLER,
};

// Enum for the players
UENUM(BlueprintType)
enum class EPlayerType : uint8
{
	Human UMETA(DisplayName = "Human"), //0
	AI UMETA(DisplayName = "AI"),       //1
};

// Enum to assign colors to various players
UENUM()
enum class EPieceColor : int8
{
	NONE = 0,	// No color assigned
	BLUE = 1,	// Blue color for player
	RED = -1,	// Red color  for Ai
	BOTH = 2	// Both color to take into account both pieces of both the colors
};

// Enum for the game phases
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	SETUP UMETA(DisplayName = "Setup"),
	PLAYING UMETA(DisplayName = "Playing"),
	GAMEOVER UMETA(DisplayName = "GameOver")
};

// Enum to check whether a unit is alive or dead
UENUM()
enum class EUnitStatus : int8
{
	ALIVE,
	DEAD
};

// Enum to check whether a cell is empty or not
UENUM(BlueprintType)
enum class ETileStatus : uint8
{
	EMPTY UMETA(DisplayName = "Empty"),
	OCCUPIED UMETA(DisplayName = "Occupied"),
};

// Enum to identify the difficulty of the game
UENUM(BlueprintType)
enum class EAIDifficulty : uint8
{
	EASY UMETA(DisplayName = "Easy"),
	HARD UMETA(DisplayName = "Hard")
};

class STRATEGICO_A_TURNI_API SaT_Enums
{
public:

	static const int32 NOT_ASSIGNED = -1;

	SaT_Enums();
	~SaT_Enums();
};
