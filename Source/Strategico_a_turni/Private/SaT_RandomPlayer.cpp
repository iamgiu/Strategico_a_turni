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
    UE_LOG(LogTemp, Warning, TEXT("AI Turn Started - Attempting to show thinking widget"));
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetShowMouseCursor(false);
        GameMode->ShowAIThinkingWidget(true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find GameMode to show thinking widget"));
    }

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

        // Hide AI thinking widget at end of turn processing
        if (GameMode)
        {
            GameMode->ShowAIThinkingWidget(false);
        }

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
    if (GameMode)
    {
        // Use a timer to ensure widget stays visible long enough
        FTimerHandle HideWidgetTimer;
        GetWorld()->GetTimerManager().SetTimer(HideWidgetTimer, [GameMode]()
            {
                GameMode->ShowAIThinkingWidget(false);
            }, 0.5f, false);
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
            // FIXED: Randomly choose between Sniper and Brawler, unless we already placed one
            bool bHasPlacedSniper = false;
            bool bHasPlacedBrawler = false;

            // Check which units we've already placed
            TArray<AActor*> AllUnits;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

            for (AActor* UnitActor : AllUnits)
            {
                AUnit* Unit = Cast<AUnit>(UnitActor);
                if (Unit && !Unit->bIsPlayerUnit) // Check AI units
                {
                    if (Cast<ASniper>(Unit))
                    {
                        bHasPlacedSniper = true;
                    }
                    else if (Cast<ABrawler>(Unit))
                    {
                        bHasPlacedBrawler = true;
                    }
                }
            }

            // Determine which unit to place
            bool bIsSniper;

            // If we haven't placed either, randomly choose
            if (!bHasPlacedSniper && !bHasPlacedBrawler)
            {
                bIsSniper = (FMath::RandBool()); // 50% chance for either
                UE_LOG(LogTemp, Warning, TEXT("AI: Randomly chose to place %s first"),
                    bIsSniper ? TEXT("Sniper") : TEXT("Brawler"));
            }
            // If we've placed Sniper but not Brawler, place Brawler
            else if (bHasPlacedSniper && !bHasPlacedBrawler)
            {
                bIsSniper = false;
                UE_LOG(LogTemp, Warning, TEXT("AI: Already placed Sniper, placing Brawler"));
            }
            // If we've placed Brawler but not Sniper, place Sniper
            else if (!bHasPlacedSniper && bHasPlacedBrawler)
            {
                bIsSniper = true;
                UE_LOG(LogTemp, Warning, TEXT("AI: Already placed Brawler, placing Sniper"));
            }
            // Just in case both are already placed
            else
            {
                UE_LOG(LogTemp, Error, TEXT("AI: Both unit types already placed! Should not reach here."));
                EndTurn();
                return;
            }

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
                Unit->bIsPlayerUnit = false; // EXPLICITLY set as AI unit
                Unit->UpdateTeamColor();      // Force update team color immediately

                UE_LOG(LogTemp, Warning, TEXT("AI Unit placed - bIsPlayerUnit set to FALSE, calling UpdateTeamColor"));

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

        // Double-check if cell is free from both occupation AND obstacles
        if (!GridManager->IsCellOccupied(OutGridX, OutGridY))
        {
            // Additional obstacle check to be absolutely sure
            ATile* Tile = nullptr;
            if (GridManager->TileMap.Contains(FVector2D(OutGridX, OutGridY)))
            {
                Tile = GridManager->TileMap[FVector2D(OutGridX, OutGridY)];
                if (Tile && !Tile->bIsObstacle && !Tile->bIsOccupied)
                {
                    // Triple check - this position is definitely valid
                    return true;
                }
            }
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
    // Check the current difficulty setting
    if (GameInstance && GameInstance->AIDifficulty == EAIDifficulty::EASY)
    {
        // Use random behavior for Easy mode
        ProcessUnitActionsRandom(Unit);
    }
    else
    {
        // Use strategic A* pathfinding for Hard mode
        ProcessUnitActionsStrategic(Unit);
    }
}

// Process unit actions with random behavior (Easy mode)
void ASaT_RandomPlayer::ProcessUnitActionsRandom(AUnit* Unit)
{
    // Random behavior for Easy AI:
    // - Random chance to move or attack first
    // - Random target selection
    // - Random movement within range

    // Check if we can attack first (50% chance)
    bool bAttackFirst = FMath::RandBool();

    if (bAttackFirst)
    {
        // Try attack first, then maybe move
        if (!Unit->bHasAttackedThisTurn)
        {
            AUnit* Target = FindRandomAttackTarget(Unit);
            if (Target)
            {
                // Attack logic
                Unit->Attack(Target);
            }
        }

        // Maybe move after attack (75% chance)
        if (!Unit->bHasMovedThisTurn && FMath::RandRange(0, 3) > 0)
        {
            MoveRandomly(Unit);
        }
    }
    else
    {
        // Move first, then maybe attack
        if (!Unit->bHasMovedThisTurn)
        {
            MoveRandomly(Unit);
        }

        // Maybe attack after moving (75% chance)
        if (!Unit->bHasAttackedThisTurn && FMath::RandRange(0, 3) > 0)
        {
            AUnit* Target = FindRandomAttackTarget(Unit);
            if (Target)
            {
                // Attack logic
                Unit->Attack(Target);
            }
        }
    }
}

// Find a random attack target within range
AUnit* ASaT_RandomPlayer::FindRandomAttackTarget(AUnit* AIUnit)
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
                UE_LOG(LogTemp, Warning, TEXT("Easy AI: Found potential target at (%d,%d), distance %d, range %d"),
                    PlayerUnit->GridX, PlayerUnit->GridY, Distance, AIUnit->RangeAttack);
            }
        }
    }

    // If we have targets, select a random one
    if (PotentialTargets.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, PotentialTargets.Num() - 1);
        return PotentialTargets[RandomIndex];
    }

    return nullptr;
}

