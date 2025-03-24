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

                AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
                ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
                if (GameMode)
                {
                    GameMode->UpdateGameHUD();
                    // Use the new formatted logging method
                    FString UnitType = bIsSniper ? TEXT("Sniper") : TEXT("Brawler");
                    GameMode->AddFormattedMoveToLog(
                        false, // IsAIUnit
                        UnitType,
                        TEXT("Place"),
                        FVector2D(0, 0), // No from position for placement
                        FVector2D(GridX, GridY)
                    );
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

    // Debug the unit properties
    UE_LOG(LogTemp, Warning, TEXT("AI: Processing %s with Movement=%d, RangeAttack=%d, HP=%d"),
        *Unit->GetName(), Unit->Movement, Unit->RangeAttack, Unit->Hp);

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

        AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
        if (GameMode)
        {
            GameMode->UpdateGameHUD();

            FString AttackerType = Cast<ASniper>(Unit) ? TEXT("Sniper") : TEXT("Brawler");
            int32 TargetHpBefore = PlayerTarget->Hp;

            Unit->Attack(PlayerTarget);

            // Calculate damage dealt
            int32 DamageDealt = TargetHpBefore - PlayerTarget->Hp;

            GameMode->AddFormattedMoveToLog(
                false, // IsPlayerUnit = false for AI units
                AttackerType,
                TEXT("Attack"),
                FVector2D(Unit->GridX, Unit->GridY),
                FVector2D(PlayerTarget->GridX, PlayerTarget->GridY),
                DamageDealt
            );
        }
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

    // First, check if unit has already moved this turn
    if (AIUnit->bHasMovedThisTurn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Unit %s has already moved this turn"), *AIUnit->GetName());
        return;
    }

    // Find closest player unit
    AUnit* ClosestUnit = FindClosestPlayerUnit(AIUnit);

    if (!ClosestUnit)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: No player units found to move toward"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("AI: Unit %s with movement range %d attempting to move toward player unit at (%d,%d)"),
        *AIUnit->GetName(), AIUnit->Movement, ClosestUnit->GridX, ClosestUnit->GridY);

    // Get the unit's movement range - ensure we're using the correct value
    int32 MovementRange = AIUnit->Movement;
    UE_LOG(LogTemp, Warning, TEXT("AI: Unit movement range is %d"), MovementRange);

    // Store valid move options
    struct FMoveOption
    {
        int32 X;
        int32 Y;
        int32 DistanceToTarget;
    };

    TArray<FMoveOption> ValidMoves;

    // Check all cells within movement range using BFS
    TArray<bool> Visited;
    Visited.SetNumZeroed(25 * 25); // For a 25x25 grid

    TQueue<FMoveOption> Queue;
    Queue.Enqueue(FMoveOption{ AIUnit->GridX, AIUnit->GridY, 0 });
    Visited[AIUnit->GridY * 25 + AIUnit->GridX] = true;

    // Direction vectors for the four cardinal directions
    int32 dx[] = { 1, -1, 0, 0 };
    int32 dy[] = { 0, 0, 1, -1 };

    while (!Queue.IsEmpty())
    {
        FMoveOption Current;
        Queue.Dequeue(Current);

        // Skip the starting position
        if (Current.X != AIUnit->GridX || Current.Y != AIUnit->GridY)
        {
            // Calculate distance to target from this position
            int32 DistToTarget = FMath::Abs(Current.X - ClosestUnit->GridX) +
                FMath::Abs(Current.Y - ClosestUnit->GridY);

            // Add this position as a valid move
            Current.DistanceToTarget = DistToTarget;
            ValidMoves.Add(Current);
        }

        // Don't explore further if we've used all movement
        if (Current.DistanceToTarget >= MovementRange)
            continue;

        // Try all four directions
        for (int32 i = 0; i < 4; i++)
        {
            int32 NewX = Current.X + dx[i];
            int32 NewY = Current.Y + dy[i];

            // Check if new position is valid and not visited
            if (NewX >= 0 && NewX < 25 && NewY >= 0 && NewY < 25 &&
                !Visited[NewY * 25 + NewX] && !GridManager->IsCellOccupied(NewX, NewY))
            {
                Visited[NewY * 25 + NewX] = true;
                Queue.Enqueue(FMoveOption{ NewX, NewY, Current.DistanceToTarget + 1 });
            }
        }
    }

    // Display all valid moves (for debugging)
    UE_LOG(LogTemp, Warning, TEXT("AI: Found %d valid moves within range %d"), ValidMoves.Num(), MovementRange);

    // Find the best move - closest to the target unit
    FMoveOption BestMove{ AIUnit->GridX, AIUnit->GridY, INT_MAX };
    bool bFoundMove = false;

    for (const FMoveOption& Move : ValidMoves)
    {
        // Skip the starting position
        if (Move.X == AIUnit->GridX && Move.Y == AIUnit->GridY)
            continue;

        // Calculate distance from this position to the target
        int32 DistToTarget = FMath::Abs(Move.X - ClosestUnit->GridX) +
            FMath::Abs(Move.Y - ClosestUnit->GridY);

        // Check if this move gets us closer to the target
        if (DistToTarget < BestMove.DistanceToTarget)
        {
            BestMove = Move;
            bFoundMove = true;
        }
    }

    // If we found a valid move, execute it
    if (bFoundMove)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Best move found at (%d,%d), distance to target: %d"),
            BestMove.X, BestMove.Y, BestMove.DistanceToTarget);

        // Create a detailed path for visualization
        TArray<FVector2D> Path;

        // Always add the starting position
        Path.Add(FVector2D(AIUnit->GridX, AIUnit->GridY));

        // Initialize current position trackers
        int32 CurrentX = AIUnit->GridX;
        int32 CurrentY = AIUnit->GridY;

        // Create a path from start to destination, showing each step
        // First move horizontally to align X coordinate
        while (CurrentX != BestMove.X)
        {
            // Move one cell at a time in the right direction
            CurrentX += (CurrentX < BestMove.X) ? 1 : -1;

            // Skip adding this point if it's occupied (except for destination)
            if (!GridManager->IsCellOccupied(CurrentX, CurrentY) ||
                (CurrentX == BestMove.X && CurrentY == BestMove.Y))
            {
                Path.Add(FVector2D(CurrentX, CurrentY));
            }
        }

        // Then move vertically to align Y coordinate
        while (CurrentY != BestMove.Y)
        {
            // Move one cell at a time in the right direction
            CurrentY += (CurrentY < BestMove.Y) ? 1 : -1;

            // Skip adding this point if it's occupied (except for destination)
            if (!GridManager->IsCellOccupied(CurrentX, CurrentY) ||
                (CurrentX == BestMove.X && CurrentY == BestMove.Y))
            {
                Path.Add(FVector2D(CurrentX, CurrentY));
            }
        }

        // Visualize the complete path
        if (GridManager && Path.Num() > 1)
        {
            UE_LOG(LogTemp, Warning, TEXT("AI: Visualizing path with %d points"), Path.Num());

            // Highlight the path
            GridManager->HighlightPath(Path, true);

            // Schedule to clear the path highlight after a delay
            FTimerHandle ClearPathHandle;
            GetWorld()->GetTimerManager().SetTimer(ClearPathHandle, [this]() {
                if (GridManager)
                {
                    GridManager->ClearPathHighlights();
                }
                }, 2.0f, false); // Show the path for 2 seconds for better visibility
        }

        // Free the current cell before moving
        GridManager->OccupyCell(AIUnit->GridX, AIUnit->GridY, nullptr);

        // Move the unit
        UE_LOG(LogTemp, Warning, TEXT("AI: Moving unit from (%d,%d) to (%d,%d)"),
            AIUnit->GridX, AIUnit->GridY, BestMove.X, BestMove.Y);

        // Update the grid position
        FVector NewLocation = GridManager->GetWorldLocationFromGrid(BestMove.X, BestMove.Y);
        AIUnit->GridX = BestMove.X;
        AIUnit->GridY = BestMove.Y;
        AIUnit->SetActorLocation(NewLocation);

        // Mark the unit as moved
        AIUnit->bHasMovedThisTurn = true;

        // Mark new cell as occupied
        GridManager->OccupyCell(BestMove.X, BestMove.Y, AIUnit);

        AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
        if (GameMode)
        {
            FString UnitType = Cast<ASniper>(AIUnit) ? TEXT("Sniper") : TEXT("Brawler");
            GameMode->AddFormattedMoveToLog(
                false, // IsPlayerUnit = false for AI units
                UnitType,
                TEXT("Move"),
                FVector2D(Path[0].X, Path[0].Y), // Starting position
                FVector2D(BestMove.X, BestMove.Y) // Destination position
            );
        }

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
        UE_LOG(LogTemp, Warning, TEXT("AI: Could not find a valid move for unit"));
    }
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