// Fill out your copyright notice in the Description page of Project Settings.


#include "Brawler.h"

ABrawler::ABrawler()
{

    Hp = 40;
    Movement = 6;
    //TypeofAttack = FString::"Melee";
    RangeAttack = 1;
    MinDamage = 1;
    MaxDamage = 6;
}

void ABrawler::BeginPlay()
{
    Super::BeginPlay();
}