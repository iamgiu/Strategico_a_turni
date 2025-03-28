// Fill out your copyright notice in the Description page of Project Settings.

/*
 * Interface class for player implementations in the strategy game
 * Used by both human and AI player classes
 * Interface that defines required functionality for all player types
 * Includes common properties and virtual methods players must implement
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Sat_GameInstance.h"
#include "SaT_PlayerInterface.generated.h"

enum class EUnitColor : int8;

class AGridManager;
class AUnit;

UINTERFACE(MinimalAPI)
class USaT_PlayerInterface : public UInterface
{
	GENERATED_BODY()
};

class STRATEGICO_A_TURNI_API ISaT_PlayerInterface
{
	GENERATED_BODY()


public:

	// Constant to represent no timer is active
	static constexpr int8 TIMER_NONE = 1;

	// -----------------
	// Player Properties
	// -----------------

	// Index of this player in the Players array (0=Human, 1=AI)
	int32 PlayerNumber;

	// Color assigned to this player's pieces
	EPieceColor Color;

	// Grid positions that can be attacked by this player's units
	TArray<TArray<std::pair<int8, int8>>> AttackableTiles;

	// Whether it's currently this player's turn
	bool IsMyTurn = false;

	// Whether this player is active in the game
	bool bIsActivePlayer = true;

	// Reference to the game instance
	USaT_GameInstance* GameInstance;

	// -----------------
	// Interface Methods
	// -----------------

	// Called when it becomes this player's turn
	virtual void OnTurn() {};

	// Called when this player wins the game
	virtual void OnWin() {};

	// Called when this player loses the game
	virtual void OnLose() {};

	// Called when the game ends in a draw (must be implemented)
	virtual void OnDraw() = 0;

};