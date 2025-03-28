// Fill out your copyright notice in the Description page of Project Settings.

/*
 * AI player implementation for the strategy game
 * Handles both random and strategic AI behavior based on difficulty setting
 */

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

    // -----------------
    // Core Game Functions
    // -----------------

    // Sets default values for this pawn's properties
    ASaT_RandomPlayer();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // ISaT_PlayerInterface implementation
    virtual void OnTurn() override;
    virtual void OnWin() override;
    virtual void OnLose() override;
    virtual void OnDraw() override;

    // -----------------
    // Units Configuration
    // -----------------

    // Unit class references
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Units")
    TSubclassOf<class ASniper> SniperClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Units")
    TSubclassOf<class ABrawler> BrawlerClass;

    // Track unit placement count
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
    int32 PlacedUnitsCount;

    // -----------------
    // AI Turn Management
    // -----------------

    // End the AI's turn
    void EndTurn();

    // Place a random unit during setup phase
    void PlaceRandomUnit();

    // Find a random empty cell on the grid
    bool FindRandomEmptyCell(int32& OutGridX, int32& OutGridY);

    // -----------------
    // AI Unit Processing
    // -----------------

    // Find all AI controlled units
    void FindAllAIUnits();

    // Process the next AI unit in sequence
    void ProcessNextAIUnit();

    // Process actions for a specific AI unit
    void ProcessUnitActions(AUnit* Unit);

    // -----------------
    // AI Strategy - Basic
    // -----------------

    // Process unit actions with random behavior (Easy mode)
    void ProcessUnitActionsRandom(AUnit* Unit);

    // Find a random target to attack
    AUnit* FindRandomAttackTarget(AUnit* AIUnit);

    // Move the unit to a random valid position
    bool MoveRandomly(AUnit* AIUnit);

    // -----------------
    // AI Strategy - Advanced
    // -----------------

    // Process unit actions with strategic behavior (Hard mode)
    void ProcessUnitActionsStrategic(AUnit* Unit);

    // Find a target to attack using strategic prioritization
    AUnit* FindAttackTarget(AUnit* AIUnit);

    // Move toward closest player unit using pathfinding
    void MoveTowardPlayerUnit(AUnit* AIUnit);

    // Find the closest player unit to the given AI unit
    AUnit* FindClosestPlayerUnit(AUnit* AIUnit);

    // -----------------
    // Pathfinding Utilities
    // -----------------

    // Reconstruct path from A* search and get the best move within range
    FVector2D ReconstructPath(const TMap<FVector2D, FVector2D>& CameFrom,
        FVector2D Current, FVector2D Start, int32 MaxSteps);

    // Calculate Manhattan distance between two points
    int32 ManhattanDistance(const FVector2D& A, const FVector2D& B);

    // Check if a path is clear between two points
    bool IsPathClear(int32 StartX, int32 StartY, int32 EndX, int32 EndY);

    // -----------------
    // References & State
    // -----------------

    // Reference to grid manager
    UPROPERTY()
    class AGridManager* GridManager;

    // Reference to game instance
    UPROPERTY()
    class USaT_GameInstance* GameInstance;

    // List of AI units for processing
    UPROPERTY()
    TArray<AUnit*> AIUnits;

    // Index of the current unit being processed
    UPROPERTY()
    int32 CurrentUnitIndex;

};