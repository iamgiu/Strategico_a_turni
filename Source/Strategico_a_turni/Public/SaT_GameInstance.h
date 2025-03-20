// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaT_Enums.h"
#include "Engine/GameInstance.h"
#include "SaT_GameInstance.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API USaT_GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:

    USaT_GameInstance();

    // Determina chi inizia il gioco (true = player, false = AI)
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    bool bPlayerStartsFirst;

    // Turno corrente (true = turno del player, false = turno dell'AI)
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    bool bIsPlayerTurn;

    UPROPERTY(BlueprintReadWrite, Category = "Game")
    EGamePhase CurrentPhase;

    // Lancia la moneta per decidere chi inizia
    UFUNCTION(BlueprintCallable, Category = "Game")
    void TossCoin();

    // Gestisce il passaggio del turno
    UFUNCTION(BlueprintCallable, Category = "Game")
    void SwitchTurn();

    // Verifica se il gioco è terminato
    UFUNCTION(BlueprintCallable, Category = "Game")
    bool CheckGameOver();

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetGamePhase(EGamePhase NewPhase);

    // Gets the current game phase
    UFUNCTION(BlueprintPure, Category = "Game")
    EGamePhase GetGamePhase() const;
};