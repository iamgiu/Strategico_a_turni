// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_HumanPlayer.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GridManager.h"
#include <Kismet/GameplayStatics.h>
#include "Sniper.h"
#include "Brawler.h"
#include "Unit.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
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
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Base initialization
    SelectionState = 0;
    CurrentPhase = EGamePhase::SETUP;
    PlacedUnitsCount = 0; // Initialize placement count
    UnitsToPlace = 2;

    ShowEndTurnButton();

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
    // Debug log to confirm function is called
    UE_LOG(LogTemp, Warning, TEXT("OnClick called - IsMyTurn: %s"), IsMyTurn ? TEXT("TRUE") : TEXT("FALSE"));

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

    // Use a more reliable hit detection method with ECC_Visibility channel
    FHitResult HitResult;
    bool bHitSuccess = PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

    if (bHitSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit detected at world location: %s"), *HitResult.Location.ToString());

        // === SETUP PHASE HANDLING ===
        if (CurrentPhase == EGamePhase::SETUP)
        {
            // Check if already placed all units
            if (PlacedUnitsCount >= UnitsToPlace)
            {
                UE_LOG(LogTemp, Warning, TEXT("Already placed all units (%d/%d). End your turn."),
                    PlacedUnitsCount, UnitsToPlace);
                return;
            }

            // Store selection location
            LastSelectedCell = HitResult.Location;

            if (GridManager)
            {
                // Get grid coordinates from world location
                FVector2D GridPosition = GridManager->GetXYPositionByRelativeLocation(HitResult.Location);
                SelectedGridX = FMath::FloorToInt(GridPosition.X);
                SelectedGridY = FMath::FloorToInt(GridPosition.Y);

                UE_LOG(LogTemp, Warning, TEXT("Cell selected in grid coordinates: X=%d, Y=%d"),
                    SelectedGridX, SelectedGridY);

                // Ensure grid position is valid
                if (SelectedGridX >= 0 && SelectedGridX < 25 && SelectedGridY >= 0 && SelectedGridY < 25)
                {
                    // Check if cell is occupied
                    if (!GridManager->IsCellOccupied(SelectedGridX, SelectedGridY))
                    {
                        // Cell is empty, show unit selection UI
                        ShowUnitSelectionWidget();
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Cell occupied! Cannot place a unit here"));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Selected position outside grid bounds"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("GridManager is not valid"));
            }
        }
        // === PLAYING PHASE HANDLING ===
        else if (CurrentPhase == EGamePhase::PLAYING)
        {
            UE_LOG(LogTemp, Warning, TEXT("Click detected in PLAYING phase"));

            // First, try to cast the hit actor to AUnit
            AUnit* ClickedUnit = Cast<AUnit>(HitResult.GetActor());

            // Debug more information about what was hit
            if (HitResult.GetActor())
            {
                UE_LOG(LogTemp, Warning, TEXT("Hit actor class: %s"), *HitResult.GetActor()->GetClass()->GetName());
            }

            // Process hitting a unit directly
            if (ClickedUnit && ClickedUnit->bIsPlayerUnit)
            {
                UE_LOG(LogTemp, Warning, TEXT("Direct hit on player unit at %d, %d"),
                    ClickedUnit->GridX, ClickedUnit->GridY);

                // If we click on the already selected unit, deselect it
                if (SelectedUnit == ClickedUnit)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Deselecting currently selected unit"));
                    SelectedUnit->UnshowSelected();
                    SelectedUnit = nullptr;

                    // Use the helper method to clear all highlights
                    ClearAllHighlightsAndPaths();
                }
                // Otherwise, select the new unit
                else
                {
                    // Deselect any previously selected unit and clear its highlights
                    if (SelectedUnit)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Deselecting previous unit"));
                        SelectedUnit->UnshowSelected();

                        // Use the helper method to clear all highlights
                        ClearAllHighlightsAndPaths();
                    }

                    // Select the new unit
                    SelectedUnit = ClickedUnit;
                    UE_LOG(LogTemp, Warning, TEXT("Selecting unit at %d,%d and changing material"),
                        SelectedUnit->GridX, SelectedUnit->GridY);
                    SelectedUnit->ShowSelected();

                    // Show valid movement range for the selected unit
                    ShowMovementRange(SelectedUnit);
                }
            }
            // If we didn't hit a unit directly, check tile at hit location
            else
            {
                if (GridManager)
                {
                    // Get grid coordinates from hit location
                    FVector2D GridPosition = GridManager->GetXYPositionByRelativeLocation(HitResult.Location);
                    int32 TargetGridX = FMath::FloorToInt(GridPosition.X);
                    int32 TargetGridY = FMath::FloorToInt(GridPosition.Y);

                    UE_LOG(LogTemp, Warning, TEXT("Hit grid position: %d, %d"), TargetGridX, TargetGridY);

                    // Make sure position is in grid bounds
                    if (TargetGridX >= 0 && TargetGridX < 25 && TargetGridY >= 0 && TargetGridY < 25)
                    {
                        // Check if there's a unit at this grid position
                        if (GridManager->IsCellOccupied(TargetGridX, TargetGridY))
                        {
                            // Try to get the tile and its occupying unit
                            ATile* Tile = nullptr;
                            if (GridManager->TileMap.Contains(FVector2D(TargetGridX, TargetGridY)))
                            {
                                Tile = GridManager->TileMap[FVector2D(TargetGridX, TargetGridY)];
                            }

                            // If we found a tile with a player unit
                            if (Tile && Tile->OccupyingUnit && Tile->OccupyingUnit->bIsPlayerUnit)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Found player unit at tile %d, %d"),
                                    TargetGridX, TargetGridY);

                                // If already selected, deselect
                                if (SelectedUnit == Tile->OccupyingUnit)
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Deselecting currently selected unit"));
                                    SelectedUnit->UnshowSelected();
                                    SelectedUnit = nullptr;

                                    // Use the helper method to clear all highlights
                                    ClearAllHighlightsAndPaths();
                                }
                                else
                                {
                                    // If we had a previously selected unit, deselect it and clear highlights
                                    if (SelectedUnit)
                                    {
                                        UE_LOG(LogTemp, Warning, TEXT("Deselecting previous unit"));
                                        SelectedUnit->UnshowSelected();

                                        // Use the helper method to clear all highlights
                                        ClearAllHighlightsAndPaths();
                                    }

                                    // Select the new unit
                                    SelectedUnit = Tile->OccupyingUnit;
                                    SelectedUnit->ShowSelected();

                                    // Show movement range
                                    ShowMovementRange(SelectedUnit);
                                }
                            }
                            // If there's an enemy unit - could implement attack logic here
                            else if (Tile && Tile->OccupyingUnit)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Found enemy unit at tile %d, %d"),
                                    TargetGridX, TargetGridY);

                                // If we have a selected unit, and it's in range, we could attack
                                // TODO: Add attack logic here
                            }
                        }
                        // Empty cell and we have a unit selected - try to move if it's highlighted
                        else if (SelectedUnit)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Attempting to move unit to %d, %d"),
                                TargetGridX, TargetGridY);

                            // Check if this cell is highlighted (in movement range)
                            bool bIsInMovementRange = false;
                            ATile* TargetTile = nullptr;

                            if (GridManager->TileMap.Contains(FVector2D(TargetGridX, TargetGridY)))
                            {
                                TargetTile = GridManager->TileMap[FVector2D(TargetGridX, TargetGridY)];
                                bIsInMovementRange = GridManager->HighlightedTiles.Contains(TargetTile);
                            }

                            if (bIsInMovementRange)
                            {
                                // First, calculate the path - this needs to happen before any movement
                                CalculatePath(SelectedUnit->GridX, SelectedUnit->GridY, TargetGridX, TargetGridY);

                                // Check if the unit has already moved this turn
                                if (SelectedUnit->bHasMovedThisTurn)
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Unit has already moved this turn!"));
                                    // We don't deselect the unit or clear highlights here
                                }
                                else
                                {
                                    // Store reference to unit so we don't lose it during operations
                                    AUnit* UnitToMove = SelectedUnit;

                                    // Clear ONLY the movement range highlights (blue cells)
                                    // but keep the path highlights intact
                                    GridManager->ClearAllHighlights();

                                    // Draw the path more visibly
                                    GridManager->HighlightPath(CurrentPath, true);

                                    // Move is valid - it's in the highlighted range and hasn't moved yet
                                    UE_LOG(LogTemp, Warning, TEXT("Moving unit from %d,%d to %d,%d"),
                                        UnitToMove->GridX, UnitToMove->GridY, TargetGridX, TargetGridY);

                                    // Update grid state - free old cell
                                    GridManager->OccupyCell(UnitToMove->GridX, UnitToMove->GridY, nullptr);

                                    // Move the unit physically
                                    FVector NewLocation = GridManager->GetWorldLocationFromGrid(TargetGridX, TargetGridY);
                                    UnitToMove->GridX = TargetGridX;
                                    UnitToMove->GridY = TargetGridY;
                                    UnitToMove->SetActorLocation(NewLocation);

                                    // Occupy new cell
                                    GridManager->OccupyCell(TargetGridX, TargetGridY, UnitToMove);

                                    // Mark that the unit has moved this turn
                                    UnitToMove->bHasMovedThisTurn = true;

                                    UE_LOG(LogTemp, Warning, TEXT("Unit moved successfully"));

                                    // IMPORTANT: Don't automatically show the new movement range
                                    // Let the player decide what to do next - they can deselect the unit
                                    // if they want to see a new movement range
                                }
                            }
                        }
                        // Empty cell with no unit selected
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Clicked on empty space with no unit selected"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Click outside valid grid bounds"));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("GridManager is invalid"));
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
    if (PC)
    {
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

        // Create unit selection widget
        UnitSelectionWidget = CreateWidget<UUserWidget>(PC, UnitSelectionWidgetClass);
        if (UnitSelectionWidget)
        {
            // Find buttons in widget
            UButton* SniperButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("SniperButton")));
            UButton* BrawlerButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("BrawlerButton")));

            // Bind buttons directly to functions
            if (SniperButton)
            {
                SniperButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetSniperSelected);
                UE_LOG(LogTemp, Display, TEXT("SniperButton found and bound to function"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SniperButton not found in widget!"));
            }

            if (BrawlerButton)
            {
                BrawlerButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetBrawlerSelected);
                UE_LOG(LogTemp, Display, TEXT("BrawlerButton found and bound to function"));
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
