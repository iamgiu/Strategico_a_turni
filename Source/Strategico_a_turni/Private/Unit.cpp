// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"
#include "Kismet/GameplayStatics.h"
#include "Sat_GameMode.h"
#include "GridManager.h"
#include "Sniper.h"
#include "Brawler.h"
#include "Engine/World.h"

/*
 * Constructor - sets default values for unit properties
 * Creates components and initializes stats
 */
AUnit::AUnit()
{
    // Set this pawn to call Tick() every frame
    PrimaryActorTick.bCanEverTick = false;

    // Create and setup the static mesh component
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
    RootComponent = StaticMeshComponent;

    // Set default values for the unit's stats
    Movement = 2;
    TypeofAttack = "Base";
    RangeAttack = 1;
    MinDamage = 10;
    MaxDamage = 20;
    Hp = 100;

    // Default position
    GridX = 1;
    GridY = 1;
    UnitGridPosition = FVector2D(0.0f, 0.0f);

    bIsPlayerUnit = true;
    bIsSelected = false;
    bHasMovedThisTurn = false;

    static ConstructorHelpers::FObjectFinder<UMaterial> BlueMaterialAsset(TEXT("/Game/Materials/M_Player"));
    if (BlueMaterialAsset.Succeeded())
    {
        BlueMaterial = BlueMaterialAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Blue Player material!"));
    }

    static ConstructorHelpers::FObjectFinder<UMaterial> RedMaterialAsset(TEXT("/Game/Materials/M_AI"));
    if (RedMaterialAsset.Succeeded())
    {
        RedMaterial = RedMaterialAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Red AI material!"));
    }
}

/*
 * Called when the game starts or when spawned
 * Initializes the unit and ensures proper team color
 */
void AUnit::BeginPlay()
{
    Super::BeginPlay();

    UpdateTeamColor();
}

/*
 * Updates the unit's material based on its team
 * Blue for player units, red for AI units
 */
void AUnit::UpdateTeamColor()
{
    if (!StaticMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("Unit %s: StaticMeshComponent is NULL!"), *GetName());
        return;
    }

    // Get the appropriate team material based on bIsPlayerUnit flag
    UMaterialInterface* TeamMaterial = nullptr;

    if (bIsPlayerUnit)
    {
        TeamMaterial = BlueMaterial;
    }
    else
    {
        TeamMaterial = RedMaterial;
    }

    // Apply the team material if it exists
    if (TeamMaterial)
    {
        // Apply to all material slots
        for (int32 i = 0; i < StaticMeshComponent->GetNumMaterials(); i++)
        {
            StaticMeshComponent->SetMaterial(i, TeamMaterial);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Unit %s: Team material is NULL!"), *GetName());
    }

    // Maintain selected visual state if needed
    if (bIsSelected && SelectedMaterial)
    {
        StaticMeshComponent->SetMaterial(0, SelectedMaterial);
    }
}

/*
 * Sets whether the unit belongs to the player or AI
 * Updates visual appearance accordingly
 * @param bIsPlayer - True if unit belongs to player, false if AI
 */
void AUnit::SetPlayerUnit(bool bIsPlayer)
{
    // Set the team flag
    bIsPlayerUnit = bIsPlayer;

    // Force update team color immediately
    UpdateTeamColor();
}

/*
 * Applies selected visual state to the unit
 * Changes material to indicate selection
 */
void AUnit::ShowSelected()
{
    // Change the material to show the unit is selected
    if (SelectedMaterial)
    {
        StaticMeshComponent->SetMaterial(0, SelectedMaterial);
        bIsSelected = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Unit %s: SelectedMaterial is NULL!"), *GetName());
    }
}

/*
 * Removes selected visual state from the unit
 * Restores original team color
 */
