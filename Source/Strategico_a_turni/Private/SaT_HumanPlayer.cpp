// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_HumanPlayer.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GridManager.h"
#include <Kismet/GameplayStatics.h>
#include "Sniper.h"
#include "Brawler.h"
#include "Unit.h"
#include "SaT_PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "SaT_GameInstance.h"
#include "UObject/ConstructorHelpers.h"

ASaT_HumanPlayer::ASaT_HumanPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create the camera directly as root
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    SetRootComponent(Camera);

    // Configure camera for orthographic view
    Camera->SetRelativeRotation(FRotator(-90, 0, 0)); // Look downward
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;

    // Calculate OrthoWidth based on grid size
    // Assuming TileSize of 100 and a 25x25 grid
    float GridSize = 25 * 100.0f;
    Camera->OrthoWidth = GridSize * 2.1f; // Some margin around the grid

    // Camera height - high enough to see the entire field
    float CameraHeight = 2000.0f;
    Camera->SetRelativeLocation(FVector(GridSize / 2, GridSize / 2, CameraHeight));

    // Adding hard-coded defaults for classes - these will be overridden by BP values if set
    static ConstructorHelpers::FClassFinder<ASniper> DefaultSniperClass(TEXT("/Game/Blueprints/BP_Sniper"));
    if (DefaultSniperClass.Succeeded())
    {
        SniperClass = DefaultSniperClass.Class;
    }

    static ConstructorHelpers::FClassFinder<ABrawler> DefaultBrawlerClass(TEXT("/Game/Blueprints/BP_Brawler"));
    if (DefaultBrawlerClass.Succeeded())
    {
        BrawlerClass = DefaultBrawlerClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultUnitSelectionClass(TEXT("/Game/UI/WBP_UnitSelection"));
    if (DefaultUnitSelectionClass.Succeeded())
    {
        UnitSelectionWidgetClass = DefaultUnitSelectionClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultEndTurnClass(TEXT("/Game/UI/WBP_EndTurn"));
    if (DefaultEndTurnClass.Succeeded())
    {
        EndTurnButtonWidgetClass = DefaultEndTurnClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultMovementAndAttackClass(TEXT("/Game/UI/WBP_MovementAndAttack"));
    if (DefaultMovementAndAttackClass.Succeeded())
    {
        MovementAndAttackWidgetClass = DefaultMovementAndAttackClass.Class;
    }
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Base initialization
    SelectionState = 0;
    CurrentPhase = EGamePhase::SETUP;
    PlacedUnitsCount = 0; // Initialize placement count
    UnitsToPlace = 2;

    bHasPlacedSniper = false;
    bHasPlacedBrawler = false;

    // Initialize mode flags
    bMoveMode = false;
    bAttackMode = false;

    ShowEndTurnButton();

    // Initialize MovementAndAttackWidget reference
    MovementAndAttackWidget = nullptr;

    if (!SniperClass)
    {
        UE_LOG(LogTemp, Error, TEXT("SniperClass is NULL! Falling back to base class."));
        SniperClass = ASniper::StaticClass();
    }

    if (!BrawlerClass)
    {
        UE_LOG(LogTemp, Error, TEXT("BrawlerClass is NULL! Falling back to base class."));
        BrawlerClass = ABrawler::StaticClass();
    }

    // Find widget class reference if not set in editor
    if (!MovementAndAttackWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MovementAndAttackWidgetClass is NULL! Widget actions will not work."));
    }

    // Find the GridManager
    TArray<AActor*> FoundGrids;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
    if (FoundGrids.Num() > 0)
    {
        GridManager = Cast<AGridManager>(FoundGrids[0]);
        if (GridManager)
        {
            UE_LOG(LogTemp, Display, TEXT("GridManager found and assigned"));

            // Position camera above grid center
            float GridSize = GridManager->Size * GridManager->TileSize;
            float CameraHeight = 2000.0f;

            // Position camera at grid center
            SetActorLocation(FVector(GridSize / 2, GridSize / 2, CameraHeight));

            UE_LOG(LogTemp, Warning, TEXT("Camera positioned at grid center: X=%f, Y=%f, Z=%f"),
                GridSize / 2, GridSize / 2, CameraHeight);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GridManager not found in scene!"));
        }
    }

    // Validate class references

    if (EndTurnButtonWidgetClass)
    {
        UE_LOG(LogTemp, Display, TEXT("EndTurnButtonWidgetClass is properly set"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("EndTurnButtonWidgetClass is NOT set! Configure this property in Blueprint"));
    }

    if (UnitSelectionWidgetClass)
    {
        UE_LOG(LogTemp, Display, TEXT("UnitSelectionWidgetClass is properly set"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UnitSelectionWidgetClass is NOT set! Configure this property in Blueprint"));
    }

    if (SniperClass)
    {
        UE_LOG(LogTemp, Display, TEXT("SniperClass is properly set"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SniperClass is NOT set! Configure this property in Blueprint"));
    }

    if (BrawlerClass)
    {
        UE_LOG(LogTemp, Display, TEXT("BrawlerClass is properly set"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("BrawlerClass is NOT set! Configure this property in Blueprint"));
    }

    // Get GameInstance to check turn status
    GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        // Set IsMyTurn based on GameInstance's turn state
        IsMyTurn = GameInstance->bIsPlayerTurn;
        UE_LOG(LogTemp, Warning, TEXT("Human Player's turn initialized: %s"),
            IsMyTurn ? TEXT("TRUE") : TEXT("FALSE"));
    }
}

void ASaT_HumanPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASaT_HumanPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Input handled by controller
}

void ASaT_HumanPlayer::OnTurn()
{
    // Make sure we have a reference to the GameInstance
    if (!GameInstance)
    {
        GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    }

    // Very important - check if it's really our turn
    if (GameInstance && !GameInstance->bIsPlayerTurn)
    {
        UE_LOG(LogTemp, Error, TEXT("Human: OnTurn called but GameInstance says it's AI's turn! Ignoring."));
        IsMyTurn = false; // Ensure flag is correct
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Human Player Turn Started - Verification PASSED"));

    // Explicitly set this player's turn flag to true
    IsMyTurn = true;

    // Explicitly set this player's turn flag to true
    IsMyTurn = true;
    UE_LOG(LogTemp, Warning, TEXT("Human: IsMyTurn set to TRUE"));

    // Explicitly set this player's turn flag to true
    IsMyTurn = true;
    UE_LOG(LogTemp, Warning, TEXT("Human: IsMyTurn set to TRUE"));

    if (GameInstance)
    {
        // Update the class member directly instead of creating a local variable
        CurrentPhase = GameInstance->GetGamePhase();

        // Log the current phase for debugging
        UE_LOG(LogTemp, Warning, TEXT("Human Player - Current game phase: %s"),
            CurrentPhase == EGamePhase::SETUP ? TEXT("SETUP") :
            CurrentPhase == EGamePhase::PLAYING ? TEXT("PLAYING") : TEXT("GAMEOVER"));

        // Log the number of units placed by each player
        UE_LOG(LogTemp, Warning, TEXT("Units placed - Human: %d, AI: %d"),
            GameInstance->HumanUnitsPlaced, GameInstance->AIUnitsPlaced);

        if (CurrentPhase == EGamePhase::SETUP)
        {

            if (PlacedUnitsCount >= UnitsToPlace)
            {
                UE_LOG(LogTemp, Warning, TEXT("Already placed all units (%d/%d). End your turn."),
                    PlacedUnitsCount, UnitsToPlace);
                return;
            }

            // Update our placed units count to match the GameInstance
            PlacedUnitsCount = GameInstance->HumanUnitsPlaced;
            UnitsToPlace = 2; // Ensure this is set correctly

            // In setup phase, check if all units are placed
            if (HasPlacedAllUnits())
            {
                UE_LOG(LogTemp, Warning, TEXT("Human player has already placed all units!"));
                // End turn automatically
                EndTurn();
            }
            else
            {
                // Can place more units
                UE_LOG(LogTemp, Warning, TEXT("Human player can place more units (%d/%d placed)"),
                    PlacedUnitsCount, UnitsToPlace);

                // Make sure the end turn button is visible
                ShowEndTurnButton();
            }
        }
        else if (CurrentPhase == EGamePhase::PLAYING)
        {
            UE_LOG(LogTemp, Warning, TEXT("Human player turn in PLAYING phase"));

            // Make sure the end turn button is visible for the playing phase
            ShowEndTurnButton();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in Human Player OnTurn!"));
    }
}

void ASaT_HumanPlayer::OnWin()
{
    // Implementation for when the player wins
    UE_LOG(LogTemp, Warning, TEXT("Human Player has won!"));

    // You can add additional win logic here, like showing a victory UI
}

void ASaT_HumanPlayer::OnLose()
{
    // Implementation for when the player loses
    UE_LOG(LogTemp, Warning, TEXT("Human Player has lost!"));

    // You can add additional loss logic here, like showing a defeat UI
}



// Implement HasPlacedAllUnits
bool ASaT_HumanPlayer::HasPlacedAllUnits() const
{
    return PlacedUnitsCount >= UnitsToPlace;
}

void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{

    if (PlacedUnitsCount >= UnitsToPlace)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot place more units - already placed maximum (%d/%d)"),
            PlacedUnitsCount, UnitsToPlace);
        return;
    }

    // Add detailed logs
    UE_LOG(LogTemp, Warning, TEXT("PlaceUnit called with coordinates: X=%d, Y=%d"), GridX, GridY);

    // Check class references
    if (bIsSniper && !SniperClass)
    {
        UE_LOG(LogTemp, Error, TEXT("SniperClass is not set! Cannot place Sniper"));
        return;
    }

    if (!bIsSniper && !BrawlerClass)
    {
        UE_LOG(LogTemp, Error, TEXT("BrawlerClass is not set! Cannot place Brawler"));
        return;
    }

    // Check coordinates are within grid
    if (GridX < 0 || GridX >= 25 || GridY < 0 || GridY >= 25)
    {
        UE_LOG(LogTemp, Error, TEXT("Coordinates outside grid!"));
        return;
    }

    if (bIsSniper)
        bHasPlacedSniper = true;
    else
        bHasPlacedBrawler = true;

    // Calculate 3D position from grid
    FVector SpawnLocation;
    if (GridManager)
    {
        SpawnLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);
        UE_LOG(LogTemp, Warning, TEXT("Spawn location calculated: X=%f, Y=%f, Z=%f"),
            SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }
    else
    {
        // Fallback if GridManager is invalid
        SpawnLocation = FVector(GridX * 100.0f, GridY * 100.0f, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("GridManager invalid! Using fallback: X=%f, Y=%f, Z=%f"),
            SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }

    // Create appropriate unit
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AUnit* PlacedUnit = nullptr;

    if (bIsSniper)
    {
        PlacedUnit = GetWorld()->SpawnActor<ASniper>(SniperClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Display, TEXT("Sniper placed at %d, %d"), GridX, GridY);
    }
    else
    {
        PlacedUnit = GetWorld()->SpawnActor<ABrawler>(BrawlerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Display, TEXT("Brawler placed at %d, %d"), GridX, GridY);
    }

    // Configure unit
    if (PlacedUnit)
    {
        PlacedUnit->GridX = GridX;
        PlacedUnit->GridY = GridY;
        PlacedUnit->bIsPlayerUnit = true;

        // Update cell state in GridManager
        if (GridManager)
        {
            GridManager->OccupyCell(GridX, GridY, PlacedUnit);
        }

        // Increment placement count
        PlacedUnitsCount++;
        UE_LOG(LogTemp, Warning, TEXT("Human player has placed %d/%d units"), PlacedUnitsCount, UnitsToPlace);

        // Update the GameInstance to track human units placed
        if (GameInstance)
        {
            GameInstance->HumanUnitsPlaced = PlacedUnitsCount;
        }
    }
}


void ASaT_HumanPlayer::OnClick()
{
    // Check if it's this player's turn
    if (!IsMyTurn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not your turn! Cannot interact with the game."));
        return;
    }

    // Get current game phase
    if (GameInstance)
    {
        CurrentPhase = GameInstance->GetGamePhase();
    }

    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerController in OnClick"));
        return;
    }

    // Use reliable hit detection
    FHitResult HitResult;
    bool bHitSuccess = PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

    if (bHitSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit detected at world location: %s"), *HitResult.Location.ToString());

        // === SETUP PHASE HANDLING ===
        if (CurrentPhase == EGamePhase::SETUP)
        {
            // Get grid coordinates from hit location
            FVector2D GridPosition = GridManager->GetXYPositionByRelativeLocation(HitResult.Location);
            SelectedGridX = FMath::FloorToInt(GridPosition.X);
            SelectedGridY = FMath::FloorToInt(GridPosition.Y);

            UE_LOG(LogTemp, Warning, TEXT("Setup phase: Selected grid position: %d, %d"), SelectedGridX, SelectedGridY);

            // Make sure position is in grid bounds
            if (SelectedGridX >= 0 && SelectedGridX < 25 && SelectedGridY >= 0 && SelectedGridY < 25)
            {
                // Check if the cell is already occupied
                if (GridManager->IsCellOccupied(SelectedGridX, SelectedGridY))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Cell is already occupied!"));
                    return;
                }

                // Show unit selection widget at this position
                ShowUnitSelectionWidget();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Clicked outside grid bounds"));
            }
        }
        // === PLAYING PHASE HANDLING ===
        else if (CurrentPhase == EGamePhase::PLAYING)
        {
            UE_LOG(LogTemp, Warning, TEXT("Click detected in PLAYING phase"));

            // Get the clicked actor and convert to unit if possible
            AUnit* ClickedUnit = Cast<AUnit>(HitResult.GetActor());

            // If we directly hit a unit
            if (ClickedUnit)
            {
                // If we hit a player unit
                if (ClickedUnit->bIsPlayerUnit)
                {
                    // If we have a unit selected and in attack mode, clicking another friendly unit cancels attack
                    if (bAttackMode && SelectedUnit)
                    {
                        // Cancel attack mode and reset
                        bAttackMode = false;
                        ClearAllHighlightsAndPaths();
                    }

                    // If we click on the already selected unit, show action widget
                    if (SelectedUnit == ClickedUnit)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Showing action widget for unit at %d,%d"),
                            ClickedUnit->GridX, ClickedUnit->GridY);
                        ShowMovementAndAttackWidget(ClickedUnit);
                    }
                    // Otherwise select the new unit
                    else
                    {
                        // Deselect previous unit if any
                        if (SelectedUnit)
                        {
                            SelectedUnit->UnshowSelected();
                            ClearAllHighlightsAndPaths();
                        }

                        // Select new unit
                        SelectedUnit = ClickedUnit;
                        SelectedUnit->ShowSelected();
                        UE_LOG(LogTemp, Warning, TEXT("Selected unit at %d,%d"),
                            SelectedUnit->GridX, SelectedUnit->GridY);

                        // Show the action widget
                        ShowMovementAndAttackWidget(SelectedUnit);
                    }
                }
                // If we hit an enemy unit
                else
                {
                    // If we're in attack mode and have a unit selected, try to attack
                    if (bAttackMode && SelectedUnit)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Trying to attack enemy at %d,%d"),
                            ClickedUnit->GridX, ClickedUnit->GridY);

                        bool bAttackSuccess = TryAttackUnit(SelectedUnit, ClickedUnit);
                        if (bAttackSuccess)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Attack successful!"));

                            // Reset attack mode
                            bAttackMode = false;
                            ClearAllHighlightsAndPaths();
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Attack failed!"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Clicked on enemy unit but not in attack mode or no unit selected"));
                    }
                }
            }
            // If we hit the grid instead of a unit
            else if (GridManager)
            {
                // Get grid coordinates from hit location
                FVector2D GridPosition = GridManager->GetXYPositionByRelativeLocation(HitResult.Location);
                int32 TargetGridX = FMath::FloorToInt(GridPosition.X);
                int32 TargetGridY = FMath::FloorToInt(GridPosition.Y);

                UE_LOG(LogTemp, Warning, TEXT("Hit grid position: %d, %d"), TargetGridX, TargetGridY);

                // Make sure position is in grid bounds
                if (TargetGridX >= 0 && TargetGridX < 25 && TargetGridY >= 0 && TargetGridY < 25)
                {
                    // Check if there's a unit at this position (might be one that wasn't directly hit)
                    AUnit* UnitAtPosition = FindUnitAtPosition(TargetGridX, TargetGridY);

                    if (UnitAtPosition)
                    {
                        // If it's a friendly unit
                        if (UnitAtPosition->bIsPlayerUnit)
                        {
                            // If we have a unit selected and in attack mode, clicking another friendly unit cancels attack
                            if (bAttackMode && SelectedUnit)
                            {
                                // Cancel attack mode and reset
                                bAttackMode = false;
                                ClearAllHighlightsAndPaths();
                            }

                            // If we click on the already selected unit, show action widget
                            if (SelectedUnit == UnitAtPosition)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Showing action widget for unit at %d,%d"),
                                    UnitAtPosition->GridX, UnitAtPosition->GridY);
                                ShowMovementAndAttackWidget(UnitAtPosition);
                            }
                            // Otherwise select the new unit
                            else
                            {
                                // Deselect previous unit if any
                                if (SelectedUnit)
                                {
                                    SelectedUnit->UnshowSelected();
                                    ClearAllHighlightsAndPaths();
                                }

                                // Select new unit
                                SelectedUnit = UnitAtPosition;
                                SelectedUnit->ShowSelected();
                                UE_LOG(LogTemp, Warning, TEXT("Selected unit at %d,%d"),
                                    SelectedUnit->GridX, SelectedUnit->GridY);

                                // Show the action widget
                                ShowMovementAndAttackWidget(SelectedUnit);
                            }
                        }
                        // If we hit an enemy unit
                        else
                        {
                            // If we're in attack mode and have a unit selected, try to attack
                            if (bAttackMode && SelectedUnit)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Trying to attack enemy at %d,%d"),
                                    UnitAtPosition->GridX, UnitAtPosition->GridY);

                                bool bAttackSuccess = TryAttackUnit(SelectedUnit, UnitAtPosition);
                                if (bAttackSuccess)
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Attack successful!"));

                                    // Reset attack mode
                                    bAttackMode = false;
                                    ClearAllHighlightsAndPaths();
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Attack failed!"));
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Clicked on enemy unit but not in attack mode or no unit selected"));
                            }
                        }
                    }
                    // If we click on an empty cell
                    else
                    {
                        // If in move mode and we have a unit selected
                        if (bMoveMode && SelectedUnit)
                        {
                            // Check if this cell is in movement range (highlighted)
                            bool bIsInMovementRange = false;
                            ATile* TargetTile = nullptr;

                            if (GridManager->TileMap.Contains(FVector2D(TargetGridX, TargetGridY)))
                            {
                                TargetTile = GridManager->TileMap[FVector2D(TargetGridX, TargetGridY)];
                                bIsInMovementRange = GridManager->HighlightedTiles.Contains(TargetTile);
                            }

                            if (bIsInMovementRange)
                            {
                                // Check if unit has already moved
                                if (SelectedUnit->bHasMovedThisTurn)
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Unit has already moved this turn!"));
                                }
                                else
                                {
                                    // Calculate path
                                    CalculatePath(SelectedUnit->GridX, SelectedUnit->GridY, TargetGridX, TargetGridY);

                                    // Store unit reference
                                    AUnit* UnitToMove = SelectedUnit;

                                    // Clear highlights but keep path
                                    GridManager->ClearAllHighlights();
                                    GridManager->HighlightPath(CurrentPath, true);

                                    // Move the unit
                                    UE_LOG(LogTemp, Warning, TEXT("Moving unit from %d,%d to %d,%d"),
                                        UnitToMove->GridX, UnitToMove->GridY, TargetGridX, TargetGridY);

                                    // Update grid - free old cell
                                    GridManager->OccupyCell(UnitToMove->GridX, UnitToMove->GridY, nullptr);

                                    // Update unit position
                                    FVector NewLocation = GridManager->GetWorldLocationFromGrid(TargetGridX, TargetGridY);
                                    UnitToMove->GridX = TargetGridX;
                                    UnitToMove->GridY = TargetGridY;
                                    UnitToMove->SetActorLocation(NewLocation);

                                    // Occupy new cell
                                    GridManager->OccupyCell(TargetGridX, TargetGridY, UnitToMove);

                                    // Mark as moved
                                    UnitToMove->bHasMovedThisTurn = true;

                                    UE_LOG(LogTemp, Warning, TEXT("Move successful!"));

                                    // Reset move mode
                                    bMoveMode = false;
                                    ClearAllHighlightsAndPaths();

                                    // Show action widget again to allow attack after move
                                    ShowMovementAndAttackWidget(UnitToMove);
                                }
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Target cell is not in movement range"));
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Clicked on empty cell but not in move mode or no unit selected"));
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Clicked outside grid bounds"));
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No hit result under cursor"));
    }
}

