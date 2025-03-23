// Fill out your copyright notice in the Description page of Project Settings.

// Fill out your copyright notice in the Description page of Project Settings.

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
        // NEW FUNCTIONALITY FOR PLAYING PHASE
        UE_LOG(LogTemp, Warning, TEXT("AI: In PLAYING phase, starting AI actions..."));

        // Get all AI units
        AIUnits.Empty();
        FindAllAIUnits();

        if (AIUnits.Num() > 0)
        {
            // Start the sequence of actions for all AI units
            UE_LOG(LogTemp, Warning, TEXT("AI: Found %d units to control"), AIUnits.Num());
            CurrentUnitIndex = 0;

            // Schedule the first unit action
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASaT_RandomPlayer::ProcessNextAIUnit, 1.0f, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AI: No units found, ending turn"));
            EndTurn();
        }
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
        GameInstance->SwitchTurn();
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

            // Always end turn after placing one unit
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
        // PLAYING PHASE LOGIC - This should now be handled in ProcessNextAIUnit
        UE_LOG(LogTemp, Warning, TEXT("PlaceRandomUnit called in PLAYING phase - this should not happen"));
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

// Find all AI controlled units
void ASaT_RandomPlayer::FindAllAIUnits()
{
    // Get all unit actors in the world
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    // Filter for AI-controlled units
    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit && !Unit->bIsPlayerUnit && Unit->IsAlive())
        {
            AIUnits.Add(Unit);
            UE_LOG(LogTemp, Warning, TEXT("AI: Found unit %s at position (%d,%d)"),
                *Unit->GetName(), Unit->GridX, Unit->GridY);
        }
    }
}

// Process the next AI unit in sequence
void ASaT_RandomPlayer::ProcessNextAIUnit()
{
    if (!IsMyTurn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Not our turn anymore, aborting unit processing"));
        return;
    }

    if (CurrentUnitIndex < AIUnits.Num())
    {
        AUnit* CurrentUnit = AIUnits[CurrentUnitIndex];
        UE_LOG(LogTemp, Warning, TEXT("AI: Processing unit %d: %s"),
            CurrentUnitIndex, *CurrentUnit->GetName());

        // Make moves with the current unit
        ProcessUnitActions(CurrentUnit);

        // Increment index for next unit and schedule next unit processing
        CurrentUnitIndex++;

        // Schedule next unit processing with delay
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASaT_RandomPlayer::ProcessNextAIUnit, 2.0f, false);
    }
    else
    {
        // All units processed, end turn
        UE_LOG(LogTemp, Warning, TEXT("AI: All units processed, ending turn"));
        EndTurn();
    }
}

// Process actions for a specific AI unit
void ASaT_RandomPlayer::ProcessUnitActions(AUnit* Unit)
{
    if (!Unit || !Unit->IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Unit is invalid or dead"));
        return;
    }

    // First, try to find and attack a player unit if in range
    AUnit* PlayerTarget = FindAttackTarget(Unit);

    // If we found a target, attack it
    if (PlayerTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Unit %s is attacking player unit at (%d,%d)"),
            *Unit->GetName(), PlayerTarget->GridX, PlayerTarget->GridY);

        // Visual indication of target
        if (GridManager)
        {
            // Highlight the target for visual feedback
            GridManager->HighlightCell(PlayerTarget->GridX, PlayerTarget->GridY, true);

            // Schedule to clear the highlight after a delay
            FTimerHandle ClearTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(ClearTimerHandle, [this, PlayerTarget]() {
                if (GridManager && PlayerTarget)
                {
                    GridManager->HighlightCell(PlayerTarget->GridX, PlayerTarget->GridY, false);
                }
                }, 1.0f, false);
        }

        // Perform the attack
        Unit->Attack(PlayerTarget);
    }
    // If no valid attack target, try to move toward a player unit
    else if (!Unit->bHasMovedThisTurn)
    {
        MoveTowardPlayerUnit(Unit);
    }
}

