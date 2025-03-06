// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_PlayerController.h"
#include "Kismet/GameplayStatics.h"

void ASaT_PlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;  // Abilita il cursore per cliccare sulla griglia
}

void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("Click", IE_Pressed, this, &ASaT_PlayerController::OnClick);
}

void ASaT_PlayerController::SelectSniper()
{
    SelectedUnitClass = ASniper::StaticClass();
}

void ASaT_PlayerController::SelectBrawler()
{
    SelectedUnitClass = ABrawler::StaticClass();
}

void ASaT_PlayerController::OnClick()
{
    if (!SelectedUnitClass) return;

    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (Hit.bBlockingHit)
    {
        AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
        if (GridManager)
        {
            FVector2D GridPos = GridManager->GetPosition(Hit);
            GridManager->SpawnUnit(GridPos, SelectedUnitClass);
        }
    }
}

