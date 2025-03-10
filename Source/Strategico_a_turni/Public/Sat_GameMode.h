// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureDefines.h"
#include "GameFramework/GameMode.h"
#include "SaT_HumanPlayer.h"
#include "GridManager.h"
#include "SaT_GameMode.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_GameMode : public AGameMode
{
    GENERATED_BODY()

public:
    ASaT_GameMode();

    virtual void BeginPlay() override;

    // Funzione per il lancio della moneta
    UFUNCTION(BlueprintCallable, Category = "Game")
    void CoinToss();

    // Evento chiamato dopo il lancio della moneta
    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void OnCoinTossComplete(bool bIsPlayerFirst);

    // Riferimento al GridManager
    UPROPERTY(Transient)
    AGridManager* GridManager;

    // Riferimento al giocatore umano
    UPROPERTY(Transient)
    ASaT_HumanPlayer* HumanPlayer;

    // Numero di unità iniziali per giocatore
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Rules")
    int32 InitialUnitsCount = 5;
};