// Find a player unit to attack if any are in range
AUnit* ASaT_RandomPlayer::FindAttackTarget(AUnit* AIUnit)
{
    if (!AIUnit) return nullptr;

    // Get all player units
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    // Find player units in attack range
    TArray<AUnit*> PotentialTargets;

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* PlayerUnit = Cast<AUnit>(UnitActor);
        if (PlayerUnit && PlayerUnit->bIsPlayerUnit && PlayerUnit->IsAlive())
        {
            // Calculate Manhattan distance
            int32 Distance = FMath::Abs(PlayerUnit->GridX - AIUnit->GridX) +
                FMath::Abs(PlayerUnit->GridY - AIUnit->GridY);

            // Check if within attack range
            if (Distance <= AIUnit->RangeAttack)
            {
                PotentialTargets.Add(PlayerUnit);
                UE_LOG(LogTemp, Warning, TEXT("AI: Found potential target at (%d,%d), distance %d, range %d"),
                    PlayerUnit->GridX, PlayerUnit->GridY, Distance, AIUnit->RangeAttack);
            }
        }
    }

    // If we have targets, select the weakest one (or random if same HP)
    if (PotentialTargets.Num() > 0)
    {
        // Sort by HP (weakest first)
        PotentialTargets.Sort([](const AUnit& A, const AUnit& B) {
            return A.Hp < B.Hp;
            });

        return PotentialTargets[0];
    }

    return nullptr;
}

