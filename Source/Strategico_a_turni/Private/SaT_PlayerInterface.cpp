// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_PlayerInterface.h"

void ISaT_PlayerInterface::OnDraw()
{
    if (GameInstance)
    {
        // Optional: Set a draw message in GameInstance if needed
        UE_LOG(LogTemp, Warning, TEXT("Player notified of game draw"));
    }
}