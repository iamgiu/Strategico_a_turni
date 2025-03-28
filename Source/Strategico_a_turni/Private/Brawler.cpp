// Fill out your copyright notice in the Description page of Project Settings.


#include "Brawler.h"

/*
 * Constructor - sets default values for Brawler's properties
 * Higher HP and movement, but limited attack range
 */
ABrawler::ABrawler()
{

    Hp = 40;
    Movement = 6;
    //TypeofAttack = FString::"Melee";
    RangeAttack = 1;
    MinDamage = 1;
    MaxDamage = 6;

    UnitTypeDisplayName = TEXT("Brawler");
}

/*
 * Called when the game starts
 * Initializes the Brawler unit
 */
void ABrawler::BeginPlay()
{
    Super::BeginPlay();
}