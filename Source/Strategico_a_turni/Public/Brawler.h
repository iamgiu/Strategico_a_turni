// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Unit.h"
#include "Brawler.generated.h"


UCLASS()
class STRATEGICO_A_TURNI_API ABrawler : public AUnit
{
	GENERATED_BODY()

public:
    ABrawler();

protected:

    virtual void BeginPlay() override;

};
