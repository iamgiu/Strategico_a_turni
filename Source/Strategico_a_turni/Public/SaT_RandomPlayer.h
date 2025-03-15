// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SaT_PlayerInterface.h"
#include "SaT_RandomPlayer.generated.h"

class USaT_GameInstance;
class AGridManager;
class AUnit;

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_RandomPlayer : public APawn, public ISaT_PlayerInterface
{
    GENERATED_BODY()

public:
    // Sets default values for this pawn's properties
    ASaT_RandomPlayer();

    // Riferimenti ai Blueprint delle unità
    UPROPERTY(EditDefaultsOnly, Category = "Units")
    TSubclassOf<class ASniper> SniperClass;

    UPROPERTY(EditDefaultsOnly, Category = "Units")
    TSubclassOf<class ABrawler> BrawlerClass;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // ISaT_PlayerInterface implementation
    virtual void OnTurn() override;
    virtual void OnWin() override;
    virtual void OnLose() override;

private:
    // References
    UPROPERTY()
    USaT_GameInstance* GameInstance;

    UPROPERTY()
    AGridManager* GridManager;

    // AI methods
    void PlaceRandomUnit();
    bool FindRandomEmptyCell(int32& OutGridX, int32& OutGridY);

};