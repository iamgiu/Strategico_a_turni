// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"
#include "Kismet/GameplayStatics.h"
#include "Sat_GameMode.h"
#include "GridManager.h"
#include "Sniper.h"
#include "Brawler.h"
#include "Engine/World.h"

// Sets default values
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

    // Default team (will be set properly when spawned)
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

    // Make sure to log success or failure for debugging
    UE_LOG(LogTemp, Display, TEXT("Unit constructor: Blue material %s, Red material %s"),
        BlueMaterial ? TEXT("loaded") : TEXT("failed"),
        RedMaterial ? TEXT("loaded") : TEXT("failed"));
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
    Super::BeginPlay();

    // Verify team setting
    UE_LOG(LogTemp, Display, TEXT("Unit %s BeginPlay: Team is %s"),
        *GetName(), bIsPlayerUnit ? TEXT("Player") : TEXT("AI"));

    UpdateTeamColor();
}

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
        UE_LOG(LogTemp, Display, TEXT("Unit %s: Using BLUE material for PLAYER unit"), *GetName());
    }
    else
    {
        TeamMaterial = RedMaterial;
        UE_LOG(LogTemp, Display, TEXT("Unit %s: Using RED material for AI unit"), *GetName());
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

void AUnit::SetPlayerUnit(bool bIsPlayer)
{
    // Add more detailed logging
    UE_LOG(LogTemp, Error, TEXT("SetPlayerUnit CALLED: Unit %s - Setting team to %s"),
        *GetName(), bIsPlayer ? TEXT("PLAYER (BLUE)") : TEXT("AI (RED)"));

    // Set the team flag
    bIsPlayerUnit = bIsPlayer;

    // Force update team color immediately
    UpdateTeamColor();
}

void AUnit::ShowSelected()
{
    // Change the material to show the unit is selected
    if (SelectedMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unit %s: Applying selected material"), *GetName());
        StaticMeshComponent->SetMaterial(0, SelectedMaterial);
        bIsSelected = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Unit %s: SelectedMaterial is NULL!"), *GetName());
    }
}

void AUnit::UnshowSelected()
{
    UE_LOG(LogTemp, Error, TEXT("UnshowSelected: Unit %s, Team = %s"),
        *GetName(), bIsPlayerUnit ? TEXT("PLAYER (BLUE)") : TEXT("AI (RED)"));

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

void AUnit::DamageTaken(int32 Damage)
{
    UE_LOG(LogTemp, Warning, TEXT("%s taking %d damage. Current HP: %d"),
        *GetName(), Damage, Hp);

    // Reduce the HP by the damage amount
    int32 PreviousHP = Hp;
    Hp = FMath::Max(0, Hp - Damage);

    UE_LOG(LogTemp, Warning, TEXT("%s HP reduced from %d to %d"),
        *GetName(), PreviousHP, Hp);

    // Check if unit is dead
    if (!IsAlive())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s has been killed!"), *GetName());

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
                UE_LOG(LogTemp, Warning, TEXT("Freed grid cell at (%d,%d) after unit death"), GridX, GridY);
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

        // Destroy the actor with a delay (e.g., 2 seconds)
        SetLifeSpan(2.0f);
    }
}

bool AUnit::IsAlive() const
{
    return Hp > 0;
}

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

        // Update the actor's physical position in the world
        // This assumes your grid has a cell size of 100 units
        float CellSize = 100.0f;
        FVector NewLocation = FVector(NewGridX * CellSize, NewGridY * CellSize, GetActorLocation().Z);
        SetActorLocation(NewLocation);

        // Set flag to indicate this unit has moved this turn
        bHasMovedThisTurn = true;

        return true;
    }

    return false;
}

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

    // Calculate damage
    int32 Damage = CalculateDamage();

    // Log the attack for debugging
    UE_LOG(LogTemp, Warning, TEXT("%s attacking %s for %d damage"),
        *GetName(), *Target->GetName(), Damage);

    // Apply damage to the target
    Target->DamageTaken(Damage);

    // Check if this is a Sniper unit
    ASniper* AttackerSniper = Cast<ASniper>(this);
    if (AttackerSniper)
    {
        // Check for counterattack conditions:
        // 1. If the target is a Sniper
        // 2. If the target is a Brawler and is adjacent (distance of 1)
        ASniper* TargetSniper = Cast<ASniper>(Target);
        ABrawler* TargetBrawler = Cast<ABrawler>(Target);

        bool bShouldCounterattack = false;

        // Condition 1: Target is a Sniper
        if (TargetSniper)
        {
            bShouldCounterattack = true;
            UE_LOG(LogTemp, Warning, TEXT("Counterattack: Sniper attacked another Sniper"));
        }
        // Condition 2: Target is a Brawler and is adjacent
        else if (TargetBrawler)
        {
            // Calculate Manhattan distance (grid-based)
            int32 Distance = FMath::Abs(Target->GridX - GridX) + FMath::Abs(Target->GridY - GridY);
            if (Distance <= 1)
            {
                bShouldCounterattack = true;
                UE_LOG(LogTemp, Warning, TEXT("Counterattack: Sniper attacked adjacent Brawler"));
            }
        }

        // Execute counterattack if conditions are met
        if (bShouldCounterattack && IsAlive() && Target->IsAlive())
        {
            // Calculate counterattack damage (random 1-3 as specified)
            int32 CounterDamage = FMath::RandRange(1, 3);
            UE_LOG(LogTemp, Warning, TEXT("%s receives counterattack damage of %d"),
                *GetName(), CounterDamage);

            // Apply counterattack damage to the attacking Sniper
            DamageTaken(CounterDamage);

            // Log the counterattack result
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
                    CounterDamage // Damage dealt
                );
            }
        }
    }

    // Check for mutual destruction before processing attack
    bool bMutualDestruction = CheckMutualDestruction(this, Target);

    if (bMutualDestruction)
    {
        // Notify game mode about draw condition
        AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
        if (GameMode)
        {
            GameMode->HandleDrawCondition();
        }
    }

    // Mark this unit as having attacked
    bHasAttackedThisTurn = true;

    // Return true if the attack was successful
    return true;
}

int32 AUnit::CalculateDamage() const
{
    // Generate a random damage value between MinDamage and MaxDamage
    return FMath::RandRange(MinDamage, MaxDamage);
}

bool AUnit::IsTargetInRange(const AUnit* Target) const
{
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is null in range check"));
        return false;
    }

    // Calculate Manhattan distance (grid-based)
    int32 Distance = FMath::Abs(Target->GridX - GridX) + FMath::Abs(Target->GridY - GridY);

    UE_LOG(LogTemp, Warning, TEXT("%s checking range to %s. Distance: %d, Attack Range: %d"),
        *GetName(), *Target->GetName(), Distance, RangeAttack);

    // Check if the target is within range
    return Distance <= RangeAttack;
}

bool AUnit::CheckMutualDestruction(AUnit* Attacker, AUnit* Target)
{
    if (!Attacker || !Target)
        return false;

    // Calculate attack and counterattack damages
    int32 AttackDamage = Attacker->CalculateDamage();
    int32 CounterAttackDamage = FMath::RandRange(1, 3);

    // Predict HP after attack and counterattack
    int32 AttackerFinalHP = Attacker->Hp - CounterAttackDamage;
    int32 TargetFinalHP = Target->Hp - AttackDamage;

    // Check for mutual destruction
    return (AttackerFinalHP <= 0 && TargetFinalHP <= 0);
}