// Move the unit to a random valid position
void ASaT_RandomPlayer::MoveRandomly(AUnit* Unit)
{
    if (!Unit || !GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("MoveRandomly: Invalid Unit or GridManager"));
        return;
    }

    // Unit has already moved
    if (Unit->bHasMovedThisTurn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unit %s has already moved this turn"), *Unit->GetName());
        return;
    }

    // Get the unit's current position and movement range
    int32 UnitX = Unit->GridX;
    int32 UnitY = Unit->GridY;
    int32 MovementRange = Unit->Movement;

    // Store valid move options
    TArray<FVector2D> ValidMoves;

    // Collect potential moves within movement range
    for (int32 X = UnitX - MovementRange; X <= UnitX + MovementRange; X++)
    {
        for (int32 Y = UnitY - MovementRange; Y <= UnitY + MovementRange; Y++)
        {
            // Skip if out of grid bounds
            if (X < 0 || X >= 25 || Y < 0 || Y >= 25)
                continue;

            // Calculate Manhattan distance
            int32 Distance = FMath::Abs(X - UnitX) + FMath::Abs(Y - UnitY);

            // Check if within movement range
            if (Distance <= MovementRange)
            {
                // Validate the entire path between current position and target
                bool bPathClear = IsPathClear(UnitX, UnitY, X, Y);

                if (bPathClear && !GridManager->IsCellOccupied(X, Y))
                {
                    ValidMoves.Add(FVector2D(X, Y));
                }
            }
        }
    }

    // No valid moves found
    if (ValidMoves.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid moves found for unit %s"), *Unit->GetName());
        return;
    }

    // Randomly select a move from valid options
    int32 RandomIndex = FMath::RandRange(0, ValidMoves.Num() - 1);
    FVector2D SelectedMove = ValidMoves[RandomIndex];

    // Perform the move
    int32 NewGridX = FMath::FloorToInt(SelectedMove.X);
    int32 NewGridY = FMath::FloorToInt(SelectedMove.Y);

    // Save old position
    int32 OldGridX = Unit->GridX;
    int32 OldGridY = Unit->GridY;

    // Free the current cell
    GridManager->OccupyCell(OldGridX, OldGridY, nullptr);

    // Update unit position
    FVector NewLocation = GridManager->GetWorldLocationFromGrid(NewGridX, NewGridY);
    Unit->GridX = NewGridX;
    Unit->GridY = NewGridY;
    Unit->SetActorLocation(NewLocation);

    // Mark the unit as moved
    Unit->bHasMovedThisTurn = true;

    // Occupy the new cell
    GridManager->OccupyCell(Unit->GridX, Unit->GridY, Unit);

    // Prepare and highlight path
    TArray<FVector2D> MovePath;
    MovePath.Add(FVector2D(OldGridX, OldGridY));
    MovePath.Add(SelectedMove);

    GridManager->HighlightPath(MovePath, true);

    // Log the move
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        FString UnitType = Cast<ASniper>(Unit) ? TEXT("Sniper") : TEXT("Brawler");
        GameMode->AddFormattedMoveToLog(
            false,
            UnitType,
            TEXT("Move"),
            FVector2D(OldGridX, OldGridY),
            FVector2D(Unit->GridX, Unit->GridY)
        );
    }
}

