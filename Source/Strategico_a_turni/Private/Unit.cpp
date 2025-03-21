// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"
#include "Kismet/GameplayStatics.h"
#include "Sat_GameMode.h"
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
    GridX = 0;
    GridY = 0;
    UnitGridPosition = FVector2D(0.0f, 0.0f);

    // Default team (will be set properly when spawned)
    bIsPlayerUnit = true;
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
    Super::BeginPlay();

    // Set the initial material
    if (BaseMaterial)
    {
        StaticMeshComponent->SetMaterial(0, BaseMaterial);
    }
}

void AUnit::ShowSelected()
{
    // Change the material to show the unit is selected
    if (SelectedMaterial)
    {
        StaticMeshComponent->SetMaterial(0, SelectedMaterial);
    }
}

void AUnit::UnshowSelected()
{
    // Change back to the base material
    if (BaseMaterial)
    {
        StaticMeshComponent->SetMaterial(0, BaseMaterial);
    }
}

void AUnit::DamageTaken(int32 Damage)
{
    // Reduce the HP by the damage amount
    Hp = FMath::Max(0, Hp - Damage);

    // Optional: Add visual feedback when taking damage
    // For example, play particle effect or animation

    // Check if unit is dead
    if (!IsAlive())
    {
        // Handle death
        // This could trigger an animation, spawn particles, etc.

        // Destroy the actor after a delay to allow for death animations
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);

        // Notify the game mode about the death
        ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GameMode)
        {
            // You would need to implement this function in your game mode
            // GameMode->NotifyUnitDeath(this);
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

        return true;
    }

    return false;
}

bool AUnit::Attack(AUnit* Target)
{
    // Make sure target is valid and alive
    if (!Target || !Target->IsAlive())
    {
        return false;
    }

    // Check if target is in range
    if (!IsTargetInRange(Target))
    {
        return false;
    }

    // Calculate damage
    int32 Damage = CalculateDamage();

    // Apply damage to the target
    Target->DamageTaken(Damage);

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
        return false;
    }

    // Calculate Manhattan distance (grid-based)
    int32 Distance = FMath::Abs(Target->GridX - GridX) + FMath::Abs(Target->GridY - GridY);

    // Check if the target is within range
    return Distance <= RangeAttack;
}