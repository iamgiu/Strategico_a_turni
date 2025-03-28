// Fill out your copyright notice in the Description page of Project Settings.

/*
 * Controller class that handles player input for the strategy game
 * Manages enhanced input setup and click processing
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "SaT_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ASaT_HumanPlayer;

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	// Constructor
	ASaT_PlayerController();

	// -----------------
	// Input Configuration
	// -----------------

	// Input mapping context for strategy game interactions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* SaTContext;

	// Action for handling grid clicks
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ClickAction;

	// Handler for click input events
	UFUNCTION(BlueprintCallable, Category = "Input")
	void ClickOnGrid(const FInputActionValue& Value);

protected:

	// Called when the controller begins play
	virtual void BeginPlay() override;

	// Set up input component bindings
	virtual void SetupInputComponent() override;

	// Camera movement speed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraPanSpeed = 1000.0f;

	// Utility to display current input system state
	void DebugInputState();

};