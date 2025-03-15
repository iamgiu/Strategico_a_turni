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

	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* SaTContext;


	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ClickAction;

	void ClickOnGrid(const FInputActionValue& Value);

protected:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	// Funzioni per il movimento della camera
	void PanCameraX(float Value);
	void PanCameraY(float Value);

	// Velocità di movimento della camera
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPanSpeed = 500.0f;
};