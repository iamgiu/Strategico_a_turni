#include "SaT_PlayerController.h"
#include "SaT_HumanPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "SaT_PlayerController.h"
#include "SaT_HumanPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ASaT_PlayerController::ASaT_PlayerController()
{
    // Costruttore
}

void ASaT_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Imposta il mapping context se stai usando Enhanced Input System
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (SaTContext)
        {
            Subsystem->AddMappingContext(SaTContext, 0);
        }
    }
}

/*void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Binding per Enhanced Input System
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ASaT_PlayerController::ClickOnGrid);
        }
    }
}*/

void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Binding per Enhanced Input System
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ASaT_PlayerController::ClickOnGrid);
        }

        // Aggiungi input per il movimento della camera (se stai usando l'input tradizionale)
        InputComponent->BindAxis("PanCameraX", this, &ASaT_PlayerController::PanCameraX);
        InputComponent->BindAxis("PanCameraY", this, &ASaT_PlayerController::PanCameraY);
    }
}

// Implementazione delle funzioni di movimento della camera
void ASaT_PlayerController::PanCameraX(float Value)
{
    if (Value != 0.0f)
    {
        ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
        if (HumanPlayer)
        {
            FVector Location = HumanPlayer->GetActorLocation();
            Location.X += Value * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
            HumanPlayer->SetActorLocation(Location);
        }
    }
}

void ASaT_PlayerController::PanCameraY(float Value)
{
    if (Value != 0.0f)
    {
        ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
        if (HumanPlayer)
        {
            FVector Location = HumanPlayer->GetActorLocation();
            Location.Y += Value * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
            HumanPlayer->SetActorLocation(Location);
        }
    }
}

void ASaT_PlayerController::ClickOnGrid(const FInputActionValue& Value)
{
    // Ottiene il pawn come SaT_HumanPlayer e chiama OnClick
    ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
    if (HumanPlayer)
    {
        HumanPlayer->OnClick();
    }
}