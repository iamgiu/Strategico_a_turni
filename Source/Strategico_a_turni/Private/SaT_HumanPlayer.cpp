// Fill out your copyright notice in the Description page of Project Settings.


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
#include "Components/Button.h"
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

    if (UnitSelectionWidgetClass)
    {
        UE_LOG(LogTemp, Display, TEXT("UnitSelectionWidgetClass è impostato correttamente"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UnitSelectionWidgetClass NON è impostato! Configura questa proprietà nel Blueprint"));
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
    UE_LOG(LogTemp, Display, TEXT("OnClick chiamato"));

    // Ottieni il controller del giocatore
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        FHitResult HitResult;
        PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

        if (HitResult.bBlockingHit)
        {
            // Seleziona la cella
            LastSelectedCell = HitResult.Location;

            // Converti la posizione 3D in coordinate della griglia
            // Utilizza GetXYPositionByRelativeLocation del GridManager se disponibile
            if (GridManager)
            {
                FVector2D GridPosition = GridManager->GetXYPositionByRelativeLocation(HitResult.Location);
                SelectedGridX = FMath::FloorToInt(GridPosition.X);
                SelectedGridY = FMath::FloorToInt(GridPosition.Y);

                UE_LOG(LogTemp, Warning, TEXT("Cella selezionata in coordinate grid: X=%d, Y=%d"),
                    SelectedGridX, SelectedGridY);

                // Verifica se la cella è occupata usando GridManager
                if (!GridManager->IsCellOccupied(SelectedGridX, SelectedGridY))
                {
                    //UE_LOG(LogTemp, Display, TEXT("Cella libera! Mostro il widget di selezione unità"));
                    ShowUnitSelectionWidget();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Cella occupata! Non è possibile piazzare un'unità qui"));
                }
            }
        }
    }
}

void ASaT_HumanPlayer::ShowUnitSelectionWidget()
{
    // Ottieni il controller del giocatore
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        // Verifica se esiste già un'istanza del widget
        if (UnitSelectionWidget)
        {
            UnitSelectionWidget->RemoveFromParent();
            UnitSelectionWidget = nullptr;
        }

        // Crea il widget di selezione unità
        UnitSelectionWidget = CreateWidget<UUserWidget>(PC, UnitSelectionWidgetClass);
        if (UnitSelectionWidget)
        {
            // Trova i pulsanti nel widget
            UButton* SniperButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("SniperButton")));
            UButton* BrawlerButton = Cast<UButton>(UnitSelectionWidget->GetWidgetFromName(TEXT("BrawlerButton")));

            // Collega i pulsanti direttamente alle funzioni
            if (SniperButton)
            {
                SniperButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetSniperSelected);
                UE_LOG(LogTemp, Display, TEXT("SniperButton trovato e collegato alla funzione"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("SniperButton non trovato nel widget!"));
            }

            if (BrawlerButton)
            {
                BrawlerButton->OnClicked.AddDynamic(this, &ASaT_HumanPlayer::OnUnitWidgetBrawlerSelected);
                UE_LOG(LogTemp, Display, TEXT("BrawlerButton trovato e collegato alla funzione"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("BrawlerButton non trovato nel widget!"));
            }

            // Mostra il widget
            UnitSelectionWidget->AddToViewport();

            // Imposta il focus sul widget e mostra il cursore
            PC->SetInputMode(FInputModeUIOnly());
            PC->bShowMouseCursor = true;
        }
        else
        {
            //UE_LOG(LogTemp, Error, TEXT("Impossibile creare il widget di selezione unità!"));
        }
    }
}

void ASaT_HumanPlayer::OnUnitWidgetSniperSelected()
{
    // Nasconde il widget
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Ripristina la modalità di input normale
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = true; // Mantieni il cursore visibile
    }

    // Piazza lo Sniper alle coordinate selezionate
    PlaceUnit(SelectedGridX, SelectedGridY, true);
}

void ASaT_HumanPlayer::OnUnitWidgetBrawlerSelected()
{
    // Nasconde il widget
    if (UnitSelectionWidget)
    {
        UnitSelectionWidget->RemoveFromParent();
        UnitSelectionWidget = nullptr;
    }

    // Ripristina la modalità di input normale
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = true; // Mantieni il cursore visibile
    }

    // Piazza il Brawler alle coordinate selezionate
    PlaceUnit(SelectedGridX, SelectedGridY, false);
}

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