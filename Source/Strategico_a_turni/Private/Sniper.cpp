 // Fill out your copyright notice in the Description page of Project Settings.


#include "Sniper.h"

ASniper::ASniper()
{
    
    Hp = 20;
    Movement = 3;
    //TypeofAttack = FString::"Distance";
    RangeAttack = 10;
    MinDamage = 4;
    MaxDamage = 8;
}

void ASniper::BeginPlay()
{
    Super::BeginPlay();
    // Logiche aggiuntive specifiche per il comportamento dello Sniper
}