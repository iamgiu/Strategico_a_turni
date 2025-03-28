// Fill out your copyright notice in the Description page of Project Settings.

//Game Instance class that manages the game state across levels

#pragma once

#include "CoreMinimal.h"
#include "SaT_Enums.h"
#include "Engine/GameInstance.h"
#include "SaT_GameInstance.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API USaT_GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:

    // Constructor - initializes default values 
    USaT_GameInstance();

    // ----------------
    // Game State Properties
    // ----------------

    // Determines who starts the game (true = player, false = AI) 
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    bool bPlayerStartsFirst;

    // Current turn (true = player's turn, false = AI's turn) 
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    bool bIsPlayerTurn;

    // Current AI difficulty level 
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    EAIDifficulty AIDifficulty;

    // Current phase of the game (setup, playing, game over)
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    EGamePhase CurrentPhase;

    // Number of units the human player has placed during setup
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    int32 HumanUnitsPlaced;

    // Number of units the AI has placed during setup
    UPROPERTY(BlueprintReadWrite, Category = "Game")
    int32 AIUnitsPlaced;

    // Current turn number, increments after both players have moved
    int32 CurrentTurnNumber;

    // ----------------
    // Game State Functions
    // ----------------

    // Checks if the setup phase is complete (both players have placed their units)
    UFUNCTION(BlueprintCallable, Category = "Game")
    bool IsSetupComplete() const;

    // Tosses a coin to randomly decide who starts the game
    UFUNCTION(BlueprintCallable, Category = "Game")
    void TossCoin();

    // Switches the turn between players and handles phase transitions
    UFUNCTION(BlueprintCallable, Category = "Game")
    void SwitchTurn();

    // Checks if the game is over 
    UFUNCTION(BlueprintCallable, Category = "Game")
    bool CheckGameOver();

    // Sets the current game phase
    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetGamePhase(EGamePhase NewPhase);

    // Gets the current game phase
    UFUNCTION(BlueprintPure, Category = "Game")
    EGamePhase GetGamePhase() const;

    // Sets the AI difficulty level
    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetAIDifficulty(EAIDifficulty NewDifficulty);

    // Sets up the game with the selected difficulty and generates the grid
    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetupGameWithDifficulty(EAIDifficulty Difficulty);

};