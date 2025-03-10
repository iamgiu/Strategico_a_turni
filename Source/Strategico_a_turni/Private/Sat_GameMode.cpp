// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameMode.h"
#include "GridManager.h"
#include "SaT_HumanPlayer.h"
#include "Kismet/GameplayStatics.h"

ASaT_GameMode::ASaT_GameMode()
{
    // Imposta il PlayerController e il Pawn predefiniti
    PlayerControllerClass = ASaT_PlayerController::StaticClass();
    DefaultPawnClass = ASaT_HumanPlayer::StaticClass();
}

void ASaT_GameMode::BeginPlay()
{
    Super::BeginPlay();

    // Crea il GridManager se non esiste già
    TArray<AActor*> FoundGridManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGridManagers);

    if (FoundGridManagers.Num() == 0)
    {
        // Spawna il GridManager
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GridManager = GetWorld()->SpawnActor<AGridManager>(AGridManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }
    else
    {
        GridManager = Cast<AGridManager>(FoundGridManagers[0]);
    }

    // Ottieni il riferimento al HumanPlayer
    HumanPlayer = Cast<ASaT_HumanPlayer>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

    if (HumanPlayer)
    {
        // Assegna il GridManager al HumanPlayer
        HumanPlayer->GridManager = GridManager;
        HumanPlayer->CurrentPhase = EGamePhase::SETUP;
        HumanPlayer->UnitsToPlace = InitialUnitsCount;
    }

    // Esegui il lancio della moneta per decidere chi inizia
    CoinToss();
}

void ASaT_GameMode::CoinToss()
{
    // Lancio della moneta casuale
    bool bIsPlayerFirst = FMath::RandBool();

    // Chiamare l'evento Blueprint
    OnCoinTossComplete(bIsPlayerFirst);

    // Se il giocatore inizia per primo, attiva il suo turno
    if (bIsPlayerFirst && HumanPlayer)
    {
        HumanPlayer->OnTurn();
    }
    else
    {
        // Gestione del turno dell'AI (da implementare)
        // ...

        // Temporaneamente, passa comunque al giocatore
        if (HumanPlayer)
        {
            HumanPlayer->OnTurn();
        }
    }
}
}


