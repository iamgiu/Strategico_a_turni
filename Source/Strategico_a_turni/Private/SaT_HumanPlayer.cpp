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

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultWidgetClass(TEXT("/Game/UI/WBP_UnitSelection"));
    if (DefaultWidgetClass.Succeeded())
    {
        UnitSelectionWidgetClass = DefaultWidgetClass.Class;
    }
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Base initialization
    SelectionState = 0;
    CurrentPhase = EGamePhase::SETUP;
    UnitsToPlace = 2;

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
    // Base functionality
    SelectionState = 0;
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
    UE_LOG(LogTemp, Display, TEXT("OnClick called"));

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
    }
}