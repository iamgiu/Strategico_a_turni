// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GridManager.h"
#include "Sniper.h"
#include "Brawler.h"
#include "SaT_GameMode.h"

ASaT_PlayerController::ASaT_PlayerController()
{
    CurrentPlacementState = EPlacementState::NotPlacing;
    SelectedGridX = -1;
    SelectedGridY = -1;
}

void ASaT_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Trova il GridManager nel livello
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        GridManager = Cast<AGridManager>(FoundActors[0]);
    }
}

void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Configura l'input per il click del mouse e per la conferma
    InputComponent->BindAction("Select", IE_Pressed, this, &ASaT_PlayerController::HandleGridSelection);
    InputComponent->BindAction("Confirm", IE_Pressed, this, &ASaT_PlayerController::ConfirmPlacement);
}

void ASaT_PlayerController::StartPlacingUnits()
{
    // Inizia con il piazzamento dello Sniper
    CurrentPlacementState = EPlacementState::PlacingSniper;

    // Qui potresti mostrare un'UI che indica all'utente di piazzare lo Sniper
}

void ASaT_PlayerController::HandleGridSelection()
{
    if (CurrentPlacementState == EPlacementState::NotPlacing ||
        CurrentPlacementState == EPlacementState::PlacementComplete)
    {
        return;
    }

    FHitResult HitResult;
    GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult);

    if (HitResult.GetActor())
    {
        // Verifica se abbiamo cliccato su un Tile
        ATile* SelectedTile = Cast<ATile>(HitResult.GetActor());
        if (SelectedTile)
        {
            // Ottieni le coordinate della griglia dal Tile
            SelectedGridX = SelectedTile->GridX;
            SelectedGridY = SelectedTile->GridY;

            // Evidenzia il tile selezionato
            // SelectedTile->Highlight(true);
        }
    }
}

bool ASaT_PlayerController::PlaceSniper(int32 GridX, int32 GridY)
{
    if (!GridManager || CurrentPlacementState != EPlacementState::PlacingSniper)
    {
        return false;
    }

    // Verifica se la cella è libera
    if (GridManager->IsCellOccupied(GridX, GridY))
    {
        return false;
    }

    // Crea e posiziona lo Sniper
    FVector WorldLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    SniperUnit = GetWorld()->SpawnActor<AUnit>(ASniper::StaticClass(), WorldLocation, FRotator::ZeroRotator, SpawnParams);

    if (SniperUnit)
    {
        // Registra la posizione nella griglia
        GridManager->OccupyCell(GridX, GridY, SniperUnit);

        // Passa al piazzamento del Brawler
        CurrentPlacementState = EPlacementState::PlacingBrawler;
        return true;
    }

    return false;
}

bool ASaT_PlayerController::PlaceBrawler(int32 GridX, int32 GridY)
{
    if (!GridManager || CurrentPlacementState != EPlacementState::PlacingBrawler)
    {
        return false;
    }

    // Verifica se la cella è libera
    if (GridManager->IsCellOccupied(GridX, GridY))
    {
        return false;
    }

    // Crea e posiziona il Brawler
    FVector WorldLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    BrawlerUnit = GetWorld()->SpawnActor<AUnit>(ABrawler::StaticClass(), WorldLocation, FRotator::ZeroRotator, SpawnParams);

    if (BrawlerUnit)
    {
        // Registra la posizione nella griglia
        GridManager->OccupyCell(GridX, GridY, BrawlerUnit);

        // Piazzamento completato
        CurrentPlacementState = EPlacementState::PlacementComplete;
        return true;
    }

    return false;
}

void ASaT_PlayerController::ConfirmPlacement()
{
    if (SelectedGridX != -1 && SelectedGridY != -1)
    {
        if (CurrentPlacementState == EPlacementState::PlacingSniper)
        {
            if (PlaceSniper(SelectedGridX, SelectedGridY))
            {
                // Reset delle coordinate selezionate
                SelectedGridX = -1;
                SelectedGridY = -1;

                // Qui potresti mostrare un messaggio che chiede di piazzare il Brawler
            }
        }
        else if (CurrentPlacementState == EPlacementState::PlacingBrawler)
        {
            if (PlaceBrawler(SelectedGridX, SelectedGridY))
            {
                // Reset delle coordinate selezionate
                SelectedGridX = -1;
                SelectedGridY = -1;

                // Notifica che il piazzamento è completo
                // Ad esempio, passa il turno all'avversario
            }
        }
    }
}