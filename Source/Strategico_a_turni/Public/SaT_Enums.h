// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


// Enum per le tipologie di unità
UENUM()
enum class EPieceUnit : uint8
{
	NONE,	
	SNIPER,
	BRAWLER,
};

UENUM(BlueprintType)
enum class EPlayerType : uint8
{
	Human UMETA(DisplayName = "Human"),
	AI UMETA(DisplayName = "AI")
};

UENUM()
enum class EPieceColor : int8
{
	NONE = 0,	// No color assigned
	BLUE = 1,	// Blue color for player
	RED = -1,	// Red color  for Ai
	BOTH = 2	// Both color to take into account both pieces of both the colors
};

// Enum per le fasi di gioco
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	SETUP UMETA(DisplayName = "Setup"),
	PLAYING UMETA(DisplayName = "Playing"),
	GAMEOVER UMETA(DisplayName = "GameOver")
};

// ENum per verificare se una unitò è viva oppure morta
UENUM()
enum class EUnitStatus : int8
{
	ALIVE,
	DEAD
};

UENUM(BlueprintType)
enum class ETileStatus : uint8
{
	EMPTY UMETA(DisplayName = "Empty"),
	OCCUPIED UMETA(DisplayName = "Occupied"),
};

class STRATEGICO_A_TURNI_API SaT_Enums
{
public:

	static const int32 NOT_ASSIGNED = -1;

	SaT_Enums();
	~SaT_Enums();
};
