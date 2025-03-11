// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Sat_GameInstance.h"
#include "Sat_GameMode.h"
#include "SaT_PlayerInterface.generated.h"

enum class EUnitColor : int8;

class AGridManager;
class AUnit;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USaT_PlayerInterface : public UInterface
{
	GENERATED_BODY()
};

class STRATEGICO_A_TURNI_API ISaT_PlayerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	static constexpr int8 TIMER_NONE = 1;

	int32 PlayerNumber;
	EPieceColor Color;
	TArray<TArray<std::pair<int8, int8>>> AttackableTiles;
	bool IsMyTurn = false;
	bool bIsActivePlayer = true;

	USaT_GameInstance* GameInstance;

	virtual void OnTurn() {};
	virtual void OnWin() {};
	virtual void OnLose() {};
	//void OnDraw();
};