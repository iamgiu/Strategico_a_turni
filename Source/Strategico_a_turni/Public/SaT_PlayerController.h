// Fill out your copyright notice in the Description page of Project Settings.

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

	ASaT_PlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* SaTContext;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ClickAction;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void ClickOnGrid(const FInputActionValue& Value);

protected:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	// Velocità di movimento della camera
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraPanSpeed = 1000.0f;

	void DebugInputState();
};