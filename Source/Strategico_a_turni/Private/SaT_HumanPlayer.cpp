// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_HumanPlayer.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GridManager.h"
#include "Sat_GameMode.h"
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

// Constructor - initializes default values, components, and widget classes
ASaT_HumanPlayer::ASaT_HumanPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // create a camera component
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    //set the camera as RootComponent
    SetRootComponent(Camera);

    UnitSelectionWidget = nullptr;

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

// Called when the game starts - initializes player state, references, and validates components
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
        SniperClass = ASniper::StaticClass();
    }

    if (!BrawlerClass)
    {
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
    }
}

// Called every frame to update player state
void ASaT_HumanPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// Configures input bindings for the player
void ASaT_HumanPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// Called when it's the player's turn - prepares UI and initializes turn state
void ASaT_HumanPlayer::OnTurn()
{
    // Make sure we have a reference to the GameInstance
    if (!GameInstance)
    {
        GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    }

    if (GameInstance && !GameInstance->bIsPlayerTurn)
    {
        IsMyTurn = false; // Ensure flag is correct
        return;
    }

    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    UnitAttackedThisTurn.Empty();

    // Reset input mode at start of turn
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        // Use GameAndUI consistently here too
        FInputModeGameAndUI InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }

    IsMyTurn = true;

    if (GameInstance)
    {
        // Update the class member directly instead of creating a local variable
        CurrentPhase = GameInstance->GetGamePhase();

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
                // End turn automatically
                EndTurn();
            }
            else
            {
                // Can place more units
                UE_LOG(LogTemp, Warning, TEXT("Human player can place more units (%d/%d placed)"),
                    PlacedUnitsCount, UnitsToPlace);
            }

            // During SETUP phase, we don't want to show the End Turn button
            // If it exists, remove it
            if (EndTurnWidget)
            {
                EndTurnWidget->RemoveFromParent();
                EndTurnWidget = nullptr;
            }
        }
        else if (CurrentPhase == EGamePhase::PLAYING)
        {
            // Only show the end turn button during the PLAYING phase
            ShowEndTurnButton();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in Human Player OnTurn!"));
    }
}

// Implementation for when the player wins the game
void ASaT_HumanPlayer::OnWin()
{
    UE_LOG(LogTemp, Warning, TEXT("Human Player has won!"));
}

// Implementation for when the player loses the game
void ASaT_HumanPlayer::OnLose()
{
    UE_LOG(LogTemp, Warning, TEXT("Human Player has lost!"));
}

// Implementation for when the game ends in a draw
void ASaT_HumanPlayer::OnDraw()
{
    UE_LOG(LogTemp, Warning, TEXT("Human Player: Game ended in a draw"));
}

// Checks if player has placed all required units during setup phase
bool ASaT_HumanPlayer::HasPlacedAllUnits() const
{
    return PlacedUnitsCount >= UnitsToPlace;
}

// Places a new unit on the grid at the specified coordinates
void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{

    if (PlacedUnitsCount >= UnitsToPlace)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot place more units - already placed maximum (%d/%d)"),
            PlacedUnitsCount, UnitsToPlace);
        return;
    }

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
    }
    else
    {
        // Fallback if GridManager is invalid
        SpawnLocation = FVector(GridX * 100.0f, GridY * 100.0f, 0.0f);
    }

    // Create appropriate unit
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AUnit* PlacedUnit = nullptr;

    if (bIsSniper)
    {
        PlacedUnit = GetWorld()->SpawnActor<ASniper>(SniperClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    }
    else
    {
        PlacedUnit = GetWorld()->SpawnActor<ABrawler>(BrawlerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
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

        // Update the GameInstance to track human units placed
        if (GameInstance)
        {
            GameInstance->HumanUnitsPlaced = PlacedUnitsCount;
        }

        AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
        if (GameMode)
        {
            GameMode->UpdateGameHUD();
            FString UnitType = bIsSniper ? TEXT("Sniper") : TEXT("Brawler");
            GameMode->AddFormattedMoveToLog(
                true, // IsPlayerUnit
                UnitType,
                TEXT("Place"),
                FVector2D(0, 0), // No from position for placement
                FVector2D(GridX, GridY)
            );
        }

        // END TURN AUTOMATICALLY AFTER PLACING A UNIT
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASaT_HumanPlayer::EndTurn, 0.5f, false);
    }
}

