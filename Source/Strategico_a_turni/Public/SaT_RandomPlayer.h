// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SaT_PlayerInterface.h"
#include "SaT_RandomPlayer.generated.h"

class USaT_GameInstance;
class AGridManager;
class AUnit;

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_RandomPlayer : public APawn, public ISaT_PlayerInterface
{
    GENERATED_BODY()

public:
    // Sets default values for this pawn's properties
    ASaT_RandomPlayer();

    // Existing properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Units")
    TSubclassOf<class ASniper> SniperClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Units")
    TSubclassOf<class ABrawler> BrawlerClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    int32 PlacedUnitsCount;

    // References
    UPROPERTY()
    class AGridManager* GridManager;

    UPROPERTY()
    class USaT_GameInstance* GameInstance;

    // Unit-related properties
    UPROPERTY()
    TArray<AUnit*> AIUnits;

    UPROPERTY()
    int32 CurrentUnitIndex;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // ISaT_PlayerInterface
    virtual void OnTurn() override;
    virtual void OnWin() override;
    virtual void OnLose() override;

    // Game flow methods
    void EndTurn();
    void PlaceRandomUnit();
    bool FindRandomEmptyCell(int32& OutGridX, int32& OutGridY);

    // AI behavior methods
    void FindAllAIUnits();
    void ProcessNextAIUnit();
    void ProcessUnitActions(AUnit* Unit);
    AUnit* FindAttackTarget(AUnit* AIUnit);
    void MoveTowardPlayerUnit(AUnit* AIUnit);
    AUnit* FindClosestPlayerUnit(AUnit* AIUnit);
    FVector2D ReconstructPath(const TMap<FVector2D, FVector2D>& CameFrom,
        FVector2D Current, FVector2D Start, int32 MaxSteps);


    // Process unit actions with strategic behavior (Hard mode)
    void ProcessUnitActionsStrategic(AUnit* Unit);

    // Calculate Manhattan distance between two points
    int32 ManhattanDistance(const FVector2D& A, const FVector2D& B);

    // Methods for different AI difficulties
    void ProcessUnitActionsRandom(AUnit* Unit);
    AUnit* FindRandomAttackTarget(AUnit* AIUnit);
    void MoveRandomly(AUnit* AIUnit);

    bool IsPathClear(int32 StartX, int32 StartY, int32 EndX, int32 EndY);
};