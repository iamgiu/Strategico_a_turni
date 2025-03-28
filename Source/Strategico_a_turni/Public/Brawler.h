// Fill out your copyright notice in the Description page of Project Settings.

/*
 * Brawler unit class for close-range combat
 * Has higher HP and movement but limited attack range
 */

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Brawler.generated.h"


UCLASS()
class STRATEGICO_A_TURNI_API ABrawler : public AUnit
{
	GENERATED_BODY()

public:
    
    // Constructor - initializes Brawler-specific stats
    ABrawler();

protected:

    /*
    * Called when the game starts or when spawned
    * Handles Brawler-specific initialization
    */
    virtual void BeginPlay() override;

};
