// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_RandomPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "GridManager.h"
#include "SaT_GameInstance.h"
#include "Sniper.h"
#include "Brawler.h"

// Sets default values
ASaT_RandomPlayer::ASaT_RandomPlayer()
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASaT_RandomPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Ottieni riferimento al GameInstance
    GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    // Trova la griglia nella scena
    TArray<AActor*> FoundGrids;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
    if (FoundGrids.Num() > 0)
    {
        GridManager = Cast<AGridManager>(FoundGrids[0]);
    }
}

// Called every frame
void ASaT_RandomPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASaT_RandomPlayer::OnTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player Turn Started"));

    // Usando un timer per dare un po' di "pensiero" all'AI
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASaT_RandomPlayer::PlaceRandomUnit, 1.5f, false);
}

void ASaT_RandomPlayer::PlaceRandomUnit()
{
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager non trovato in SaT_RandomPlayer"));

        // Passa comunque il turno anche in caso di errore
        if (GameInstance)
        {
            GameInstance->SwitchTurn();
        }
        return;
    }

    // Trova una cella vuota in modo casuale
    int32 GridX, GridY;
    bool bFoundEmptyCell = FindRandomEmptyCell(GridX, GridY);

    if (bFoundEmptyCell)
    {
        // Decidi casualmente se piazzare uno Sniper o un Brawler (50% di probabilità)
        bool bIsSniper = FMath::RandBool();

        // Calcola la posizione 3D dalla griglia
        FVector SpawnLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);

        // Configura i parametri di spawn
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Spawna l'unità appropriata
        if (bIsSniper)
        {
            AUnit* Unit = GetWorld()->SpawnActor<ASniper>(ASniper::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            Unit->bIsPlayerUnit = false; // Imposta come unità AI
            UE_LOG(LogTemp, Warning, TEXT("AI: Sniper piazzato in %d, %d"), GridX, GridY);
        }
        else
        {
            AUnit* Unit = GetWorld()->SpawnActor<ABrawler>(ABrawler::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            Unit->bIsPlayerUnit = false; // Imposta come unità AI
            UE_LOG(LogTemp, Warning, TEXT("AI: Brawler piazzato in %d, %d"), GridX, GridY);
        }

        // Marca la cella come occupata
        GridManager->OccupyCell(GridX, GridY, nullptr); // Aggiorna questo per passare l'unità corretta
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AI: Non è stata trovata una cella vuota valida"));
    }

    // Passa il turno
    if (GameInstance)
    {
        GameInstance->SwitchTurn();
    }
}

bool ASaT_RandomPlayer::FindRandomEmptyCell(int32& OutGridX, int32& OutGridY)
{
    if (!GridManager) return false;

    // Dimensione della griglia
    int32 GridSize = 25; // Questo dovrebbe corrispondere alla dimensione in GridManager

    // Numero massimo di tentativi per trovare una cella vuota
    const int32 MaxAttempts = 100;

    for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
    {
        // Genera coordinate casuali
        OutGridX = FMath::RandRange(0, GridSize - 1);
        OutGridY = FMath::RandRange(0, GridSize - 1);

        // Verifica se la cella è libera
        if (!GridManager->IsCellOccupied(OutGridX, OutGridY))
        {
            return true;
        }
    }

    return false;
}

void ASaT_RandomPlayer::OnWin()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player ha vinto!"));
}

void ASaT_RandomPlayer::OnLose()
{
    UE_LOG(LogTemp, Warning, TEXT("AI Player ha perso!"));
}