void ASaT_HumanPlayer::ShowUnitSelectionWidget()
{
    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerController in ShowUnitSelectionWidget"));
        return;
    }

    // Check if widget instance already exists
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Check if class reference is valid
    if (!UnitSelectionWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UnitSelectionWidgetClass is not set! Cannot show widget"));
        return;
    }

    // Determine which units have already been placed
    bool bSniperPlaced = false;
    bool bBrawlerPlaced = false;

    // Count placed units of each type
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit && Unit->bIsPlayerUnit)
        {
            if (Cast<ASniper>(Unit))
            {
                bSniperPlaced = true;
                UE_LOG(LogTemp, Warning, TEXT("Sniper already placed"));
            }
            else if (Cast<ABrawler>(Unit))
            {
                bBrawlerPlaced = true;
                UE_LOG(LogTemp, Warning, TEXT("Brawler already placed"));
            }
        }
    }

    // If both unit types have been placed, don't show widget at all
    if (bSniperPlaced && bBrawlerPlaced)
    {
        UE_LOG(LogTemp, Warning, TEXT("Both unit types already placed, not showing selection widget"));
        return;
    }

    // Create unit selection widget
    UnitSelectionWidget = CreateWidget<UUserWidget>(PC, UnitSelectionWidgetClass);
    if (UnitSelectionWidget)
    {
        // Find buttons in widget
        UButton* SniperButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("SniperButton")));
        UButton* BrawlerButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("BrawlerButton")));

        // Only bind events for units that haven't been placed yet
        if (SniperButton && !bSniperPlaced)
        {
            SniperButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetSniperSelected);
            UE_LOG(LogTemp, Display, TEXT("SniperButton found and bound to function"));
        }
        else if (SniperButton)
        {
            UE_LOG(LogTemp, Warning, TEXT("Sniper already placed - not binding button"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SniperButton not found in widget!"));
        }

        if (BrawlerButton && !bBrawlerPlaced)
        {
            BrawlerButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetBrawlerSelected);
            UE_LOG(LogTemp, Display, TEXT("BrawlerButton found and bound to function"));
        }
        else if (BrawlerButton)
        {
            UE_LOG(LogTemp, Warning, TEXT("Brawler already placed - not binding button"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("BrawlerButton not found in widget!"));
        }

        // Show widget
        UnitSelectionWidget->AddToViewport();

        // Set focus on widget and show cursor
        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not create unit selection widget!"));
    }
}

