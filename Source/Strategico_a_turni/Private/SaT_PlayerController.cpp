// Fill out your copyright notice in the Description page of Project Settings.


#include "Sat_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SaT_HumanPlayer.h"


ASaT_PlayerController::ASaT_PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
}

void ASaT_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(SaTContext, 0);
	}
}

void ASaT_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ASaT_PlayerController::ClickOnGrid);
	}
}


void ASaT_PlayerController::ClickOnGrid(const FInputActionValue& Value)
{
	const auto HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
	if (IsValid(HumanPlayer))
	{
		HumanPlayer->OnClick();
	}
}