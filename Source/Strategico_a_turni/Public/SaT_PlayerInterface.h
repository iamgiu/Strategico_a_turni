// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaT_GameInstance.h"
#include "SaT_PlayerInterface.generated.h"

enum class EUnitType : uint8
{
    Sniper,
    Brawler
};

enum class EPlayerType : uint8
{
    Human,
    AI
};

class ASaT_GameField;
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

    int32 PlayerNumber;

    EPlayerType PlayerType;

    TArray<AUnit*> Units;

    bool IsMyTurn = false;

    bool bIsActivePlayer = true;

    USaT_GameInstance* GameInstance;

    virtual void OnGameStart() {};

    virtual void OnTurn() {};

    virtual void OnWin() {};

    virtual void OnLose() {};

    void OnDraw();

    void PlaceUnit(int32 GridX, int32 GridY, EUnitType UnitType);

    virtual void SelectUnit(AUnit* Unit) {};

    virtual void OnCoinTossWin() {};

    virtual void OnCoinTossLose() {};
};
