// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Unit.h"
#include "GridManager.h"
#include "SaT_PlayerController.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_PlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

    virtual void BeginPlay() override;

    virtual void SetupInputComponent() override;

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AUnit> SelectedUnitClass;

    void SelectSniper();

    void SelectBrawler();

    void OnClick();
	
};
