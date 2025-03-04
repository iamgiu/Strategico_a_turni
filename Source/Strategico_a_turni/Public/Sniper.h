// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Sniper.generated.h"

UCLASS()

class STRATEGICO_A_TURNI_API ASniper : public AUnit
{
	GENERATED_BODY()

public:
    // Costruttore
    ASniper();

protected:
    // Sovrascrittura dei valori per Sniper
    virtual void BeginPlay() override;

};
