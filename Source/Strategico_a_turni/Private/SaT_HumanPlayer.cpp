// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_HumanPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sniper.h"
#include "Camera/CameraComponent.h"
#include "Brawler.h"


ASaT_HumanPlayer::ASaT_HumanPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crea la camera
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    RootComponent = Camera;
    Camera->SetRelativeLocation(FVector(0, 0, 400)); // Posiziona sopra la griglia
    Camera->SetRelativeRotation(FRotator(-90, 0, 0)); // Guarda verso il basso
}

void ASaT_HumanPlayer::BeginPlay()
{
    Super::BeginPlay();


    // Ottieni referenza al GameInstance
    GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    SelectionState = 0;


}

void ASaT_HumanPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASaT_HumanPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Collega la funzione OnClick all'input del mouse
    PlayerInputComponent->BindAction("Click", IE_Pressed, this, &ASaT_HumanPlayer::OnClick);
}

void ASaT_HumanPlayer::OnTurn()
{
    // Abilita l'input quando è il turno del giocatore
    EnableInput(UGameplayStatics::GetPlayerController(GetWorld(), 0));

    // Resetta lo stato di selezione
    SelectionState = 0;
}

void ASaT_HumanPlayer::OnWin()
{
    // Implementa qui la logica di vittoria
}

void ASaT_HumanPlayer::OnLose()
{
    // Implementa qui la logica di sconfitta
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
            if (SelectionState == 0)
            {
                // Seleziona la cella
                LastSelectedCell = HitResult.Location;

                // Converti la posizione 3D in coordinate della griglia
                SelectedGridX = FMath::FloorToInt(LastSelectedCell.X / 100.0f); // Assumo celle di 100 unità
                SelectedGridY = FMath::FloorToInt(LastSelectedCell.Y / 100.0f);

                // Passa allo stato successivo
                SelectionState = 1;

                // Qui potresti aprire un menu di selezione per Sniper o Brawler
                // Per ora usiamo un log per debug
                //UE_LOG(LogTemp, Warning, TEXT("Cella selezionata: %d, %d - Seleziona tipo unità"), SelectedGridX, SelectedGridY);
            }
            else if (SelectionState == 1)
            {
                // Questa parte andrebbe normalmente gestita da un UI Widget
                // Per ora, assumiamo che un click nella metà superiore dello schermo = Sniper
                // e nella metà inferiore = Brawler

                float MouseX, MouseY;
                PC->GetMousePosition(MouseX, MouseY); // Ottieni la posizione del mouse

                int32 ViewportX, ViewportY;
                PC->GetViewportSize(ViewportX, ViewportY); // Ottieni la dimensione della viewport

                bool bIsSniper = MouseY < (ViewportY / 2); // Confronta solo la coordinata Y

                // Piazza l'unità
                PlaceUnit(SelectedGridX, SelectedGridY, bIsSniper);

                // Resetta lo stato e passa il turno
                SelectionState = 0;
                GameInstance->SwitchTurn();
            }
        }
    }
}

void ASaT_HumanPlayer::PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper)
{
    // Calcola la posizione 3D dalla griglia
    FVector SpawnLocation = FVector(GridX * 100.0f, GridY * 100.0f, 0.0f);

    // Crea l'unità appropriata
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (bIsSniper)
    {
        // Spawn Sniper
        GetWorld()->SpawnActor<ASniper>(ASniper::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Warning, TEXT("Sniper piazzato in %d, %d"), GridX, GridY);
    }
    else
    {
        // Spawn Brawler
        GetWorld()->SpawnActor<ABrawler>(ABrawler::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        UE_LOG(LogTemp, Warning, TEXT("Brawler piazzato in %d, %d"), GridX, GridY);
    }
}