// Handles mouse click input during gameplay
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

    FHitResult HitResult;
    bool bHitSuccess = PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

    if (bHitSuccess)
    {
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
            if (CurrentPhase == EGamePhase::SETUP)
            {
                // Check if the tile is already occupied
                if (ClickedTile->bIsOccupied)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Tile is already occupied!"));
                    return;
                }

                SelectedGridX = ClickedTile->GridX;
                SelectedGridY = ClickedTile->GridY;

                // Show unit selection widget
                ShowUnitSelectionWidget();
            }
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

// Processes clicks during the playing phase - handles unit selection, movement, and attacks
void ASaT_HumanPlayer::HandlePlayingPhaseClick(ATile* ClickedTile)
{
    if (!ClickedTile)
        return;

    // Check if there's a unit on this tile
    AUnit* ClickedUnit = ClickedTile->OccupyingUnit;

    // If we're clicking on empty tile and we're not in move or attack mode,
    // deselect the current unit if there is one
    if (!ClickedUnit && !bMoveMode && !bAttackMode && SelectedUnit)
    {
        // Deselect the current unit
        SelectedUnit->UnshowSelected();

        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            GameMode->SetSelectedUnit(nullptr);
        }

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

                ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GetWorld()->GetAuthGameMode());
                if (GameMode)
                {
                    GameMode->SetSelectedUnit(SelectedUnit);
                }

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
                bool bAttackSuccess = TryAttackUnit(SelectedUnit, ClickedUnit);
                if (bAttackSuccess)
                {
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


                    // IMPORTANT: Save the old position
                    int32 OldGridX = UnitToMove->GridX;
                    int32 OldGridY = UnitToMove->GridY;

                    // Update grid - free old cell FIRST
                    GridManager->OccupyCell(OldGridX, OldGridY, nullptr);

                    // Update unit position
                    FVector NewLocation = GridManager->GetWorldLocationFromGrid(ClickedTile->GridX, ClickedTile->GridY);
                    UnitToMove->GridX = ClickedTile->GridX;
                    UnitToMove->GridY = ClickedTile->GridY;
                    UnitToMove->SetActorLocation(NewLocation);

                    // Occupy new cell
                    GridManager->OccupyCell(ClickedTile->GridX, ClickedTile->GridY, UnitToMove);

                    // Mark as moved
                    UnitToMove->bHasMovedThisTurn = true;

                    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GetWorld()->GetAuthGameMode());
                    if (GameMode)
                    {
                        GameMode->UpdateGameHUD();

                        FString UnitType = Cast<ASniper>(UnitToMove) ? TEXT("Sniper") : TEXT("Brawler");
                        GameMode->AddFormattedMoveToLog(
                            true, // IsPlayerUnit
                            UnitType,
                            TEXT("Move"),
                            FVector2D(CurrentPath[0].X, CurrentPath[0].Y), // Use first position in path as starting point
                            FVector2D(ClickedTile->GridX, ClickedTile->GridY)
                        );
                    }

                    UE_LOG(LogTemp, Warning, TEXT("Move successful!"));

                    // Reset move mode
                    bMoveMode = false;
                    // Show the path for a while before clearing it
                    FTimerHandle ClearPathTimer;
                    GetWorld()->GetTimerManager().SetTimer(ClearPathTimer, [this]() {
                        ClearAllHighlightsAndPaths();
                    }, 5.0f, false);

                    // Show action widget again to allow attack after move
                    ShowMovementAndAttackWidget(UnitToMove);
                }
            }
            else
            {
                // If clicked outside movement range, cancel move mode
                bMoveMode = false;
            }
        }
        else if (bAttackMode && SelectedUnit)
        {
            // If clicked empty space while in attack mode, cancel attack mode
            bAttackMode = false;
            ClearAllHighlightsAndPaths();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Clicked on empty cell but not in move mode or no unit selected"));
        }
    }
}

