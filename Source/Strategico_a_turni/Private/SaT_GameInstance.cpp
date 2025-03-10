// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameInstance.h"

void USaT_GameInstance::TossCoin()
{
    // 50% di probabilità
    bPlayerStartsFirst = (FMath::RandRange(0, 1) == 1);
    bIsPlayerTurn = bPlayerStartsFirst;
}

void USaT_GameInstance::SwitchTurn()
{
    bIsPlayerTurn = !bIsPlayerTurn;
}

bool USaT_GameInstance::CheckGameOver()
{
    // Implementa qui la tua logica di fine gioco
    // Ad esempio: return bPlayerUnitsRemaining == 0 || bAIUnitsRemaining == 0;
    return false;
}