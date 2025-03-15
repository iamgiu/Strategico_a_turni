// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_HumanPlayer.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GridManager.h"
#include <Kismet/GameplayStatics.h>
#include "Sniper.h"
#include "Brawler.h"
#include "Unit.h"
#include "Blueprint/UserWidget.h"
#include "SaT_GameInstance.h"

ASaT_HumanPlayer::ASaT_HumanPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crea la camera direttamente come root
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    SetRootComponent(Camera);

    // Configurazione della camera per la visuale ortografica
    Camera->SetRelativeRotation(FRotator(-90, 0, 0)); // Guarda verso il basso
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;

    // Calcola l'OrthoWidth in base alla dimensione della griglia
    // Assumiamo un TileSize di 100 e una griglia 25x25
    float GridSize = 25 * 100.0f;
    Camera->OrthoWidth = GridSize * 2.1f; // Un po' di margine intorno alla griglia

    // Altezza della camera - abbastanza alta da vedere tutto il campo
    float CameraHeight = 2000.0f;
    Camera->SetRelativeLocation(FVector(GridSize / 2, GridSize / 2, CameraHeight));
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Inizializzazione base
    SelectionState = 0;
    CurrentPhase = EGamePhase::SETUP;
    UnitsToPlace = 2;

    // Trova il GridManager
    TArray<AActor*> FoundGrids;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
    if (FoundGrids.Num() > 0)
    {
        GridManager = Cast<AGridManager>(FoundGrids[0]);
        if (GridManager)
        {
            UE_LOG(LogTemp, Display, TEXT("GridManager trovato e assegnato"));

            // Posiziona la camera sopra il centro della griglia
            float GridSize = GridManager->Size * GridManager->TileSize;
            float CameraHeight = 2000.0f;

            // Posiziona la camera al centro della griglia
            SetActorLocation(FVector(GridSize / 2, GridSize / 2, CameraHeight));

            UE_LOG(LogTemp, Warning, TEXT("Camera posizionata al centro della griglia: X=%f, Y=%f, Z=%f"),
                GridSize / 2, GridSize / 2, CameraHeight);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GridManager non trovato nella scena!"));
        }
    }
}

void ASaT_HumanPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASaT_HumanPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Input gestito dal controller
}

void ASaT_HumanPlayer::OnTurn()
{
    // Funzionalità base
    SelectionState = 0;
}

void ASaT_HumanPlayer::OnWin()
{
    // Base implementation
}

void ASaT_HumanPlayer::OnLose()
{
    // Base implementation
}

void ASaT_HumanPlayer::OnClick()
{
    // Funzione ridotta al minimo
    UE_LOG(LogTemp, Display, TEXT("OnClick chiamato"));

    // Aggiungi una chiamata di test a PlaceUnit
    // Questo posizionerà uno Sniper nella posizione 5,5
    PlaceUnit(5, 5, true);

    // Ottieni il controller del giocatore
    //APlayerController* PC = GetWorld()->GetFirstPlayerController();


/*    if (PC)
    {
        FHitResult HitResult;
        PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

        if (HitResult.bBlockingHit)
        {
            // Seleziona la cella
            LastSelectedCell = HitResult.Location;

            // Converti la posizione 3D in coordinate della griglia
            SelectedGridX = FMath::FloorToInt(LastSelectedCell.X / 100.0f);
            SelectedGridY = FMath::FloorToInt(LastSelectedCell.Y / 100.0f);

            // Verifica se la cella è occupata usando GridManager
            if (GridManager && !GridManager->IsCellOccupied(SelectedGridX, SelectedGridY))
            {
                UE_LOG(LogTemp, Display, TEXT("Cella selezionata: %d, %d"), SelectedGridX, SelectedGridY);

                if (UnitSelectionWidgetClass)
                {
                    // Elimina widget esistenti
                    if (UnitSelectionWidget)
                    {
                        UnitSelectionWidget->RemoveFromParent();
                        UnitSelectionWidget = nullptr;
                    }

                    // Crea il widget
                    UnitSelectionWidget = CreateWidget<UUserWidget>(PC, UnitSelectionWidgetClass);
                    if (UnitSelectionWidget)
                    {
                        // Mostra il widget
                        UnitSelectionWidget->AddToViewport();

                        // Imposta l'input mode per gestire il widget
                        PC->SetInputMode(FInputModeUIOnly());
                        PC->bShowMouseCursor = true;
                    }
                }
            }
        }
    }*/
}

/*void ASaT_HumanPlayer::OnUnitWidgetSniperSelected()
{
    // Nasconde il widget
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent(); // Usa RemoveFromParent invece di RemoveFromViewport

        // Ripristina la modalità di input normale
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->SetInputMode(FInputModeGameOnly());
            PC->bShowMouseCursor = false;
        }
    }

    // Piazza lo Sniper alle coordinate selezionate
    PlaceUnit(SelectedGridX, SelectedGridY, true);
}*/

/*void ASaT_HumanPlayer::OnUnitWidgetBrawlerSelected()
{
    // Nasconde il widget
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent(); // Usa RemoveFromParent invece di RemoveFromViewport

        // Ripristina la modalità di input normale
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->SetInputMode(FInputModeGameOnly());
            PC->bShowMouseCursor = false;
        }
    }

    // Piazza il Brawler alle coordinate selezionate
    PlaceUnit(SelectedGridX, SelectedGridY, false);
}*/

void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{
    // Aggiungi log dettagliati
    UE_LOG(LogTemp, Warning, TEXT("PlaceUnit chiamato con coordinate: X=%d, Y=%d"), GridX, GridY);

    // Controlla che le coordinate siano all'interno della griglia
    if (GridX < 0 || GridX >= 25 || GridY < 0 || GridY >= 25)
    {
        UE_LOG(LogTemp, Error, TEXT("Coordinate fuori griglia!"));
        return;
    }

    // Calcola la posizione 3D dalla griglia
    FVector SpawnLocation;
    if (GridManager)
    {
        SpawnLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);
        UE_LOG(LogTemp, Warning, TEXT("Posizione di spawn calcolata: X=%f, Y=%f, Z=%f"),
            SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }
    else
    {
        // Fallback se GridManager non è valido
        SpawnLocation = FVector(GridX * 100.0f, GridY * 100.0f, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("GridManager non valido! Usando fallback: X=%f, Y=%f, Z=%f"),
            SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }

    // Crea l'unità appropriata
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AUnit* PlacedUnit = nullptr;

    if (bIsSniper)
    {
        // Usa SniperClass invece di ASniper::StaticClass()
        PlacedUnit = GetWorld()->SpawnActor<ASniper>(SniperClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Display, TEXT("Sniper piazzato in %d, %d"), GridX, GridY);
    }
    else
    {
        // Usa BrawlerClass invece di ABrawler::StaticClass()
        PlacedUnit = GetWorld()->SpawnActor<ABrawler>(BrawlerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Display, TEXT("Brawler piazzato in %d, %d"), GridX, GridY);
    }
    // Configura l'unità
    if (PlacedUnit)
    {
        PlacedUnit->GridX = GridX;
        PlacedUnit->GridY = GridY;
        PlacedUnit->bIsPlayerUnit = true;
        // Aggiorna lo stato della cella nel GridManager
        if (GridManager)
        {
            GridManager->OccupyCell(GridX, GridY, PlacedUnit);
        }
    }
}
