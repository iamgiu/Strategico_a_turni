// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "SaT_PlayerInterface.h"
#include "SaT_GameInstance.h"
#include "SaT_Enums.h"
#include "Blueprint/UserWidget.h"
#include "SaT_HumanPlayer.generated.h"

// Forward declarations
class AGridManager;
class AUnit;
class ATile;

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_HumanPlayer : public APawn, public ISaT_PlayerInterface
{
    GENERATED_BODY()

public:

    // Constructor 
    ASaT_HumanPlayer();

    // -----------------
    // Components
    // -----------------

    // Scene root component
    UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    USceneComponent* DefaultSceneRoot;

    // Camera attached to player
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    UCameraComponent* Camera;

    // -----------------
    // Core Game Functions
    // -----------------

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Configure input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Interface methods
    virtual void OnTurn() override;
    virtual void OnWin() override;
    virtual void OnLose() override;
    virtual void OnDraw() override;

    // -----------------
    // Units Configuration
    // -----------------

    // Unit class references
    UPROPERTY(EditDefaultsOnly, Category = "Units")
    TSubclassOf<class ASniper> SniperClass;

    UPROPERTY(EditDefaultsOnly, Category = "Units")
    TSubclassOf<class ABrawler> BrawlerClass;

    // Place a new unit on the grid
    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    void PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper);

    // Track how many units the player has placed
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    int32 PlacedUnitsCount;

    // Number of units to place
    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    int32 UnitsToPlace;

    // Function to check if player has placed enough units
    bool HasPlacedAllUnits() const;

    // Unit placement tracking flags
    bool bHasPlacedSniper;
    bool bHasPlacedBrawler;

    // -----------------
    // Input Handling
    // -----------------

    // Handle mouse clicks
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnClick();

    // Helper method to handle clicks during the playing phase
    void HandlePlayingPhaseClick(ATile* ClickedTile);

    // -----------------
    // Player State
    // -----------------

    // Current game phase
    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    EGamePhase CurrentPhase;

    // Turn status tracking
    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    bool IsMyTurn;

    // -----------------
    // Unit Selection
    // -----------------

    // Currently selected unit
    UPROPERTY()
    AUnit* SelectedUnit;

    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void DeselectCurrentUnit();

    // Unit selection widget class
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> UnitSelectionWidgetClass;

    // Reference to unit selection widget instance
    UPROPERTY()
    UUserWidget* UnitSelectionWidget;

    // Show unit selection widget
    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void ShowUnitSelectionWidget();

    // Unit selection handlers
    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void OnUnitWidgetSniperSelected();

    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void OnUnitWidgetBrawlerSelected();

    // -----------------
    // UI Elements
    // -----------------

    // Reference to the EndTurnButton widget
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    UUserWidget* EndTurnWidget;

    // Reference to the EndTurnButton widget class
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> EndTurnButtonWidgetClass;

    // Function to handle the end turn button click
    UFUNCTION(BlueprintCallable, Category = "Game")
    void EndTurn();

    // Show/hide end turn button
    void ShowEndTurnButton();

    // Reference to the Movement and Attack widget class
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> MovementAndAttackWidgetClass;

    // Reference to the Movement and Attack widget instance
    UPROPERTY()
    UUserWidget* MovementAndAttackWidget;

    // Function to show the Movement and Attack widget
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowMovementAndAttackWidget(AUnit* UnitToShow);

    // Functions to handle button clicks from the widget
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnMoveButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnAttackButtonClicked();

    // -----------------
    // Movement & Combat
    // -----------------

    // Selection mode tracking
    UPROPERTY()
    bool bMoveMode;

    UPROPERTY()
    bool bAttackMode;

    // Shows the possible movement range for a unit
    void ShowMovementRange(AUnit* Unit);

    // Try to move a unit to the specified grid location
    bool TryMoveUnit(AUnit* Unit, int32 TargetGridX, int32 TargetGridY);

    // Path calculation functions
    void CalculatePath(int32 StartX, int32 StartY, int32 EndX, int32 EndY);
    void ClearPath();

    // Clear all highlights and paths
    void ClearAllHighlightsAndPaths();

    // Path tracking
    UPROPERTY()
    TArray<FVector2D> CurrentPath;

    // Track units that have attacked this turn
    UPROPERTY()
    TMap<AUnit*, bool> UnitAttackedThisTurn;

    // Show attack range for a unit
    UFUNCTION()
    void ShowAttackRange(AUnit* Unit);

    // Try to attack an enemy unit
    UFUNCTION()
    bool TryAttackUnit(AUnit* AttackingUnit, AUnit* TargetUnit);

    // -----------------
    // References & Services
    // -----------------

    // Game instance reference
    USaT_GameInstance* GameInstance;

    // Find unit at a specific grid position
    UFUNCTION()
    AUnit* FindUnitAtPosition(int32 GridX, int32 GridY);

    // Setter for GridManager
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetGridManager(AGridManager* InGridManager) { GridManager = InGridManager; }

    // Setter for CurrentPhase
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetCurrentPhase(EGamePhase InPhase) { CurrentPhase = InPhase; }

    // Setter for UnitsToPlace
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetUnitsToPlace(int32 InUnitsToPlace) { UnitsToPlace = InUnitsToPlace; }

protected:
    virtual void BeginPlay() override;

    // Selected cell
    int32 SelectedGridX;
    int32 SelectedGridY;

    // Selection state flag
    // 0 = Select cell
    // 1 = Choose unit type (Sniper or Brawler)
    int32 SelectionState;

    // Last selected cell
    FVector LastSelectedCell;

    // Reference to the GridManager
    UPROPERTY(BlueprintReadOnly, Category = "References")
    AGridManager* GridManager;
};