void ASaT_HumanPlayer::OnUnitWidgetSniperSelected()
{
    // Hide widget
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Restore normal input mode
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = true; // Keep cursor visible
    }

    // Place Sniper at selected coordinates
    PlaceUnit(SelectedGridX, SelectedGridY, true);
}

void ASaT_HumanPlayer::OnUnitWidgetBrawlerSelected()
{
    // Hide widget
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Restore normal input mode
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = true; // Keep cursor visible
    }

    // Place Brawler at selected coordinates
    PlaceUnit(SelectedGridX, SelectedGridY, false);
}

void ASaT_HumanPlayer::EndTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("Human Player ending turn"));

    ClearAllHighlightsAndPaths();

    // Get the game instance to pass turn
    GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot switch turns: GameInstance not found"));
        return;
    }

    // Before switching turn, update GameInstance with our placed units count
    GameInstance->HumanUnitsPlaced = PlacedUnitsCount;

    // Important: Set our own IsMyTurn to false BEFORE calling GameMode->EndTurn()
    IsMyTurn = false;
    UE_LOG(LogTemp, Warning, TEXT("Human Player setting IsMyTurn to FALSE"));

    // Get the game mode and tell it to end the turn
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("Human Player calling GameMode->EndTurn()"));
        GameMode->EndTurn();
    }
    else
    {
        // Fallback if we can't get GameMode - update directly
        UE_LOG(LogTemp, Warning, TEXT("GameMode not found, calling GameInstance->SwitchTurn() directly"));
        GameInstance->SwitchTurn();
    }
}

