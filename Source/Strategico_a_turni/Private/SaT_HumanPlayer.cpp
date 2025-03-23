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

       UnitSelectionWidget = nullptr;

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

    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Reset input mode at start of turn
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        // Use GameAndUI consistently here too
        FInputModeGameAndUI InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
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

        // Find the clicked tile or closest tile
        ATile* ClickedTile = nullptr;

        // First try to get the directly hit tile
        ClickedTile = Cast<ATile>(HitResult.GetActor());

        // If no direct hit, find the closest tile
        if (!ClickedTile && GridManager)
        {
            float ClosestDistance = FLT_MAX;
            for (auto& TilePair : GridManager->TileMap)
            {
                ATile* Tile = TilePair.Value;
                if (Tile)
                {
                    float DistSq = FVector::DistSquared(Tile->GetActorLocation(), HitResult.Location);
                    if (DistSq < ClosestDistance)
                    {
                        ClosestDistance = DistSq;
                        ClickedTile = Tile;
                    }
                }
            }
        }

        // Process the clicked tile if found
        if (ClickedTile)
        {
            // === SETUP PHASE HANDLING ===
            if (CurrentPhase == EGamePhase::SETUP)
            {
                UE_LOG(LogTemp, Warning, TEXT("Setup phase: Selected tile at position: %d, %d"),
                    ClickedTile->GridX, ClickedTile->GridY);

                // Check if the tile is already occupied
                if (ClickedTile->bIsOccupied)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Tile is already occupied!"));
                    return;
                }

                // Store selected tile coordinates
                SelectedGridX = ClickedTile->GridX;
                SelectedGridY = ClickedTile->GridY;

                // Show unit selection widget
                ShowUnitSelectionWidget();
            }
            // === PLAYING PHASE HANDLING ===
            else if (CurrentPhase == EGamePhase::PLAYING)
            {
                // Playing phase logic - handle unit or tile selection
                HandlePlayingPhaseClick(ClickedTile);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find a valid tile at click location"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No hit result under cursor"));
    }
}

