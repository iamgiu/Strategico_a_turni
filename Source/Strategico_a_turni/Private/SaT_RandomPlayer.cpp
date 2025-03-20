// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_RandomPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "GridManager.h"
#include "SaT_GameInstance.h"
#include "Sniper.h"
#include "Brawler.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ASaT_RandomPlayer::ASaT_RandomPlayer()
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

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
}

// Called when the game starts or when spawned
void ASaT_RandomPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Get reference to GameInstance
    GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    // Find grid in scene
    TArray<AActor*> FoundGrids;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
    if (FoundGrids.Num() > 0)
    {
        GridManager = Cast<AGridManager>(FoundGrids[0]);
    }

    // Validate class references
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

// Called every frame
void ASaT_RandomPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASaT_RandomPlayer::OnTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player Turn Started"));

    // Using a timer to give the AI some "thinking" time
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASaT_RandomPlayer::PlaceRandomUnit, 1.5f, false);
}

void ASaT_RandomPlayer::PlaceRandomUnit()
{
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager not found in SaT_RandomPlayer"));

        // Pass turn even in case of error
        if (GameInstance)
        {
            GameInstance->SwitchTurn();
        }
        return;
    }

    // Find an empty cell randomly
    int32 GridX, GridY;
    bool bFoundEmptyCell = FindRandomEmptyCell(GridX, GridY);

    if (bFoundEmptyCell)
    {
        // Randomly decide whether to place a Sniper or Brawler (50% probability)
        bool bIsSniper = FMath::RandBool();

        // Check if class references are valid
        if (bIsSniper && !SniperClass)
        {
            UE_LOG(LogTemp, Error, TEXT("SniperClass is not set! Cannot place Sniper"));
            if (GameInstance)
            {
                GameInstance->SwitchTurn();
            }
            return;
        }

        if (!bIsSniper && !BrawlerClass)
        {
            UE_LOG(LogTemp, Error, TEXT("BrawlerClass is not set! Cannot place Brawler"));
            if (GameInstance)
            {
                GameInstance->SwitchTurn();
            }
            return;
        }

        // Calculate 3D position from grid
        FVector SpawnLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);

        // Configure spawn parameters
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Spawn appropriate unit
        AUnit* Unit = nullptr;

        if (bIsSniper)
        {
            Unit = GetWorld()->SpawnActor<ASniper>(SniperClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            UE_LOG(LogTemp, Warning, TEXT("AI: Sniper placed at %d, %d"), GridX, GridY);
        }
        else
        {
            Unit = GetWorld()->SpawnActor<ABrawler>(BrawlerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            UE_LOG(LogTemp, Warning, TEXT("AI: Brawler placed at %d, %d"), GridX, GridY);
        }

        // Configure unit
        if (Unit)
        {
            Unit->GridX = GridX;
            Unit->GridY = GridY;
            Unit->bIsPlayerUnit = false; // Set as AI unit

            // Mark cell as occupied
            GridManager->OccupyCell(GridX, GridY, Unit);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: No valid empty cell found"));
    }

    // Pass turn
    if (GameInstance)
    {
        GameInstance->SwitchTurn();
    }
}

bool ASaT_RandomPlayer::FindRandomEmptyCell(int32& OutGridX, int32& OutGridY)
{
    if (!GridManager) return false;

    // Grid size
    int32 GridSize = 25; // This should match the size in GridManager

    // Maximum number of attempts to find an empty cell
    const int32 MaxAttempts = 100;

    for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
    {
        // Generate random coordinates
        OutGridX = FMath::RandRange(0, GridSize - 1);
        OutGridY = FMath::RandRange(0, GridSize - 1);

        // Check if cell is free
        if (!GridManager->IsCellOccupied(OutGridX, OutGridY))
        {
            return true;
        }
    }

    return false;
}

void ASaT_RandomPlayer::OnWin()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player has won!"));
}

void ASaT_RandomPlayer::OnLose()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player has lost!"));
}