void AUnit::UnshowSelected()
{
    // Explicitly choose the correct material based on team
    UMaterialInterface* TeamMaterial = bIsPlayerUnit ? BlueMaterial : RedMaterial;

    if (TeamMaterial)
    {
        // Apply to all material slots
        for (int32 i = 0; i < StaticMeshComponent->GetNumMaterials(); i++)
        {
            StaticMeshComponent->SetMaterial(i, TeamMaterial);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UnshowSelected: Team material is NULL for %s!"), *GetName());
    }

    bIsSelected = false;
}

/*
 * Applies damage to the unit and handles death if HP reaches zero
 * @param Damage - Amount of damage to apply
 */
void AUnit::DamageTaken(int32 Damage)
{
    // Reduce the HP by the damage amount
    int32 PreviousHP = Hp;
    Hp = FMath::Max(0, Hp - Damage);

    // Check if unit is dead
    if (!IsAlive())
    {
        // Free the grid cell this unit was occupying
        AGridManager* GridManager = nullptr;
        TArray<AActor*> FoundGrids;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
        if (FoundGrids.Num() > 0)
        {
            GridManager = Cast<AGridManager>(FoundGrids[0]);
            if (GridManager)
            {
                // Free the cell
                GridManager->OccupyCell(GridX, GridY, nullptr);
            }
        }

        // Destroy the actor after a delay to allow for death animations
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);

        // Notify the game mode about the death
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GameMode)
        {
            GameMode->NotifyUnitDeath(this);
        }

        // Destroy the actor with a delay
        SetLifeSpan(2.0f);
    }
}

/*
 * Checks if the unit is still alive
 * @return True if unit has HP remaining, false if dead
 */
bool AUnit::IsAlive() const
{
    return Hp > 0;
}

/*
 * Moves the unit to a new grid position within movement range
 * @param NewGridX - Target X coordinate
 * @param NewGridY - Target Y coordinate
 * @return True if move was successful, false otherwise
 */
bool AUnit::Move(int32 NewGridX, int32 NewGridY)
{
    // Check if the unit has already moved this turn
    if (bHasMovedThisTurn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unit %s: Cannot move - already moved this turn"), *GetName());
        return false;
    }

    // Calculate distance from current position to new position
    int32 Distance = FMath::Abs(NewGridX - GridX) + FMath::Abs(NewGridY - GridY);

    // Check if the move is valid (within movement range)
    if (Distance <= Movement)
    {
        // Update grid coordinates
        GridX = NewGridX;
        GridY = NewGridY;
        UnitGridPosition = FVector2D(NewGridX, NewGridY);

        float CellSize = 100.0f;
        FVector NewLocation = FVector(NewGridX * CellSize, NewGridY * CellSize, GetActorLocation().Z);
        SetActorLocation(NewLocation);

        // Set flag to indicate this unit has moved this turn
        bHasMovedThisTurn = true;

        return true;
    }

    return false;
}

/*
 * Performs an attack against another unit with possible counterattack
 * @param Target - Unit to attack
 * @return True if attack was successful, false otherwise
 */
bool AUnit::Attack(AUnit* Target)
{
    // Make sure target is valid and alive
    if (!Target || !Target->IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attack failed: Invalid or dead target"));
        return false;
    }

    // Check if target is in range
    if (!IsTargetInRange(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("Attack failed: Target out of range"));
        return false;
    }

    // Calculate main attack damage
    int32 Damage = CalculateDamage();

    // Store the attacker's and target's health before attack for damage calculation
    int32 AttackerHpBefore = this->Hp;
    int32 TargetHpBefore = Target->Hp;

    // Variables to track counterattack
    bool bShouldCounterattack = false;
    int32 CounterDamage = 0;

    if (Target->IsAlive())
    {
        // Check for counterattack conditions
        ASniper* TargetSniper = Cast<ASniper>(Target);
        ABrawler* TargetBrawler = Cast<ABrawler>(Target);

        if (TargetSniper)
        {
            bShouldCounterattack = true;
        }

        else if (TargetBrawler)
        {
            // Calculate Manhattan distance (grid-based)
            int32 Distance = FMath::Abs(Target->GridX - GridX) +
                FMath::Abs(Target->GridY - GridY);
            if (Distance <= 1)
            {
                bShouldCounterattack = true;
            }
        }
        if (bShouldCounterattack)
        {
            // Calculate counterattack damage (random 1-3)
            CounterDamage = FMath::RandRange(1, 3);
        }
    }

    Target->DamageTaken(Damage);

    if (bShouldCounterattack && CounterDamage > 0)
    {
        DamageTaken(CounterDamage);
    }

    // Calculate actual damages dealt
    int32 ActualDamage = TargetHpBefore - Target->Hp;
    int32 ActualCounterDamage = AttackerHpBefore - this->Hp;

    // Log the counterattack result if there was one
    if (bShouldCounterattack && CounterDamage > 0)
    {
        AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
        if (GameMode)
        {
            FString UnitType = Target->UnitTypeDisplayName;
            GameMode->AddFormattedMoveToLog(
                Target->bIsPlayerUnit, // IsPlayerUnit based on the target
                UnitType,
                TEXT("Counterattack"),
                FVector2D(Target->GridX, Target->GridY), // From position
                FVector2D(GridX, GridY), // To position (this unit)
                ActualCounterDamage // Damage dealt
            );
        }
    }

    if (!this->IsAlive() && !Target->IsAlive())
    {
        // Count ALL remaining units on both sides
        TArray<AActor*> AllUnits;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

        int32 HumanUnitsAlive = 0;
        int32 AIUnitsAlive = 0;

        for (AActor* UnitActor : AllUnits)
        {
            AUnit* Unit = Cast<AUnit>(UnitActor);
            if (Unit && Unit != this && Unit != Target && Unit->IsAlive())
            {
                if (Unit->bIsPlayerUnit)
                    HumanUnitsAlive++;
                else
                    AIUnitsAlive++;
            }
        }

        if (HumanUnitsAlive == 0 && AIUnitsAlive == 0)
        {
            AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
            ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
            if (GameMode)
            {
                GameMode->HandleDrawCondition();
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Units killed each other, but other units remain: Human=%d, AI=%d"),
                HumanUnitsAlive, AIUnitsAlive);
        }
    }

    // Mark this unit as having attacked
    bHasAttackedThisTurn = true;

    // Return true if the attack was successful
    return true;
}