void ASaT_HumanPlayer::HandlePlayingPhaseClick(ATile* ClickedTile)
{
    if (!ClickedTile)
        return;

    UE_LOG(LogTemp, Warning, TEXT("Click detected in PLAYING phase on tile %d,%d"),
        ClickedTile->GridX, ClickedTile->GridY);

    // Check if there's a unit on this tile
    AUnit* ClickedUnit = ClickedTile->OccupyingUnit;

    // If we're clicking on empty tile and we're not in move or attack mode,
    // deselect the current unit if there is one
    if (!ClickedUnit && !bMoveMode && !bAttackMode && SelectedUnit)
    {
        // Deselect the current unit
        SelectedUnit->UnshowSelected();
        SelectedUnit = nullptr;
        ClearAllHighlightsAndPaths();
        return;
    }

    // If we found a unit on the tile
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
    // If the tile is empty
    else
    {
        // Handle empty tile click (movement logic)
        if (bMoveMode && SelectedUnit)
        {
            // Check if this cell is in movement range (highlighted)
            bool bIsInMovementRange = GridManager->HighlightedTiles.Contains(ClickedTile);

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
                    CalculatePath(SelectedUnit->GridX, SelectedUnit->GridY, ClickedTile->GridX, ClickedTile->GridY);

                    // Store unit reference
                    AUnit* UnitToMove = SelectedUnit;

                    // Clear highlights but keep path
                    GridManager->ClearAllHighlights();
                    GridManager->HighlightPath(CurrentPath, true);

                    // Move the unit
                    UE_LOG(LogTemp, Warning, TEXT("Moving unit from %d,%d to %d,%d"),
                        UnitToMove->GridX, UnitToMove->GridY, ClickedTile->GridX, ClickedTile->GridY);

                    // Update grid - free old cell
                    GridManager->OccupyCell(UnitToMove->GridX, UnitToMove->GridY, nullptr);

                    // Update unit position
                    FVector NewLocation = GridManager->GetWorldLocationFromGrid(ClickedTile->GridX, ClickedTile->GridY);
                    UnitToMove->GridX = ClickedTile->GridX;
                    UnitToMove->GridY = ClickedTile->GridY;
                    UnitToMove->SetActorLocation(NewLocation);

                    // Occupy new cell
                    GridManager->OccupyCell(ClickedTile->GridX, ClickedTile->GridY, UnitToMove);

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

                // If clicked outside movement range, cancel move mode
                bMoveMode = false;
                ClearAllHighlightsAndPaths();
            }
        }
        else if (bAttackMode && SelectedUnit)
        {
            // If clicked empty space while in attack mode, cancel attack mode
            bAttackMode = false;
            ClearAllHighlightsAndPaths();
            UE_LOG(LogTemp, Warning, TEXT("Attack mode canceled"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Clicked on empty cell but not in move mode or no unit selected"));
        }
    }
}

void ASaT_HumanPlayer::DeselectCurrentUnit()
{
    if (SelectedUnit)
    {
        SelectedUnit->UnshowSelected();
        SelectedUnit = nullptr;

        // Cancel any modes
        bMoveMode = false;
        bAttackMode = false;

        // Clear UI elements
        ClearAllHighlightsAndPaths();

        if (MovementAndAttackWidget)
        {
            MovementAndAttackWidget->RemoveFromParent();
            MovementAndAttackWidget = nullptr;
        }
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
        // Add to viewport first
        UnitSelectionWidget->AddToViewport(10000); // High Z-order to be above other UI

        // Find buttons in widget
        UButton* SniperButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("SniperButton")));
        UButton* BrawlerButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("BrawlerButton")));

        // Only bind events for units that haven't been placed yet
        if (SniperButton && !bSniperPlaced)
        {
            SniperButton->OnClicked.Clear(); // Clear any existing bindings
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
            BrawlerButton->OnClicked.Clear(); // Clear any existing bindings
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

        // Set focus AFTER adding to viewport
        FInputModeGameAndUI InputMode;

        // VERY IMPORTANT: This is the critical part - explicitly set focus to the widget
        InputMode.SetWidgetToFocus(UnitSelectionWidget->TakeWidget());

        // Make sure mouse isn't locked
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        // Apply input mode
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        UE_LOG(LogTemp, Warning, TEXT("Unit selection widget set up with proper focus"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not create unit selection widget!"));
    }
}

void ASaT_HumanPlayer::OnUnitWidgetSniperSelected()
{
    // Hide widget and clean up reference
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Use GameAndUI input mode consistently
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
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

    // Use GameAndUI input mode consistently
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true; // Keep cursor visible
    }

    // Place Brawler at selected coordinates
    PlaceUnit(SelectedGridX, SelectedGridY, false);
}