// Helper method to validate the entire path
bool ASaT_RandomPlayer::IsPathClear(int32 StartX, int32 StartY, int32 EndX, int32 EndY)
{
    // Determine movement direction
    int32 DeltaX = EndX - StartX;
    int32 DeltaY = EndY - StartY;

    // First, check horizontal movement
    if (DeltaX != 0)
    {
        int32 XStep = (DeltaX > 0) ? 1 : -1;
        for (int32 X = StartX + XStep; X != EndX; X += XStep)
        {
            if (GridManager->IsCellOccupied(X, StartY))
            {
                UE_LOG(LogTemp, Warning, TEXT("Path blocked horizontally at (%d,%d)"), X, StartY);
                return false;
            }
        }
    }

    // Then, check vertical movement
    if (DeltaY != 0)
    {
        int32 YStep = (DeltaY > 0) ? 1 : -1;
        for (int32 Y = StartY + YStep; Y != EndY; Y += YStep)
        {
            if (GridManager->IsCellOccupied(EndX, Y))
            {
                UE_LOG(LogTemp, Warning, TEXT("Path blocked vertically at (%d,%d)"), EndX, Y);
                return false;
            }
        }
    }

    // Final destination check
    if (GridManager->IsCellOccupied(EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("Destination tile (%d,%d) is occupied"), EndX, EndY);
        return false;
    }

    return true;
}

void ASaT_RandomPlayer::ProcessUnitActionsStrategic(AUnit* Unit)
{
    if (!Unit || !Unit->IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Unit is invalid or dead"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("AI: Processing strategic move for %s with Movement=%d, RangeAttack=%d"),
        *Unit->GetName(), Unit->Movement, Unit->RangeAttack);

    // First try to attack without moving
    AUnit* TargetWithoutMoving = FindAttackTarget(Unit);

    // If we can attack now, do it
    if (TargetWithoutMoving && !Unit->bHasAttackedThisTurn)
    {
        // Get target's HP before attack
        int32 TargetHPBefore = TargetWithoutMoving->Hp;

        // Perform attack
        Unit->Attack(TargetWithoutMoving);

        // Calculate actual damage by checking HP difference
        int32 DamageDealt = TargetHPBefore - TargetWithoutMoving->Hp;

        // Log the attack
        AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
        if (GameMode)
        {
            FString UnitType = Cast<ASniper>(Unit) ? TEXT("Sniper") : TEXT("Brawler");
            GameMode->AddFormattedMoveToLog(
                false, // IsPlayerUnit = false for AI units
                UnitType,
                TEXT("Attack"),
                FVector2D(Unit->GridX, Unit->GridY), // Attack from position
                FVector2D(TargetWithoutMoving->GridX, TargetWithoutMoving->GridY), // Target position
                DamageDealt
            );
        }
    }
    // If we can't attack now but haven't moved yet, move strategically toward a player unit
    else if (!Unit->bHasMovedThisTurn)
    {
        // Use A* pathing to move toward a player unit
        MoveTowardPlayerUnit(Unit);

        // After moving, check if we can attack
        if (!Unit->bHasAttackedThisTurn)
        {
            // Try to find a new target after moving
            AUnit* TargetAfterMoving = FindAttackTarget(Unit);
            if (TargetAfterMoving)
            {
                // Get target's HP before attack
                int32 TargetHPBefore = TargetAfterMoving->Hp;

                // Perform attack
                Unit->Attack(TargetAfterMoving);

                // Calculate actual damage by checking HP difference
                int32 DamageDealt = TargetHPBefore - TargetAfterMoving->Hp;

                // Log the attack
                AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
                ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
                if (GameMode)
                {
                    FString UnitType = Cast<ASniper>(Unit) ? TEXT("Sniper") : TEXT("Brawler");
                    GameMode->AddFormattedMoveToLog(
                        false, // IsPlayerUnit = false for AI units
                        UnitType,
                        TEXT("Attack"),
                        FVector2D(Unit->GridX, Unit->GridY), // Attack from position
                        FVector2D(TargetAfterMoving->GridX, TargetAfterMoving->GridY), // Target position
                        DamageDealt
                    );
                }
            }
        }
    }
}

