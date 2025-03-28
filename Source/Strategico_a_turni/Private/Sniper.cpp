 // Fill out your copyright notice in the Description page of Project Settings.


#include "Sniper.h"

/*
 * Constructor - sets default values for Sniper's properties
 * Lower HP and movement, but high attack range and damage
 */
ASniper::ASniper()
{
    
    Hp = 20;
    Movement = 3;
    //TypeofAttack = FString::"Distance";
    RangeAttack = 10;
    MinDamage = 4;
    MaxDamage = 8;

    UnitTypeDisplayName = TEXT("Sniper");
}

/*
 * Called when the game starts
 * Initializes the Sniper unit
 */
void ASniper::BeginPlay()
{
    Super::BeginPlay();
}