// Deselects the currently selected unit and clears highlights
void ASaT_HumanPlayer::DeselectCurrentUnit()
{
    if (SelectedUnit)
    {
        SelectedUnit->UnshowSelected();

        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            GameMode->SetSelectedUnit(nullptr);
        }

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

// Shows the widget for selecting unit type during placement
void ASaT_HumanPlayer::ShowUnitSelectionWidget()
{

    if (GameInstance)
    {
        bool bAlreadyPlacedThisTurn = false;

        // If it's a setup phase, check against turns
        if (GameInstance->GetGamePhase() == EGamePhase::SETUP)
        {
            int32 CurrentTurnNumber = GameInstance->CurrentTurnNumber;
            int32 ExpectedUnits = 2; 

            // If we've placed the expected number of units for this turn, don't show widget
            if (PlacedUnitsCount >= ExpectedUnits)
            {
                UE_LOG(LogTemp, Warning, TEXT("Already placed a unit this turn. End your turn to continue."));
                return;
            }
        }
    }

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
            }
            else if (Cast<ABrawler>(Unit))
            {
                bBrawlerPlaced = true;
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
        UnitSelectionWidget->AddToViewport(10000);

        // Find buttons in widget
        UButton* SniperButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("SniperButton")));
        UButton* BrawlerButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("BrawlerButton")));

        if (SniperButton && !bSniperPlaced)
        {
            SniperButton->OnClicked.Clear(); // Clear any existing bindings
            SniperButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetSniperSelected);
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

        InputMode.SetWidgetToFocus(UnitSelectionWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        // Apply input mode
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not create unit selection widget!"));
    }
}

// Handler for when the Sniper unit type is selected from the widget
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

// Handler for when the Brawler unit type is selected from the widget
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

// Ends the current player's turn and passes control to the AI
void ASaT_HumanPlayer::EndTurn()
{
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

    // Hide the End Turn button explicitly when ending turn
    if (EndTurnWidget)
    {
        EndTurnWidget->RemoveFromParent();
        EndTurnWidget = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("End Turn button removed at end of turn"));
    }

    // Restore normal input mode
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = true; // Keep cursor visible
    }

    IsMyTurn = false;

    // Get the game mode and tell it to end the turn
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        GameMode->EndTurn();
    }
    else
    {
        GameInstance->SwitchTurn();
    }
}

// Shows or hides the end turn button based on game state
void ASaT_HumanPlayer::ShowEndTurnButton()
{
    // Only show the End Turn button in the PLAYING phase and when it's the player's turn
    if (!GameInstance)
    {
        GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    }

    // Check if it's in the playing phase and it's the player's turn
    if (GameInstance)
    {
        EGamePhase CurrentGamePhase = GameInstance->GetGamePhase();
        bool bIsPlayerTurn = GameInstance->bIsPlayerTurn;

        // Only show the button during the PLAYING phase and when it's the player's turn
        if (CurrentGamePhase != EGamePhase::PLAYING || !bIsPlayerTurn)
        {
            // Hide the button if it exists
            if (EndTurnWidget)
            {
                EndTurnWidget->RemoveFromParent();
                EndTurnWidget = nullptr;
            }
            return;
        }
    }

    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot show End Turn button: Player Controller is NULL"));
        return;
    }

    // Create widget
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

        EndTurnWidget->AddToViewport(10000);

        UButton* EndTurnBtn = Cast<UButton>(EndTurnWidget->GetWidgetFromName(TEXT("EndTurnButton")));
        if (EndTurnBtn)
        {
            EndTurnBtn->OnClicked.Clear(); // Clear any existing bindings
            EndTurnBtn->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::EndTurn);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find EndTurnButton in widget!"));
        }

        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(EndTurnWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create End Turn widget"));
    }
}

