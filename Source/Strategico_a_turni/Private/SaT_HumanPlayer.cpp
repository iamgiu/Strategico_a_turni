// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_HumanPlayer.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GridManager.h"
#include <Kismet/GameplayStatics.h>

ASaT_HumanPlayer::ASaT_HumanPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crea un SceneComponent come root
    DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = DefaultSceneRoot;

    // Crea la camera come componente figlio
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeLocation(FVector(0, 0, 400));
    Camera->SetRelativeRotation(FRotator(-90, 0, 0));
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Inizializzazione base
    SelectionState = 0;
    CurrentPhase = EGamePhase::SETUP;
    UnitsToPlace = 2;

    // Trova il GridManager nella scena
    TArray<AActor*> FoundGrids;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
    if (FoundGrids.Num() > 0)
    {
        GridManager = Cast<AGridManager>(FoundGrids[0]);
        UE_LOG(LogTemp, Display, TEXT("GridManager trovato e assegnato"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GridManager non trovato nella scena!"));
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
    // Funzionalit� base
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
    // Questo posizioner� uno Sniper nella posizione 5,5
    PlaceUnit(5, 5, true);
}

void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{
    UE_LOG(LogTemp, Display, TEXT("PlaceUnit chiamato: GridX=%d, GridY=%d, IsSniper=%d"),
        GridX, GridY, bIsSniper ? 1 : 0);

    // Verifica se GridManager � valido
    if (GridManager)
    {

        // Verifica se la cella � gi� occupata
        if (!GridManager->IsCellOccupied(GridX, GridY))
        {
            UE_LOG(LogTemp, Display, TEXT("Cella %d,%d libera, procedo con il piazzamento"), GridX, GridY);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Cella %d,%d gi� occupata!"), GridX, GridY);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GridManager non trovato!"));
    }
}




/*#include "SaT_HumanPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Unit.h"
#include "Sniper.h"
#include "Brawler.h"
#include "GridManager.h"

ASaT_HumanPlayer::ASaT_HumanPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crea un SceneComponent come root
    DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent = DefaultSceneRoot;

    // Crea la camera come componente figlio
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeLocation(FVector(0, 0, 400)); // Posiziona sopra la griglia
    Camera->SetRelativeRotation(FRotator(-90, 0, 0)); // Guarda verso il basso
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Ottieni referenza al GameInstance
    GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    SelectionState = 0;

    // Inizializza valori di default
    CurrentPhase = EGamePhase::SETUP;
    UnitsToPlace = 2; // Assumiamo 2 unit� come da specifiche di gioco
}

void ASaT_HumanPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASaT_HumanPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Input gestito nel PlayerController
}

void ASaT_HumanPlayer::OnTurn()
{
    // Abilita l'input quando � il turno del giocatore
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        EnableInput(PC);
    }

    // Resetta lo stato di selezione
    SelectionState = 0;

    UE_LOG(LogTemp, Display, TEXT("Turno del giocatore umano"));
}

void ASaT_HumanPlayer::OnWin()
{
    // Implementa qui la logica di vittoria
    UE_LOG(LogTemp, Display, TEXT("Hai vinto!"));
}

void ASaT_HumanPlayer::OnLose()
{
    // Implementa qui la logica di sconfitta
    UE_LOG(LogTemp, Display, TEXT("Hai perso!"));
}

void ASaT_HumanPlayer::OnClick()
{
    // Ottieni il controller del giocatore
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (PC)
    {
        FHitResult HitResult;
        PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

        if (HitResult.bBlockingHit)
        {
            if (CurrentPhase == EGamePhase::SETUP)
            {
                // Fase di piazzamento unit�
                if (SelectionState == 0)
                {
                    // Seleziona la cella
                    LastSelectedCell = HitResult.Location;

                    // Converti la posizione 3D in coordinate della griglia
                    SelectedGridX = FMath::FloorToInt(LastSelectedCell.X / 100.0f); // Assumo celle di 100 unit�
                    SelectedGridY = FMath::FloorToInt(LastSelectedCell.Y / 100.0f);

                    // Verifica se la cella � occupata usando GridManager
                    if (GridManager && !GridManager->IsCellOccupied(SelectedGridX, SelectedGridY))
                    {
                        // Passa allo stato successivo
                        SelectionState = 1;
                        //UE_LOG(LogTemp, Display, TEXT("Cella selezionata: %d, %d - Seleziona tipo unit�"), SelectedGridX, SelectedGridY);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Cella occupata o non valida!"));
                    }
                }
                else if (SelectionState == 1)
                {
                    // Questa parte andrebbe normalmente gestita da un UI Widget
                    // Per ora, assumiamo che un click nella met� superiore dello schermo = Sniper
                    // e nella met� inferiore = Brawler

                    float MouseX, MouseY;
                    PC->GetMousePosition(MouseX, MouseY); // Ottieni la posizione del mouse

                    int32 ViewportX, ViewportY;
                    PC->GetViewportSize(ViewportX, ViewportY); // Ottieni la dimensione della viewport

                    bool bIsSniper = MouseY < (ViewportY / 2); // Confronta solo la coordinata Y

                    // Piazza l'unit�
                    PlaceUnit(SelectedGridX, SelectedGridY, bIsSniper);

                    // Resetta lo stato e passa il turno se tutte le unit� sono state piazzate
                    SelectionState = 0;
                    UnitsToPlace--;

                    if (UnitsToPlace <= 0 && GameInstance)
                    {
                        // Tutte le unit� sono state piazzate, passa alla fase di battaglia
                        CurrentPhase = EGamePhase::PLAYING;
                        GameInstance->SwitchTurn();
                    }
                }
            }
            else if (CurrentPhase == EGamePhase::PLAYING)
            {
                // Implementa qui la logica per selezionare e muovere le unit� in battaglia
                UE_LOG(LogTemp, Display, TEXT("Fase di battaglia - da implementare"));
            }
        }
    }
}

void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{
    // Calcola la posizione 3D dalla griglia
    FVector SpawnLocation;

    // Se GridManager � valido, usa la sua funzione per ottenere la posizione nel mondo
    if (GridManager)
    {
        SpawnLocation = GridManager->GetWorldLocationFromGrid(GridX, GridY);
    }
    else
    {
        // Fallback se GridManager non � valido
        SpawnLocation = FVector(GridX * 100.0f, GridY * 100.0f, 0.0f);
    }

    // Crea l'unit� appropriata
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AUnit* PlacedUnit = nullptr;

    if (bIsSniper)
    {
        // Spawn Sniper
        PlacedUnit = GetWorld()->SpawnActor<ASniper>(ASniper::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Display, TEXT("Sniper piazzato in %d, %d"), GridX, GridY);
    }
    else
    {
        // Spawn Brawler
        PlacedUnit = GetWorld()->SpawnActor<ABrawler>(ABrawler::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Display, TEXT("Brawler piazzato in %d, %d"), GridX, GridY);
    }

    // Configura l'unit�
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
}*/
