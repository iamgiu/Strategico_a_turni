// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Unit.h"
#include "GridManager.h"
#include "Tile.h"
#include "SaT_GameMode.h"
#include "SaT_PlayerController.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    
    ASaT_PlayerController();

    UFUNCTION(BlueprintCallable, Category = "Game")
    void StartPlacingUnits();

    // Funzione per piazzare lo Sniper
    UFUNCTION(BlueprintCallable, Category = "Game")
    bool PlaceSniper(int32 GridX, int32 GridY);

    // Funzione per piazzare il Brawler
    UFUNCTION(BlueprintCallable, Category = "Game")
    bool PlaceBrawler(int32 GridX, int32 GridY);

    // Funzione per confermare il piazzamento
    UFUNCTION(BlueprintCallable, Category = "Game")
    void ConfirmPlacement();

protected:

    virtual void BeginPlay() override;

    virtual void SetupInputComponent() override;

    // Gestione dell'input per selezionare la cella
    void HandleGridSelection();

public:

    UPROPERTY()
    class AGridManager* GridManager;

    // Unità da piazzare
    UPROPERTY()
    AUnit* SniperUnit;

    UPROPERTY()
    AUnit* BrawlerUnit;

    // Stato attuale del piazzamento
    enum EPlacementState
    {
        NotPlacing,
        PlacingSniper,
        PlacingBrawler,
        PlacementComplete
    };

    EPlacementState CurrentPlacementState;

    // Coordinated dell'unità selezionata attualmente
    int32 SelectedGridX;
    int32 SelectedGridY;

};