// Move the AI unit toward the closest player unit
void ASaT_RandomPlayer::MoveTowardPlayerUnit(AUnit* AIUnit)
{
    if (!AIUnit || !GridManager) return;

    // Find closest player unit
    AUnit* ClosestUnit = FindClosestPlayerUnit(AIUnit);

    if (!ClosestUnit)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: No player units found to move toward"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("AI: Moving toward player unit at (%d,%d)"),
        ClosestUnit->GridX, ClosestUnit->GridY);

    // Calculate direction toward player unit
    int32 DirX = (ClosestUnit->GridX > AIUnit->GridX) ? 1 :
        (ClosestUnit->GridX < AIUnit->GridX) ? -1 : 0;
    int32 DirY = (ClosestUnit->GridY > AIUnit->GridY) ? 1 :
        (ClosestUnit->GridY < AIUnit->GridY) ? -1 : 0;

    // Create a list of possible moves in priority order (prefer moving on both axes if possible)
    TArray<FVector2D> PossibleMoves;

    // First, try to move diagonally (moving on both X and Y)
    if (DirX != 0 && DirY != 0)
    {
        // Priority 1: Move along X axis
        PossibleMoves.Add(FVector2D(AIUnit->GridX + DirX, AIUnit->GridY));

        // Priority 2: Move along Y axis
        PossibleMoves.Add(FVector2D(AIUnit->GridX, AIUnit->GridY + DirY));
    }
    // If target is directly horizontal
    else if (DirX != 0)
    {
        PossibleMoves.Add(FVector2D(AIUnit->GridX + DirX, AIUnit->GridY));
        // Add some randomness to Y movement if X is the main direction
        PossibleMoves.Add(FVector2D(AIUnit->GridX + DirX, AIUnit->GridY + 1));
        PossibleMoves.Add(FVector2D(AIUnit->GridX + DirX, AIUnit->GridY - 1));
    }
    // If target is directly vertical
    else if (DirY != 0)
    {
        PossibleMoves.Add(FVector2D(AIUnit->GridX, AIUnit->GridY + DirY));
        // Add some randomness to X movement if Y is the main direction
        PossibleMoves.Add(FVector2D(AIUnit->GridX + 1, AIUnit->GridY + DirY));
        PossibleMoves.Add(FVector2D(AIUnit->GridX - 1, AIUnit->GridY + DirY));
    }

    // Try each possible move until we find a valid one
    for (const FVector2D& Move : PossibleMoves)
    {
        int32 NewX = FMath::Clamp(Move.X, 0, 24);  // Clamp to grid boundaries
        int32 NewY = FMath::Clamp(Move.Y, 0, 24);

        // Calculate distance from current position
        int32 MoveDistance = FMath::Abs(NewX - AIUnit->GridX) + FMath::Abs(NewY - AIUnit->GridY);

        // Check if move is valid (within movement range and not occupied)
        if (MoveDistance <= AIUnit->Movement && !GridManager->IsCellOccupied(NewX, NewY))
        {
            // Visual feedback - highlight path
            if (GridManager)
            {
                TArray<FVector2D> Path;
                Path.Add(FVector2D(AIUnit->GridX, AIUnit->GridY));
                Path.Add(FVector2D(NewX, NewY));
                GridManager->HighlightPath(Path, true);

                // Schedule to clear the path highlight
                FTimerHandle ClearPathHandle;
                GetWorld()->GetTimerManager().SetTimer(ClearPathHandle, [this]() {
                    if (GridManager)
                    {
                        GridManager->ClearPathHighlights();
                    }
                    }, 1.0f, false);
            }

            // Move the unit
            UE_LOG(LogTemp, Warning, TEXT("AI: Moving unit from (%d,%d) to (%d,%d)"),
                AIUnit->GridX, AIUnit->GridY, NewX, NewY);

            // Update the grid manager about the move
            if (GridManager)
            {
                // Mark old cell as unoccupied
                // Note: You might need to add a method to GridManager to handle this
                // For now, we'll just mark the new cell as occupied

                // Execute the move
                bool bSuccess = AIUnit->Move(NewX, NewY);

                if (bSuccess)
                {
                    // Mark new cell as occupied
                    GridManager->OccupyCell(NewX, NewY, AIUnit);
                    UE_LOG(LogTemp, Warning, TEXT("AI: Successfully moved unit"));

                    // After moving, check if we can now attack
                    AUnit* NewTarget = FindAttackTarget(AIUnit);
                    if (NewTarget)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("AI: After moving, now attacking player unit at (%d,%d)"),
                            NewTarget->GridX, NewTarget->GridY);

                        // Visual indication of target
                        if (GridManager)
                        {
                            GridManager->HighlightCell(NewTarget->GridX, NewTarget->GridY, true);

                            // Schedule to clear the highlight
                            FTimerHandle ClearHighlightHandle;
                            GetWorld()->GetTimerManager().SetTimer(ClearHighlightHandle, [this, NewTarget]() {
                                if (GridManager && NewTarget)
                                {
                                    GridManager->HighlightCell(NewTarget->GridX, NewTarget->GridY, false);
                                }
                                }, 1.0f, false);
                        }

                        // Perform the attack
                        AIUnit->Attack(NewTarget);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("AI: Failed to move unit"));
                }
            }

            // We found and executed a valid move, so exit
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("AI: Could not find a valid move for unit"));
}

// Find the closest player unit to the given AI unit
AUnit* ASaT_RandomPlayer::FindClosestPlayerUnit(AUnit* AIUnit)
{
    if (!AIUnit) return nullptr;

    AUnit* ClosestUnit = nullptr;
    int32 ClosestDistance = INT_MAX;

    // Get all units
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* PlayerUnit = Cast<AUnit>(UnitActor);
        if (PlayerUnit && PlayerUnit->bIsPlayerUnit && PlayerUnit->IsAlive())
        {
            // Calculate Manhattan distance
            int32 Distance = FMath::Abs(PlayerUnit->GridX - AIUnit->GridX) +
                FMath::Abs(PlayerUnit->GridY - AIUnit->GridY);

            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestUnit = PlayerUnit;
            }
        }
    }

    return ClosestUnit;
}

void ASaT_RandomPlayer::OnWin()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player has won!"));
}

void ASaT_RandomPlayer::OnLose()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player has lost!"));
}