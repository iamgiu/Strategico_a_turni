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
    ASaT_HumanPlayer();

    // Scene root component
    UPROPERTY(VisibleDefaultsOnly, Category = "Components")
    USceneComponent* DefaultSceneRoot;

    // Camera attaccata al giocatore
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    UCameraComponent* Camera;

    UPROPERTY(EditDefaultsOnly, Category = "Units")
    TSubclassOf<class ASniper> SniperClass;

    UPROPERTY(EditDefaultsOnly, Category = "Units")
    TSubclassOf<class ABrawler> BrawlerClass;

    // Chiamato ogni frame
    virtual void Tick(float DeltaTime) override;

    // Configura l'input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Metodi di interfaccia
    virtual void OnTurn() override;
    virtual void OnWin() override;
    virtual void OnLose() override;

    // Gestisce i click del mouse
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnClick();

    // Piazza una nuova unità sulla griglia
    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    void PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper);

    // Setter per GridManager
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetGridManager(AGridManager* InGridManager) { GridManager = InGridManager; }

    // Setter per CurrentPhase
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetCurrentPhase(EGamePhase InPhase) { CurrentPhase = InPhase; }

    // Setter per UnitsToPlace
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetUnitsToPlace(int32 InUnitsToPlace) { UnitsToPlace = InUnitsToPlace; }

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> UnitSelectionWidgetClass;

    UPROPERTY()
    UUserWidget* UnitSelectionWidget;

    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void OnUnitWidgetSniperSelected();

    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void OnUnitWidgetBrawlerSelected();

    // Aggiungere questa nuova funzione
    UFUNCTION(BlueprintCallable, Category = "Unit Selection")
    void ShowUnitSelectionWidget();

    // Track how many units the player has placed
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    int32 PlacedUnitsCount;

    // Reference to the EndTurnButton widget
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    UUserWidget* EndTurnWidget;

    // Reference to the EndTurnButton widget class
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> EndTurnButtonWidgetClass;

    // Function to handle the end turn button click
    UFUNCTION(BlueprintCallable, Category = "Game")
    void EndTurn();

    // Function to check if player has placed enough units
    bool HasPlacedAllUnits() const;

    // Show/hide end turn button
    void ShowEndTurnButton();

    USaT_GameInstance* GameInstance;

    // Currently selected unit
    UPROPERTY()
    AUnit* SelectedUnit;

    // Shows the possible movement range for a unit
    void ShowMovementRange(AUnit* Unit);

    // Try to move a unit to the specified grid location
    bool TryMoveUnit(AUnit* Unit, int32 TargetGridX, int32 TargetGridY);

    void CalculatePath(int32 StartX, int32 StartY, int32 EndX, int32 EndY);
    void ClearPath();

    void ClearAllHighlightsAndPaths();

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

    // Selection mode tracking
    UPROPERTY()
    bool bMoveMode;

    UPROPERTY()
    bool bAttackMode;

    UPROPERTY()
    TMap<AUnit*, bool> UnitAttackedThisTurn;

    UFUNCTION()
    void ShowAttackRange(AUnit* Unit);

    // Try to attack an enemy unit
    UFUNCTION()
    bool TryAttackUnit(AUnit* AttackingUnit, AUnit* TargetUnit);

    // Find unit at a specific grid position
    UFUNCTION()
    AUnit* FindUnitAtPosition(int32 GridX, int32 GridY);

    // Helper method to handle clicks during the playing phase
    void HandlePlayingPhaseClick(ATile* ClickedTile);

protected:
    virtual void BeginPlay() override;

    // Riferimento al GameInstance
    //UPROPERTY()
    //USaT_GameInstance* GameInstance;

    // Cella selezionata
    int32 SelectedGridX;
    int32 SelectedGridY;

    // Flag per sapere cosa stiamo facendo
    // 0 = Seleziona cella
    // 1 = Scegli tipo di unità (Sniper o Brawler)
    int32 SelectionState;

    // Ultima cella selezionata
    FVector LastSelectedCell;

    // Riferimento al GridManager
    UPROPERTY(BlueprintReadOnly, Category = "References")
    AGridManager* GridManager;

    // Fase di gioco corrente
    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    EGamePhase CurrentPhase;

    // Numero di unità da piazzare
    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    int32 UnitsToPlace;

    UPROPERTY()
    TArray<FVector2D> CurrentPath;

    bool bHasPlacedSniper;
    bool bHasPlacedBrawler;

    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    bool IsMyTurn;
};