// Shows the movement and attack options widget for the selected unit
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
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MoveButton not found in widget!"));
        }

        if (AttackButton)
        {
            AttackButton->OnClicked.Clear(); // Clear existing bindings
            AttackButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnAttackButtonClicked);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AttackButton not found in widget!"));
        }

        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(MovementAndAttackWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create MovementAndAttack widget!"));
    }
}

// Handler for when the Move button is clicked - shows movement range
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
    }

    // Hide widget after selection
    if (MovementAndAttackWidget)
    {
        MovementAndAttackWidget->RemoveFromParent();
        MovementAndAttackWidget = nullptr;
    }
}

// Handler for when the Attack button is clicked - shows attack range
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
    }

    // Hide widget after selection
    if (MovementAndAttackWidget)
    {
        MovementAndAttackWidget->RemoveFromParent();
        MovementAndAttackWidget = nullptr;
    }
}

// Calculates and highlights the valid movement range for a unit
void ASaT_HumanPlayer::ShowMovementRange(AUnit * Unit)
{
    if (!Unit || !GridManager)
    {
        return;
    }

    // Clear previous highlights
    GridManager->ClearAllHighlights();

    // Get unit's current position and movement range
    int32 UnitX = Unit->GridX;
    int32 UnitY = Unit->GridY;
    int32 MovementRange = Unit->Movement;

    // Dijkstra's algorithm for finding reachable tiles
    // Initialize distance array (distance from start position)
    int32 Distance[25][25];
    for (int32 x = 0; x < 25; x++)
    {
        for (int32 y = 0; y < 25; y++)
        {
            Distance[x][y] = INT_MAX; // "Infinity"
        }
    }

    // Start position has distance 0
    Distance[UnitX][UnitY] = 0;

    // Priority queue for Dijkstra
    TArray<TPair<int32, FVector2D>> Queue;
    Queue.Add(TPair<int32, FVector2D>(0, FVector2D(UnitX, UnitY)));

    // Direction vectors for 4-way movement
    int32 dx[] = { 1, -1, 0, 0 };
    int32 dy[] = { 0, 0, 1, -1 };

    while (Queue.Num() > 0)
    {
        // Find smallest distance node
        int32 SmallestIdx = 0;
        for (int32 i = 1; i < Queue.Num(); i++)
        {
            if (Queue[i].Key < Queue[SmallestIdx].Key)
            {
                SmallestIdx = i;
            }
        }

        // Get and remove closest node
        TPair<int32, FVector2D> Current = Queue[SmallestIdx];
        Queue.RemoveAt(SmallestIdx);

        int32 CurrentX = FMath::FloorToInt(Current.Value.X);
        int32 CurrentY = FMath::FloorToInt(Current.Value.Y);
        int32 CurrentDist = Current.Key;

        // If we've exceeded movement range, don't process this node further
        if (CurrentDist > MovementRange)
            continue;

        // Try all four directions
        for (int32 i = 0; i < 4; i++)
        {
            int32 NewX = CurrentX + dx[i];
            int32 NewY = CurrentY + dy[i];

            // Skip if invalid position
            if (!GridManager->IsValidPosition(FVector2D(NewX, NewY)))
                continue;

            // Get the tile
            ATile* Tile = nullptr;
            if (GridManager->TileMap.Contains(FVector2D(NewX, NewY)))
                Tile = GridManager->TileMap[FVector2D(NewX, NewY)];

            // Skip if tile is an obstacle or occupied
            if (!Tile || Tile->bIsObstacle || (Tile->bIsOccupied && !(NewX == UnitX && NewY == UnitY)))
                continue;

            // Cost to move to this tile is 1
            int32 NewDist = CurrentDist + 1;

            // If we found a shorter path, update distance
            if (NewDist < Distance[NewX][NewY])
            {
                Distance[NewX][NewY] = NewDist;
                Queue.Add(TPair<int32, FVector2D>(NewDist, FVector2D(NewX, NewY)));
            }
        }
    }

    // Highlight all reachable cells
    int32 HighlightCount = 0;
    for (int32 x = 0; x < 25; x++)
    {
        for (int32 y = 0; y < 25; y++)
        {
            if (Distance[x][y] <= MovementRange && Distance[x][y] > 0)
            {
                GridManager->HighlightCell(x, y, true);
                HighlightCount++;
            }
        }
    }
}

