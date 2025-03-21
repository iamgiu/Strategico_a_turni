// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_RandomPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "GridManager.h"
#include "SaT_GameInstance.h"
#include "Sniper.h"
#include "Brawler.h"
#include "UObject/ConstructorHelpers.h"
#include "SaT_GameMode.h"

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

    // Initialize unit placement count
    PlacedUnitsCount = 0;

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
    // Debug - identify which Player object this is
    UE_LOG(LogTemp, Warning, TEXT("OnTurn called on AI Player (this=%p)"), this);

    // Get current game state
    if (!GameInstance)
    {
        GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
        if (!GameInstance)
        {
            UE_LOG(LogTemp, Error, TEXT("AI: GameInstance not found in OnTurn!"));
            return;
        }
    }

    // Very important - check if it's really our turn
    if (GameInstance->bIsPlayerTurn)
    {
        UE_LOG(LogTemp, Error, TEXT("AI: OnTurn called but GameInstance says it's Human's turn! Ignoring."));
        IsMyTurn = false; // Ensure flag is correct
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("AI Player Turn Started - Verification PASSED"));

    // Explicitly set this player's turn flag to true
    IsMyTurn = true;
    UE_LOG(LogTemp, Warning, TEXT("AI: IsMyTurn set to TRUE"));

    // Check the game phase
    EGamePhase CurrentPhase = GameInstance->GetGamePhase();
    UE_LOG(LogTemp, Warning, TEXT("AI Turn - Current phase: %s"),
        CurrentPhase == EGamePhase::SETUP ? TEXT("SETUP") :
        CurrentPhase == EGamePhase::PLAYING ? TEXT("PLAYING") : TEXT("GAMEOVER"));

    if (CurrentPhase == EGamePhase::SETUP)
    {
        // Check if we already placed all units
        if (PlacedUnitsCount >= 2)
        {
            UE_LOG(LogTemp, Warning, TEXT("AI: Already placed all units, ending turn"));

            // End turn with a slight delay
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
                {
                    // CHANGED: Using EndTurn() instead of GameInstance->SwitchTurn()
                    EndTurn();
                }, 0.5f, false);
            return;
        }

        // In SETUP phase, use a timer for unit placement
        FTimerHandle TimerHandle;
        UE_LOG(LogTemp, Warning, TEXT("AI: Scheduling unit placement in 1.5 seconds..."));
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASaT_RandomPlayer::PlaceRandomUnit, 1.5f, false);
    }
    else if (CurrentPhase == EGamePhase::PLAYING)
    {
        // In PLAYING phase, just end turn for now
        UE_LOG(LogTemp, Warning, TEXT("AI: In PLAYING phase, ending turn..."));

        // For now just end turn after a short delay
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                UE_LOG(LogTemp, Warning, TEXT("AI: Ending turn in PLAYING phase"));
                // CHANGED: Using EndTurn() instead of GameInstance->SwitchTurn()
                EndTurn();
            }, 1.0f, false);
    }
}

void ASaT_RandomPlayer::EndTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player ending turn"));

    // Update the GameInstance with our placed units count
    if (GameInstance)
    {
        GameInstance->AIUnitsPlaced = PlacedUnitsCount;
    }

    // Set our own IsMyTurn to false BEFORE calling GameMode->EndTurn()
    IsMyTurn = false;
    UE_LOG(LogTemp, Warning, TEXT("AI Player setting IsMyTurn to FALSE"));

    // Get the game mode and tell it to end the turn
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI Player calling GameMode->EndTurn()"));
        GameMode->EndTurn();
    }
    else if (GameInstance)
    {
        // Fallback if we can't get GameMode
        UE_LOG(LogTemp, Warning, TEXT("GameMode not found, calling GameInstance->SwitchTurn() directly"));
        GameInstance->SwitchTurn(); // CORRECTED from EndTurn();
    }
}

void ASaT_RandomPlayer::PlaceRandomUnit()
{
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager not found in SaT_RandomPlayer"));
        EndTurn();
        return;
    }

    // Get the current phase
    EGamePhase CurrentPhase = EGamePhase::SETUP;
    if (GameInstance)
    {
        CurrentPhase = GameInstance->GetGamePhase();
    }

    // Handle differently based on game phase
    if (CurrentPhase == EGamePhase::SETUP)
    {
        // SETUP PHASE LOGIC
        // Check if AI has already placed all units
        if (PlacedUnitsCount >= 2)
        {
            UE_LOG(LogTemp, Warning, TEXT("AI has already placed all units! Passing turn."));
            EndTurn();
            return;
        }

        // Find an empty cell randomly
        int32 GridX, GridY;
        bool bFoundEmptyCell = FindRandomEmptyCell(GridX, GridY);

        if (bFoundEmptyCell)
        {
            // Determine which unit to place based on what's already placed
            bool bIsSniper = (PlacedUnitsCount == 0); // First unit is Sniper, second is Brawler

            // Check if class references are valid
            if (bIsSniper && !SniperClass)
            {
                UE_LOG(LogTemp, Error, TEXT("SniperClass is not set! Cannot place Sniper"));
                EndTurn();
                return;
            }
            if (!bIsSniper && !BrawlerClass)
            {
                UE_LOG(LogTemp, Error, TEXT("BrawlerClass is not set! Cannot place Brawler"));
                EndTurn();
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

                // Increment placement count and update GameInstance
                PlacedUnitsCount++;
                UE_LOG(LogTemp, Warning, TEXT("AI has placed %d/2 units"), PlacedUnitsCount);

                // Update GameInstance
                if (GameInstance)
                {
                    GameInstance->AIUnitsPlaced = PlacedUnitsCount;
                    UE_LOG(LogTemp, Warning, TEXT("Updated GameInstance AIUnitsPlaced = %d"), GameInstance->AIUnitsPlaced);
                }
            }

            // CHANGE HERE - Always end turn after placing one unit
            UE_LOG(LogTemp, Warning, TEXT("AI passing turn after unit placement"));
            EndTurn();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AI couldn't find an empty cell for unit placement!"));
            EndTurn();
        }
    }
    else if (CurrentPhase == EGamePhase::PLAYING)
    {
        // PLAYING PHASE LOGIC
        UE_LOG(LogTemp, Warning, TEXT("AI Turn in PLAYING phase - passing turn"));
        EndTurn();
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