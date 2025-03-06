// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameMode.h"
#include "SaT_PlayerController.h"

ASaT_GameMode::ASaT_GameMode()
{
    // Imposta il PlayerController predefinito come il tuo controller personalizzato
    PlayerControllerClass = ASaT_PlayerController::StaticClass();

    // Se hai una classe Pawn personalizzata, puoi impostarla qui
    // DefaultPawnClass = ATuaPawnClass::StaticClass();
}

void ASaT_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Avvia il lancio della moneta quando inizia il gioco
    CoinToss();
}

void ASaT_GameMode::CoinToss()
{
    // Implementa la logica per determinare chi inizia
    bool bPlayerFirst = FMath::RandBool();

    // Notifica i risultati
    OnCoinTossComplete(bPlayerFirst);

    // Ottieni il player controller e inizia il piazzamento
    if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
    {
        if (ASaT_PlayerController* SaT_Player = Cast<ASaT_PlayerController>(Player))
        {
            // Se il giocatore è il primo, inizia subito il piazzamento
            if (bPlayerFirst)
            {
                SaT_Player->StartPlacingUnits();
            }
            // Altrimenti dovresti implementare la logica per l'AI
        }
    }
}