// Attempts to move a unit to the specified grid location
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

// Calculates the optimal path between two grid positions
void ASaT_HumanPlayer::CalculatePath(int32 StartX, int32 StartY, int32 EndX, int32 EndY)
{
    // Clear existing path
    CurrentPath.Empty();

    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager is NULL in CalculatePath"));
        return;
    }

    // Check if destination is valid
    if (!GridManager->IsValidPosition(FVector2D(EndX, EndY)))
    {
        UE_LOG(LogTemp, Error, TEXT("Destination coordinates are outside the grid"));
        return;
    }

    ATile* DestTile = nullptr;
    if (GridManager->TileMap.Contains(FVector2D(EndX, EndY)))
    {
        DestTile = GridManager->TileMap[FVector2D(EndX, EndY)];
        if (DestTile && (DestTile->bIsObstacle || DestTile->bIsOccupied))
        {
            UE_LOG(LogTemp, Warning, TEXT("Destination is occupied or an obstacle"));
            return;
        }
    }

    // Setup Dijkstra's algorithm for pathfinding
    int32 Distance[25][25];
    FVector2D Previous[25][25];
    bool Visited[25][25] = { false };

    // Initialize distance array
    for (int32 x = 0; x < 25; x++)
    {
        for (int32 y = 0; y < 25; y++)
        {
            Distance[x][y] = INT_MAX;
            Previous[x][y] = FVector2D(-1, -1); // Invalid previous position
        }
    }

    // Start position has distance 0
    Distance[StartX][StartY] = 0;

    // Direction vectors for 4-way movement
    int32 dx[] = { 0, 1, 0, -1 };
    int32 dy[] = { -1, 0, 1, 0 };

    // Dijkstra's algorithm
    for (int32 i = 0; i < 25 * 25; i++) // Max iterations
    {
        // Find unvisited node with smallest distance
        int32 MinDist = INT_MAX;
        int32 MinX = -1, MinY = -1;

        for (int32 x = 0; x < 25; x++)
        {
            for (int32 y = 0; y < 25; y++)
            {
                if (!Visited[x][y] && Distance[x][y] < MinDist)
                {
                    MinDist = Distance[x][y];
                    MinX = x;
                    MinY = y;
                }
            }
        }

        // If no more nodes to process or we reached destination
        if (MinX == -1 || (MinX == EndX && MinY == EndY))
            break;

        // Mark as visited
        Visited[MinX][MinY] = true;

        // Try all four directions
        for (int32 dir = 0; dir < 4; dir++)
        {
            int32 NewX = MinX + dx[dir];
            int32 NewY = MinY + dy[dir];

            // Skip if outside grid
            if (!GridManager->IsValidPosition(FVector2D(NewX, NewY)))
                continue;

            // Skip if already visited
            if (Visited[NewX][NewY])
                continue;

            // Skip obstacles
            ATile* Tile = nullptr;
            if (GridManager->TileMap.Contains(FVector2D(NewX, NewY)))
                Tile = GridManager->TileMap[FVector2D(NewX, NewY)];

            if (!Tile || Tile->bIsObstacle || (Tile->bIsOccupied && !(NewX == EndX && NewY == EndY)))
                continue;

            // Calculate new distance (cost is 1 per step)
            int32 NewDist = Distance[MinX][MinY] + 1;

            // If found shorter path
            if (NewDist < Distance[NewX][NewY])
            {
                Distance[NewX][NewY] = NewDist;
                Previous[NewX][NewY] = FVector2D(MinX, MinY);
            }
        }
    }

    // Check if we found a path to the destination
    if (Distance[EndX][EndY] == INT_MAX)
    {
        // Add start point for fallback visualization
        CurrentPath.Add(FVector2D(StartX, StartY));
        return;
    }

    // Reconstruct path from end to start
    TArray<FVector2D> ReversePath;
    int32 CurrentX = EndX;
    int32 CurrentY = EndY;

    while (CurrentX != -1 && CurrentY != -1)
    {
        ReversePath.Add(FVector2D(CurrentX, CurrentY));

        // If we reached the start
        if (CurrentX == StartX && CurrentY == StartY)
            break;

        // Get previous position
        FVector2D PrevPos = Previous[CurrentX][CurrentY];
        CurrentX = FMath::FloorToInt(PrevPos.X);
        CurrentY = FMath::FloorToInt(PrevPos.Y);
    }

    // Reverse to get start-to-end path
    for (int32 i = ReversePath.Num() - 1; i >= 0; i--)
    {
        CurrentPath.Add(ReversePath[i]);
    }

    // Visualize the path
    if (CurrentPath.Num() > 0 && GridManager)
    {
        GridManager->HighlightPath(CurrentPath, true);
    }
}