// Calculate Manhattan distance between two points
int32 ASaT_RandomPlayer::ManhattanDistance(const FVector2D& A, const FVector2D& B)
{
    return FMath::Abs(FMath::FloorToInt(A.X) - FMath::FloorToInt(B.X)) +
        FMath::Abs(FMath::FloorToInt(A.Y) - FMath::FloorToInt(B.Y));
}

// Find a player unit to attack if any are in range
AUnit* ASaT_RandomPlayer::FindAttackTarget(AUnit* AIUnit)
{
    if (!AIUnit) return nullptr;

    // Get all player units
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    // Find player units in attack range, prioritizing the weakest ones
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
                UE_LOG(LogTemp, Warning, TEXT("AI: Found potential target at (%d,%d), HP: %d, distance: %d"),
                    PlayerUnit->GridX, PlayerUnit->GridY, PlayerUnit->Hp, Distance);
            }
        }
    }

    // If we have targets, prioritize the weakest one
    if (PotentialTargets.Num() > 0)
    {
        // Sort by HP (weakest first)
        PotentialTargets.Sort([](const AUnit& A, const AUnit& B) {
            return A.Hp < B.Hp;
            });

        // Return the weakest unit
        return PotentialTargets[0];
    }

    return nullptr;
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

// Move the AI unit toward the closest player unit using A* pathfinding
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

    // Get the unit's movement range
    int32 MovementRange = AIUnit->Movement;

    // Find best move using A* pathfinding
    // Initialize data structures for A* algorithm
    TArray<FVector2D> ClosedSet;
    TArray<FVector2D> OpenSet;
    TMap<FVector2D, FVector2D> CameFrom;
    TMap<FVector2D, int32> GScore;
    TMap<FVector2D, int32> FScore;

    // Direction vectors for the four cardinal directions
    int32 dx[] = { 1, -1, 0, 0 };
    int32 dy[] = { 0, 0, 1, -1 };

    // Start position
    FVector2D Start(AIUnit->GridX, AIUnit->GridY);
    // Target position
    FVector2D Target(ClosestUnit->GridX, ClosestUnit->GridY);

    // Initialize A* parameters
    OpenSet.Add(Start);
    GScore.Add(Start, 0);
    FScore.Add(Start, ManhattanDistance(Start, Target));

    // Best move found so far
    FVector2D BestMove = Start;
    int32 BestDistanceToTarget = ManhattanDistance(Start, Target);
    bool bFoundPath = false;

    // A* main loop - find a path toward the target, limited by movement range
    while (OpenSet.Num() > 0)
    {
        // Find node with lowest F score
        FVector2D Current = OpenSet[0];
        int32 LowestFScore = FScore[Current];
        int32 CurrentIndex = 0;

        for (int32 i = 1; i < OpenSet.Num(); i++)
        {
            if (FScore[OpenSet[i]] < LowestFScore)
            {
                Current = OpenSet[i];
                LowestFScore = FScore[Current];
                CurrentIndex = i;
            }
        }

        // Check if we've reached the target
        if (Current.Equals(Target))
        {
            bFoundPath = true;
            BestMove = ReconstructPath(CameFrom, Current, Start, MovementRange);
            break;
        }

        // Remove current from open set and add to closed set
        OpenSet.RemoveAt(CurrentIndex);
        ClosedSet.Add(Current);

        // Get current G score
        int32 CurrentG = GScore[Current];

        // Stop expanding if we've reached our movement limit
        if (CurrentG >= MovementRange)
            continue;

        // Check all four directions
        for (int32 i = 0; i < 4; i++)
        {
            int32 NewX = FMath::FloorToInt(Current.X) + dx[i];
            int32 NewY = FMath::FloorToInt(Current.Y) + dy[i];
            FVector2D Neighbor(NewX, NewY);

            // Skip if invalid position
            if (!GridManager->IsValidPosition(Neighbor))
                continue;

            // Skip if in closed set
            if (ClosedSet.Contains(Neighbor))
                continue;

            // Skip if cell is occupied (unless it's the target)
            if (GridManager->IsCellOccupied(NewX, NewY) &&
                !(NewX == Target.X && NewY == Target.Y))
                continue;

            // Calculate new G score (one more move)
            int32 TentativeG = CurrentG + 1;

            // If we haven't seen this neighbor yet or found a better path
            if (!GScore.Contains(Neighbor) || TentativeG < GScore[Neighbor])
            {
                // Record this path
                CameFrom.Add(Neighbor, Current);
                GScore.Add(Neighbor, TentativeG);
                FScore.Add(Neighbor, TentativeG + ManhattanDistance(Neighbor, Target));

                // If this move is better than our current best and within movement range
                if (TentativeG <= MovementRange &&
                    ManhattanDistance(Neighbor, Target) < BestDistanceToTarget)
                {
                    BestMove = Neighbor;
                    BestDistanceToTarget = ManhattanDistance(Neighbor, Target);
                }

                // Add to open set if not already there
                if (!OpenSet.Contains(Neighbor))
                {
                    OpenSet.Add(Neighbor);
                }
            }
        }
    }

    if (!bFoundPath && !BestMove.Equals(Start))
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: No complete path found, using best available move"));
    }
    else if (BestMove.Equals(Start))
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: No valid moves found, staying in place"));
        return;
    }

    // Reconstruct the path from the start to the best move
    TArray<FVector2D> Path;
    FVector2D Current = BestMove;
    while (CameFrom.Contains(Current) && !Current.Equals(Start))
    {
        Path.Insert(Current, 0);
        Current = CameFrom[Current];
    }
    Path.Insert(Start, 0);

    // Move the unit to the best position found
    UE_LOG(LogTemp, Warning, TEXT("AI: Moving unit from (%d,%d) to (%d,%d)"),
        AIUnit->GridX, AIUnit->GridY, FMath::FloorToInt(BestMove.X), FMath::FloorToInt(BestMove.Y));

    // Store old position for logging
    int32 OldX = AIUnit->GridX;
    int32 OldY = AIUnit->GridY;

    // Free the current cell
    GridManager->OccupyCell(OldX, OldY, nullptr);

    // Update unit's position
    AIUnit->GridX = FMath::FloorToInt(BestMove.X);
    AIUnit->GridY = FMath::FloorToInt(BestMove.Y);
    AIUnit->SetActorLocation(GridManager->GetWorldLocationFromGrid(AIUnit->GridX, AIUnit->GridY));

    // Mark unit as moved
    AIUnit->bHasMovedThisTurn = true;

    // Occupy the new cell
    GridManager->OccupyCell(AIUnit->GridX, AIUnit->GridY, AIUnit);

    // Highlight the path
    GridManager->HighlightPath(Path, true);

    // Log the move
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());  
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        FString UnitType = Cast<ASniper>(AIUnit) ? TEXT("Sniper") : TEXT("Brawler");
        GameMode->AddFormattedMoveToLog(
            false, // IsPlayerUnit = false for AI units  
            UnitType,
            TEXT("Move"),
            FVector2D(OldX, OldY),
            FVector2D(AIUnit->GridX, AIUnit->GridY)
        );
    }
}

// Helper function to reconstruct a path from A* search and get the best move within range
FVector2D ASaT_RandomPlayer::ReconstructPath(const TMap<FVector2D, FVector2D>& CameFrom,
    FVector2D Current, FVector2D Start, int32 MaxSteps)
{
    // Build the complete path
    TArray<FVector2D> Path;
    Path.Add(Current);

    while (CameFrom.Contains(Current) && !Current.Equals(Start))
    {
        Current = CameFrom[Current];
        Path.Add(Current);
    }

    // Reverse the path and find the last step within range
    if (Path.Num() <= MaxSteps + 1)
    {
        // We can reach the end of the path, so return the last non-start point
        for (int32 i = Path.Num() - 1; i >= 0; i--)
        {
            if (!Path[i].Equals(Start))
            {
                return Path[i];
            }
        }
    }
    else
    {
        // We can only go MaxSteps steps, so return the MaxSteps-th position from the end
        return Path[Path.Num() - 1 - MaxSteps];
    }

    // Fallback - return starting position if no valid move found
    return Start;
}

void ASaT_RandomPlayer::OnWin()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player has won!"));
}

void ASaT_RandomPlayer::OnLose()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player has lost!"));
}