void ASaT_HumanPlayer::EndTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("Human Player ending turn"));

    DeselectCurrentUnit();
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

    // Clean up any UI widgets before ending turn
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Restore normal input mode
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        // Don't change input mode when ending turn, as we'll set it correctly
        // when the next turn starts
        PC->bShowMouseCursor = true; // Keep cursor visible
    }

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

    // Create fresh widget
    if (EndTurnWidget)
    {
        EndTurnWidget->RemoveFromParent();
        EndTurnWidget = nullptr;
    }

    EndTurnWidget = CreateWidget<UUserWidget>(PC, EndTurnButtonWidgetClass);
    if (EndTurnWidget)
    {
        // Make sure the widget is visible and enabled
        EndTurnWidget->SetVisibility(ESlateVisibility::Visible);
        EndTurnWidget->SetIsEnabled(true);

        // Add to viewport with high Z-order - DO THIS ONLY ONCE
        EndTurnWidget->AddToViewport(10000);

        UButton* EndTurnBtn = Cast<UButton>(EndTurnWidget->GetWidgetFromName(TEXT("EndTurnButton")));
        if (EndTurnBtn)
        {
            EndTurnBtn->OnClicked.Clear(); // Clear any existing bindings
            EndTurnBtn->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::EndTurn);
            UE_LOG(LogTemp, Warning, TEXT("End Turn Button bound successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find EndTurnButton in widget!"));
        }

        // IMPORTANT: Use GameAndUI mode consistently
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(EndTurnWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        UE_LOG(LogTemp, Warning, TEXT("End Turn widget set up with proper focus"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create End Turn widget"));
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

    // Ensure GridManager is valid
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager is NULL in CalculatePath!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Calculating path from (%d,%d) to (%d,%d)"),
        StartX, StartY, EndX, EndY);

    // Always start with the source position
    CurrentPath.Add(FVector2D(StartX, StartY));

    // BFS approach to find the shortest path avoiding obstacles
    TArray<FVector2D> CameFrom;
    CameFrom.SetNumZeroed(25 * 25); // For a 25x25 grid, using 1D array
    TArray<bool> Visited;
    Visited.SetNumZeroed(25 * 25);

    // Direction vectors for the four cardinal directions
    int32 dx[] = { 1, -1, 0, 0 };
    int32 dy[] = { 0, 0, 1, -1 };

    TQueue<FVector2D> Queue;
    Queue.Enqueue(FVector2D(StartX, StartY));
    Visited[StartY * 25 + StartX] = true;

    bool bFoundPath = false;

    while (!Queue.IsEmpty() && !bFoundPath)
    {
        FVector2D Current;
        Queue.Dequeue(Current);

        // Check if we reached the destination
        if (FMath::FloorToInt(Current.X) == EndX && FMath::FloorToInt(Current.Y) == EndY)
        {
            bFoundPath = true;
            break;
        }

        // Try all four directions
        for (int32 i = 0; i < 4; i++)
        {
            int32 NewX = FMath::FloorToInt(Current.X) + dx[i];
            int32 NewY = FMath::FloorToInt(Current.Y) + dy[i];

            // Check if new position is valid and not visited
            if (GridManager->IsValidPosition(FVector2D(NewX, NewY)) && !Visited[NewY * 25 + NewX])
            {
                // Check if the cell is not occupied (except for the destination)
                bool bIsCellFree = !GridManager->IsCellOccupied(NewX, NewY) ||
                    (NewX == EndX && NewY == EndY);

                if (bIsCellFree)
                {
                    // Mark as visited
                    Visited[NewY * 25 + NewX] = true;

                    // Record where we came from
                    CameFrom[NewY * 25 + NewX] = Current;

                    // Add to queue
                    Queue.Enqueue(FVector2D(NewX, NewY));
                }
            }
        }
    }

    // If we found a path, reconstruct it
    if (bFoundPath)
    {
        // Clear the current path and add the destination
        CurrentPath.Empty();

        // Start from the destination
        FVector2D Current(EndX, EndY);

        // Build the path backwards
        TArray<FVector2D> ReversePath;
        ReversePath.Add(Current);

        // Keep going until we reach the start
        while (Current.X != StartX || Current.Y != StartY)
        {
            Current = CameFrom[FMath::FloorToInt(Current.Y) * 25 + FMath::FloorToInt(Current.X)];
            ReversePath.Add(Current);
        }

        // Reverse the path to get the correct order
        for (int32 i = ReversePath.Num() - 1; i >= 0; i--)
        {
            CurrentPath.Add(ReversePath[i]);
        }

        UE_LOG(LogTemp, Warning, TEXT("Found path with %d steps"), CurrentPath.Num());
    }
    else
    {
        // Fallback to simple direct path if no valid path found
        UE_LOG(LogTemp, Warning, TEXT("No valid path found, using direct path"));

        CurrentPath.Empty();
        CurrentPath.Add(FVector2D(StartX, StartY));
        CurrentPath.Add(FVector2D(EndX, EndY));
    }

    // Display the path
    if (GridManager)
    {
        // Use true to clear any existing path highlights
        GridManager->HighlightPath(CurrentPath, true);
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
        // Add widget to viewport with high Z-order
        MovementAndAttackWidget->AddToViewport(10000);
        UE_LOG(LogTemp, Warning, TEXT("Movement and Attack widget added to viewport"));

        // Find and bind buttons
        UButton* MoveButton = Cast<UButton>(MovementAndAttackWidget->GetWidgetFromName(TEXT("MoveButton")));
        UButton* AttackButton = Cast<UButton>(MovementAndAttackWidget->GetWidgetFromName(TEXT("AttackButton")));

        if (MoveButton)
        {
            MoveButton->OnClicked.Clear(); // Clear existing bindings
            MoveButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnMoveButtonClicked);
            UE_LOG(LogTemp, Display, TEXT("Move button bound successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MoveButton not found in widget!"));
        }

        if (AttackButton)
        {
            AttackButton->OnClicked.Clear(); // Clear existing bindings
            AttackButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnAttackButtonClicked);
            UE_LOG(LogTemp, Display, TEXT("Attack button bound successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AttackButton not found in widget!"));
        }

        // IMPORTANT: Set the input mode with proper focus
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(MovementAndAttackWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        UE_LOG(LogTemp, Warning, TEXT("Movement and Attack widget set up with proper focus"));
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