// Clears the current path highlighting
void ASaT_HumanPlayer::ClearPath()
{
    CurrentPath.Empty();

    if (GridManager)
    {
        GridManager->ClearPathHighlights();
    }
}

// Removes all highlights and path visualizations from the grid
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
    }
}

// Calculates and highlights valid attack targets for a unit
void ASaT_HumanPlayer::ShowAttackRange(AUnit* Unit)
{
    if (!Unit || !GridManager)
    {
        return;
    }

    GridManager->ClearAllHighlights();

    // Get the unit's current position and attack range
    int32 UnitX = Unit->GridX;
    int32 UnitY = Unit->GridY;
    int32 AttackRange = Unit->RangeAttack;

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
                        GridManager->HighlightCell(X, Y, true);
                        HighlightedCount++;
                    }
                }
            }
        }
    }
}

// Attempts to attack a target unit and processes the results
bool ASaT_HumanPlayer::TryAttackUnit(AUnit* AttackingUnit, AUnit* TargetUnit)
{
    if (!AttackingUnit || !TargetUnit || !GridManager)
    {
        return false;
    }

    // Check if attacking unit has already attacked this turn
    if (AttackingUnit->bHasAttackedThisTurn)
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
        // Save the target's health before attack for damage calculation
        int32 TargetHpBefore = TargetUnit->Hp;

        // Perform the attack
        bool bAttackSuccess = AttackingUnit->Attack(TargetUnit);

        if (bAttackSuccess)
        {
            AttackingUnit->bHasAttackedThisTurn = true;

            // Calculate damage dealt
            int32 DamageDealt = TargetHpBefore - TargetUnit->Hp;

            // Update GameHUD and log
            ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GetWorld()->GetAuthGameMode());
            if (GameMode)
            {
                GameMode->UpdateGameHUD();

                FString AttackerType = Cast<ASniper>(AttackingUnit) ? TEXT("Sniper") : TEXT("Brawler");
                GameMode->AddFormattedMoveToLog(
                    true, // IsPlayerUnit
                    AttackerType,
                    TEXT("Attack"),
                    FVector2D(AttackingUnit->GridX, AttackingUnit->GridY),
                    FVector2D(TargetUnit->GridX, TargetUnit->GridY),
                    DamageDealt
                );
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

// Helper method to find a unit at the specified grid coordinates
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

