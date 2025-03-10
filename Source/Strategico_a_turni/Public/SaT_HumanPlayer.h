// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "SaT_PlayerInterface.h"
#include "Unit.h"
#include "SaT_GameInstance.h"
#include "SaT_HumanPlayer.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_HumanPlayer : public APawn, public ISaT_PlayerInterface
{
    GENERATED_BODY()

public:
    // Camera attaccata al giocatore
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    UCameraComponent* Camera;

    ASaT_HumanPlayer();

    // Chiamato ogni frame
    virtual void Tick(float DeltaTime) override;

    // Configura l'input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Metodi di interfaccia
    virtual void OnTurn() override;
    virtual void OnWin() override;
    virtual void OnLose() override;

    // Gestisce i click del mouse
    UFUNCTION()
    void OnClick();

    // Piazza una nuova unità sulla griglia
    UFUNCTION(BlueprintCallable, Category = "Gameplay")
    void PlaceUnit(int32 GridX, int32 GridY, bool bIsSniper);

protected:
    virtual void BeginPlay() override;

    // Riferimento al GameInstance
    USaT_GameInstance* GameInstance;

    // Cella selezionata
    int32 SelectedGridX;
    int32 SelectedGridY;

    // Flag per sapere cosa stiamo facendo
    // 0 = Seleziona cella
    // 1 = Scegli tipo di unità (Sniper o Brawler)
    int32 SelectionState;

    // Ultima cella selezionata
    FVector LastSelectedCell;
};