void ASaT_HumanPlayer::ShowEndTurnButton()
{
    UE_LOG(LogTemp, Warning, TEXT("ShowEndTurnButton called"));

    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot show End Turn button: Player Controller is NULL"));
        return;
    }

    // Check if we have the widget class set
    if (!EndTurnButtonWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("EndTurnButtonWidgetClass is not set! Cannot show End Turn button"));
        return;
    }

    // Create EndTurnButton widget if it doesn't exist
    if (!EndTurnWidget && EndTurnButtonWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Creating new End Turn button widget"));

        // Use the class reference instead of trying to find it at runtime
        EndTurnWidget = CreateWidget<UUserWidget>(PC, EndTurnButtonWidgetClass);
        if (EndTurnWidget)
        {
            EndTurnWidget->AddToViewport();
            UE_LOG(LogTemp, Warning, TEXT("End Turn widget added to viewport"));

            // Find and bind the button
            UButton* EndTurnBtn = Cast<UButton>(EndTurnWidget->GetWidgetFromName(TEXT("EndTurnButton")));
            if (EndTurnBtn)
            {
                EndTurnBtn->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::EndTurn);
                UE_LOG(LogTemp, Display, TEXT("End Turn Button bound successfully"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to find EndTurnButton in widget"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create End Turn widget"));
        }
    }
    else if (EndTurnWidget && !EndTurnWidget->IsInViewport())
    {
        // Make sure it's in the viewport if not already
        UE_LOG(LogTemp, Warning, TEXT("Adding existing End Turn widget to viewport"));
        EndTurnWidget->AddToViewport();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("End Turn widget already exists and is in viewport"));
    }
}

