// Fill out your copyright notice in the Description page of Project Settings.

/*
 * Sniper unit class for long-range attacks
 * Has lower HP but higher attack range than other unit types
 */

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Sniper.generated.h"

UCLASS()

class STRATEGICO_A_TURNI_API ASniper : public AUnit
{
    GENERATED_BODY()

public:

    //  Constructor - initializes Sniper-specific stats
    ASniper();

protected:

    /*
    * Called when the game starts or when spawned
    * Handles Sniper-specific initialization
    */
    virtual void BeginPlay() override;

};