/*
 * Calculates random damage value between min and max damage
 * @return Damage amount
 */
int32 AUnit::CalculateDamage() const
{
    // Generate a random damage value between MinDamage and MaxDamage
    return FMath::RandRange(MinDamage, MaxDamage);
}

/*
 * Checks if a target unit is within attack range
 * Uses Manhattan distance on grid
 * @param Target - Unit to check range to
 * @return True if target is in range, false otherwise
 */
bool AUnit::IsTargetInRange(const AUnit* Target) const
{
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is null in range check"));
        return false;
    }

    // Calculate Manhattan distance (grid-based)
    int32 Distance = FMath::Abs(Target->GridX - GridX) + FMath::Abs(Target->GridY - GridY);

    // Check if the target is within range
    return Distance <= RangeAttack;
}

/*
 * Static method to check if two units would destroy each other
 * Used for detecting potential draw scenarios
 * @param Attacker - First unit in potential mutual destruction
 * @param Target - Second unit in potential mutual destruction
 * @return True if mutual destruction would occur, false otherwise
 */
bool AUnit::CheckMutualDestruction(AUnit* Attacker, AUnit* Target)
{
    // Validate inputs
    if (!Attacker || !Target)
        return false;

    // Verify both units are still alive
    if (!Attacker->IsAlive() || !Target->IsAlive())
        return false;

    // Check if both units are the last ones alive for their respective teams
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(Attacker->GetWorld(), AUnit::StaticClass(), AllUnits);

    int32 PlayerUnitsAlive = 0;
    int32 AIUnitsAlive = 0;

    // Count all living units
    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit && Unit->IsAlive() && Unit != Attacker && Unit != Target)
        {
            if (Unit->bIsPlayerUnit)
                PlayerUnitsAlive++;
            else
                AIUnitsAlive++;
        }
    }

    if (PlayerUnitsAlive > 0 || AIUnitsAlive > 0)
    {
        return false;
    }

    ASniper* AttackerSniper = Cast<ASniper>(Attacker);
    ASniper* TargetSniper = Cast<ASniper>(Target);
    if (!AttackerSniper || !TargetSniper)
    {
        return false;
    }

    bool AttackerIsHuman = Attacker->bIsPlayerUnit;
    bool TargetIsHuman = Target->bIsPlayerUnit;

    if (AttackerIsHuman == TargetIsHuman)
    {
        return false;
    }


    if (Attacker->Hp != 1 || Target->Hp != 1)
    {
        return false;
    }

    bool AttackerCanHitTarget = Attacker->IsTargetInRange(Target);
    bool TargetCanHitAttacker = Target->IsTargetInRange(Attacker);

    if (!AttackerCanHitTarget || !TargetCanHitAttacker)
    {
        return false;
    }
    return true;
}