void ASaT_HumanPlayer::ShowMovementRange(AUnit* Unit)
{
    if (!Unit || !GridManager)
    {
        return;
    }

    // Clear only movement highlights, not path highlights
    GridManager->ClearAllHighlights();

    // Get the unit's current position and movement range
    int32 UnitX = Unit->GridX;
    int32 UnitY = Unit->GridY;
    int32 MovementRange = Unit->Movement;

    UE_LOG(LogTemp, Warning, TEXT("Showing movement range for unit at (%d, %d) with range %d"),
        UnitX, UnitY, MovementRange);

    // BFS to find all reachable cells within movement range
    // Initialize visited array (25x25 grid)
    bool Visited[25][25] = { false };

    // Queue for BFS - stores positions and remaining movement
    TQueue<TPair<FVector2D, int32>> Queue;

    // Start at unit position with full movement range
    Queue.Enqueue(TPair<FVector2D, int32>(FVector2D(UnitX, UnitY), MovementRange));
    Visited[UnitX][UnitY] = true;

    // Direction vectors for the four cardinal directions
    int32 dx[] = { 1, -1, 0, 0 };
    int32 dy[] = { 0, 0, 1, -1 };

    // Track how many cells we highlight
    int32 HighlightedCount = 0;

    while (!Queue.IsEmpty())
    {
        TPair<FVector2D, int32> Current;
        Queue.Dequeue(Current);

        FVector2D Pos = Current.Key;
        int32 RemainingMovement = Current.Value;

        int32 CurrX = FMath::FloorToInt(Pos.X);
        int32 CurrY = FMath::FloorToInt(Pos.Y);

        // Highlight the cell if it's not the unit's position
        if (CurrX != UnitX || CurrY != UnitY)
        {
            GridManager->HighlightCell(CurrX, CurrY, true);
            HighlightedCount++;
        }

        // If no more movement left, don't explore further from this cell
        if (RemainingMovement <= 0)
        {
            continue;
        }

        // Try all four directions
        for (int32 i = 0; i < 4; i++)
        {
            int32 NewX = CurrX + dx[i];
            int32 NewY = CurrY + dy[i];

            // Check if new position is valid and not visited
            if (GridManager->IsValidPosition(FVector2D(NewX, NewY)) && !Visited[NewX][NewY])
            {
                // Check if the cell is not occupied (except by the unit itself)
                bool bIsCellFree = !GridManager->IsCellOccupied(NewX, NewY) ||
                    (NewX == UnitX && NewY == UnitY);

                if (bIsCellFree)
                {
                    // Mark as visited and add to the queue with one less movement point
                    Visited[NewX][NewY] = true;
                    Queue.Enqueue(TPair<FVector2D, int32>(FVector2D(NewX, NewY), RemainingMovement - 1));
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Movement range visualization complete: %d cells highlighted"), HighlightedCount);
}

bool ASaT_HumanPlayer::TryMoveUnit(AUnit* Unit, int32 TargetGridX, int32 TargetGridY)
{
    if (!Unit || !GridManager)
    {
        return false;
    }

    // Use the Unit's Move method which has the bHasMovedThisTurn check
    if (Unit->Move(TargetGridX, TargetGridY))
    {
        // The move was successful, update the grid
        GridManager->OccupyCell(TargetGridX, TargetGridY, Unit);

        return true;
    }

    return false;
}

void ASaT_HumanPlayer::CalculatePath(int32 StartX, int32 StartY, int32 EndX, int32 EndY)
{
    // Clear existing path array
    CurrentPath.Empty();

    // Always start with the source position
    CurrentPath.Add(FVector2D(StartX, StartY));

    // Calculate Manhattan path (this will always move horizontally first, then vertically)
    // You could modify this to choose different path strategies

    int32 CurrentX = StartX;
    int32 CurrentY = StartY;

    // Move horizontally first to align X-coordinate
    while (CurrentX != EndX)
    {
        CurrentX += (CurrentX < EndX) ? 1 : -1;

        // Skip if cell is occupied (except for the destination)
        if (GridManager->IsCellOccupied(CurrentX, CurrentY) && (CurrentX != EndX || CurrentY != EndY))
            continue;

        CurrentPath.Add(FVector2D(CurrentX, CurrentY));
    }

    // Then move vertically to align Y-coordinate
    while (CurrentY != EndY)
    {
        CurrentY += (CurrentY < EndY) ? 1 : -1;

        // Skip if cell is occupied (except for the destination)
        if (GridManager->IsCellOccupied(CurrentX, CurrentY) && (CurrentX != EndX || CurrentY != EndY))
            continue;

        CurrentPath.Add(FVector2D(CurrentX, CurrentY));
    }

    // Display the path WITHOUT clearing previous path
    if (GridManager)
    {
        // Pass false to prevent clearing previous path
        GridManager->HighlightPath(CurrentPath, false);
    }
}

void ASaT_HumanPlayer::ClearPath()
{
    CurrentPath.Empty();

    if (GridManager)
    {
        GridManager->ClearPathHighlights();
    }
}

void ASaT_HumanPlayer::ClearAllHighlightsAndPaths()
{
    if (GridManager)
    {
        // First clear the highlighted tiles
        GridManager->ClearAllHighlights();

        // Then clear the path tiles
        GridManager->ClearPathHighlights();

        // Also clear our path tracking array
        CurrentPath.Empty();

        UE_LOG(LogTemp, Warning, TEXT("All highlights and paths cleared"));
    }
}

void ASaT_HumanPlayer::ShowAttackRange(AUnit* Unit)
{
    if (!Unit || !GridManager)
    {
        return;
    }

    // Clear existing movement highlights, but keep any path highlights
    GridManager->ClearAllHighlights();

    // Get the unit's current position and attack range
    int32 UnitX = Unit->GridX;
    int32 UnitY = Unit->GridY;
    int32 AttackRange = Unit->RangeAttack;

    UE_LOG(LogTemp, Warning, TEXT("Showing attack range for unit at (%d, %d) with range %d"),
        UnitX, UnitY, AttackRange);

    // For Sniper (long range attack), show all cells within range
    // For Brawler (close combat), only show adjacent cells
    bool bIsSniper = Cast<ASniper>(Unit) != nullptr;

    // Highlight cells in attack range
    int32 HighlightedCount = 0;

    // Simple Manhattan distance calculation for all cells
    for (int32 X = UnitX - AttackRange; X <= UnitX + AttackRange; X++)
    {
        for (int32 Y = UnitY - AttackRange; Y <= UnitY + AttackRange; Y++)
        {
            // Skip if out of grid bounds
            if (X < 0 || X >= 25 || Y < 0 || Y >= 25)
                continue;

            // Calculate Manhattan distance
            int32 Distance = FMath::Abs(X - UnitX) + FMath::Abs(Y - UnitY);

            // Check if in range and not the unit's own position
            if (Distance <= AttackRange && (X != UnitX || Y != UnitY))
            {
                // For Snipers, ignore obstacles
                // For Brawlers, check if path is clear (simplified)
                bool bCanAttack = false;

                if (bIsSniper)
                {
                    // Snipers can attack through obstacles
                    bCanAttack = true;
                }
                else
                {
                    // Brawlers need adjacent cells (already checked with Distance)
                    bCanAttack = (Distance <= AttackRange);
                }

                // Only highlight cells that can be attacked
                if (bCanAttack)
                {
                    // Check if there's an enemy unit at this position
                    AUnit* TargetUnit = FindUnitAtPosition(X, Y);
                    if (TargetUnit && !TargetUnit->bIsPlayerUnit)
                    {
                        // Highlight with a different color for enemy units in range
                        // For simplicity, use the path highlight color (usually red/yellow)
                        GridManager->HighlightCell(X, Y, true);
                        HighlightedCount++;
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Attack range visualization complete: %d cells highlighted"), HighlightedCount);
}

bool ASaT_HumanPlayer::TryAttackUnit(AUnit* AttackingUnit, AUnit* TargetUnit)
{
    if (!AttackingUnit || !TargetUnit || !GridManager)
    {
        return false;
    }

    // Check if attacking unit has already attacked this turn
    if (UnitAttackedThisTurn.Contains(AttackingUnit) && UnitAttackedThisTurn[AttackingUnit])
    {
        UE_LOG(LogTemp, Warning, TEXT("Unit has already attacked this turn!"));
        return false;
    }

    // Get positions
    int32 AttackerX = AttackingUnit->GridX;
    int32 AttackerY = AttackingUnit->GridY;
    int32 TargetX = TargetUnit->GridX;
    int32 TargetY = TargetUnit->GridY;

    // Calculate Manhattan distance
    int32 Distance = FMath::Abs(TargetX - AttackerX) + FMath::Abs(TargetY - AttackerY);

    // Check if target is in range
    if (Distance <= AttackingUnit->RangeAttack)
    {
        // Perform the attack using the Unit's Attack method
        bool bAttackSuccess = AttackingUnit->Attack(TargetUnit);

        if (bAttackSuccess)
        {
            // Mark the unit as having attacked this turn
            UnitAttackedThisTurn.Add(AttackingUnit, true);

            UE_LOG(LogTemp, Warning, TEXT("Attack successful! Target HP: %d"), TargetUnit->Hp);

            // If target is now dead, handle it
            if (!TargetUnit->IsAlive())
            {
                UE_LOG(LogTemp, Warning, TEXT("Target was killed by the attack!"));

                // Make sure the cell is marked as unoccupied
                GridManager->OccupyCell(TargetX, TargetY, nullptr);

                // Update the HUD
                AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
                ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
                if (GameMode)
                {
                    GameMode->UpdateGameHUD();
                }
            }

            return true;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is out of attack range!"));
    }

    return false;
}

AUnit* ASaT_HumanPlayer::FindUnitAtPosition(int32 GridX, int32 GridY)
{
    // Check if the GridManager is valid
    if (!GridManager)
    {
        return nullptr;
    }

    // First check if the cell is occupied
    if (!GridManager->IsCellOccupied(GridX, GridY))
    {
        return nullptr;
    }

    // Try to get the tile at this position
    ATile* Tile = nullptr;
    if (GridManager->TileMap.Contains(FVector2D(GridX, GridY)))
    {
        Tile = GridManager->TileMap[FVector2D(GridX, GridY)];
    }

    // Return the occupying unit if it exists
    if (Tile)
    {
        return Tile->OccupyingUnit;
    }

    return nullptr;
}

void ASaT_HumanPlayer::ShowMovementAndAttackWidget(AUnit* UnitToShow)
{
    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerController"));
        return;
    }

    // Remove previous widget if it exists
    if (MovementAndAttackWidget)
    {
        MovementAndAttackWidget->RemoveFromParent();
        MovementAndAttackWidget = nullptr;
    }

    // Check if class reference is valid
    if (!MovementAndAttackWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MovementAndAttackWidgetClass is not set! Cannot show widget"));
        return;
    }

    // Create widget
    MovementAndAttackWidget = CreateWidget<UUserWidget>(PC, MovementAndAttackWidgetClass);
    if (MovementAndAttackWidget)
    {
        // Add widget to viewport
        MovementAndAttackWidget->AddToViewport();

        // Find and bind buttons
        UButton* MoveButton = Cast<UButton>(MovementAndAttackWidget->GetWidgetFromName(TEXT("MoveButton")));
        UButton* AttackButton = Cast<UButton>(MovementAndAttackWidget->GetWidgetFromName(TEXT("AttackButton")));

        if (MoveButton)
        {
            MoveButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnMoveButtonClicked);
            UE_LOG(LogTemp, Display, TEXT("Move button bound successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MoveButton not found in widget!"));
        }

        if (AttackButton)
        {
            AttackButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnAttackButtonClicked);
            UE_LOG(LogTemp, Display, TEXT("Attack button bound successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AttackButton not found in widget!"));
        }

        // Update unit name text if it exists
        //UTextBlock* UnitNameText = Cast<UTextBlock>(MovementAndAttackWidget->GetWidgetFromName(TEXT("UnitNameText")));
        //if (UnitNameText && Unit)
        //{
            // Determine unit type
           // FString UnitType = Cast<ASniper>(Unit) ? TEXT("Sniper") : TEXT("Brawler");
           // UnitNameText->SetText(FText::FromString(FString::Printf(TEXT("%s at (%d,%d)"),
                //*UnitType, Unit->GridX, Unit->GridY)));
        //}

        // Disable attack button if unit has already attacked
        //if (AttackButton && UnitAttackedThisTurn.Contains(Unit) && UnitAttackedThisTurn[Unit])
        //{
            //AttackButton->SetIsEnabled(false);
        //}

        // Disable move button if unit has already moved
        //if (MoveButton && Unit && Unit->bHasMovedThisTurn)
        //{
           // MoveButton->SetIsEnabled(false);
        //}
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create MovementAndAttack widget!"));
    }
}

void ASaT_HumanPlayer::OnMoveButtonClicked()
{
    // Clear previous highlights
    ClearAllHighlightsAndPaths();

    // Set flags
    bMoveMode = true;
    bAttackMode = false;

    // Show movement range
    if (SelectedUnit)
    {
        ShowMovementRange(SelectedUnit);
        UE_LOG(LogTemp, Warning, TEXT("Move mode activated - showing movement range"));
    }

    // Hide widget after selection
    if (MovementAndAttackWidget)
    {
        MovementAndAttackWidget->RemoveFromParent();
        MovementAndAttackWidget = nullptr;
    }
}

void ASaT_HumanPlayer::OnAttackButtonClicked()
{
    // Clear previous highlights
    ClearAllHighlightsAndPaths();

    // Set flags
    bMoveMode = false;
    bAttackMode = true;

    // Show attack range
    if (SelectedUnit)
    {
        ShowAttackRange(SelectedUnit);
        UE_LOG(LogTemp, Warning, TEXT("Attack mode activated - showing attack range"));
    }

    // Hide widget after selection
    if (MovementAndAttackWidget)
    {
        MovementAndAttackWidget->RemoveFromParent();
        MovementAndAttackWidget = nullptr;
    }
}

