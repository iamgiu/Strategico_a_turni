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

// Implement HasPlacedAllUnits
bool ASaT_HumanPlayer::HasPlacedAllUnits() const
{
    return PlacedUnitsCount >= UnitsToPlace;
}

void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{
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


void ASaT_HumanPlayer::OnWin()
{
    // Base implementation
}

void ASaT_HumanPlayer::OnLose()
{
    // Base implementation
}

void ASaT_HumanPlayer::OnClick()
{
    // Check if it's this player's turn
    if (!IsMyTurn)
    {
        // Add explicit debug info - check game instance too
        GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
        bool gameInstancePlayerTurn = GameInstance ? GameInstance->bIsPlayerTurn : false;

        UE_LOG(LogTemp, Warning, TEXT("Not your turn! Cannot interact with the game. IsMyTurn=%s, GameInstance.bIsPlayerTurn=%s"),
            IsMyTurn ? TEXT("TRUE") : TEXT("FALSE"),
            gameInstancePlayerTurn ? TEXT("TRUE") : TEXT("FALSE"));
        return; // Exit early if it's not the player's turn
    }

    UE_LOG(LogTemp, Display, TEXT("OnClick called - interaction allowed"));

    // Get player controller
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        FHitResult HitResult;
        PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

        if (HitResult.bBlockingHit)
        {
            // Select cell
            LastSelectedCell = HitResult.Location;

            // Convert 3D position to grid coordinates
            // Use GetXYPositionByRelativeLocation from GridManager if available
            if (GridManager)
            {
                FVector2D GridPosition = GridManager->GetXYPositionByRelativeLocation(HitResult.Location);
                SelectedGridX = FMath::FloorToInt(GridPosition.X);
                SelectedGridY = FMath::FloorToInt(GridPosition.Y);

                UE_LOG(LogTemp, Warning, TEXT("Cell selected in grid coordinates: X=%d, Y=%d"),
                    SelectedGridX, SelectedGridY);

                // Check if cell is occupied using GridManager
                if (!GridManager->IsCellOccupied(SelectedGridX, SelectedGridY))
                {
                    //UE_LOG(LogTemp, Display, TEXT("Empty cell! Showing unit selection widget"));
                    ShowUnitSelectionWidget();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Cell occupied! Cannot place a unit here"));
                }
            }
        }
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