// Game Mode class that manages the turn-based strategy game

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Unit.h"
#include "SaT_Enums.h"
#include "SaT_GameMode.generated.h"

class UMainGameHUDClass;
class ISaT_PlayerInterface;
class AGridManager;
class AUnit;

UCLASS()
class STRATEGICO_A_TURNI_API ASaT_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

    // Minimum number of players required to start the game
	static constexpr int8 MIN_NUMBER_SPAWN_PLAYERS = 2;

    // Constructor 
	ASaT_GameMode();

    // Called when the game starts
	virtual void BeginPlay() override;

    // ----------------
    // Game Management
    // ----------------

	// Initializes and starts the game
	void StartGame();

	// Initializes the first turn after game start
	void StartFirstTurn();

	// Randomly determines which player starts the game
	void FlipCoinToDecideFirstPlayer();

	// Switches to the next player (legacy method - use EndTurn)
	void TurnNextPlayer();

	// Ends the current player's turn
	void EndTurn();

	// Checks if the game is over and handles win/lose conditions
	bool CheckGameOver();

	// Checks for a potential draw between two units
	bool CheckPotentialDraw(const TArray<AUnit*>& HumanUnits, const TArray<AUnit*>& AIUnits);

	// Handles special draw conditions in the game 
	void HandleDrawCondition();

	// ----------------
	// Game State Properties
	// ----------------

	// Class of the grid manager to spawn 
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGridManager> GameManagerClass;

	// Reference to the active grid manager
	UPROPERTY(VisibleAnywhere)
	AGridManager* Gmanager;

	// List of all players in the game 
	TArray<ISaT_PlayerInterface*> Players;

	// Index of the current player (0 = Human, 1 = AI) 
	int32 CurrentPlayer;

	// Flag indicating if the game is over 
	bool bIsGameOver;

	// Type of the current player (Human or AI)
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	EPlayerType CurrentPlayerType;

	// Maximum number of log entries to keep
	UPROPERTY(EditDefaultsOnly, Category = "Game Log")
	int32 MaxGameLogEntries = 50;

	// Game log array for storing entries 
	UPROPERTY(BlueprintReadOnly, Category = "Game Log")
	TArray<FString> GameLog;

	// Most recent log entry formatted for display
	UPROPERTY(BlueprintReadOnly, Category = "Game Log")
	FString FormattedEntry;

	// Raw move history without turn numbers
	UPROPERTY(BlueprintReadOnly, Category = "Game Log")
	TArray<FString> RawMoveHistory;

	// ----------------
	// UI Properties 
	// ----------------

	// Currently selected unit
	AUnit* CurrentlySelectedUnit;

	// Sets the currently selected unit
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SetSelectedUnit(AUnit* Unit);

	// Gets information text about the currently selected unit
	UFUNCTION(BlueprintCallable, Category = "UI")
	FString GetSelectedUnitInfoText() const;

	// Unit Health and Position Properties
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 PlayerSniperHP;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 PlayerBrawlerHP;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 AISniperHP;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 AIBrawlerHP;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString PlayerSniperPos;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString PlayerBrawlerPos;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString AISniperPos;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString AIBrawlerPos;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString PlayerSniperHPFormatted;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString PlayerBrawlerHPFormatted;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString AISniperHPFormatted;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString AIBrawlerHPFormatted;

	// Turn Information Properties
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool IsPlayerTurn;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	int32 CurrentTurnNumber;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString TurnText;

	// Updates all UI elements with current game state
	void UpdateGameHUD();

	// ----------------
	// UI Widget Classes
	// ----------------

	// Main HUD widget class
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainGameHUDClass;

	// Widget that shows during AI thinking
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> AIThinkingWidgetClass;

	// Instance of AI thinking widget
	UPROPERTY()
	UUserWidget* AIThinkingWidget;

	// Shows or hides the AI thinking widget
	void ShowAIThinkingWidget(bool bShow);

	// Result text from the coin flip
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FString CoinflipResult;

	// Instance of coin flip result widget
	UPROPERTY()
	UUserWidget* CoinFlipResultWidget;

	// Coin flip result widget class 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> CoinFlipResultWidgetClass;

	// Game over widget class 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> GameOverWidgetClass;

	// Instance of game over widget 
	UPROPERTY()
	UUserWidget* GameOverWidget;

	// Text displayed when game is over
	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	FString WinnerText;

	// Shows or hides the coin flip result widget
	void ShowCoinFlipResultWidget(bool bShow);

	// Hides the coin flip widget
	void HideCoinFlipWidget();

	// Shows or hides the game over widget
	void ShowGameOverWidget(bool bShow);

	// Widget for difficulty selection 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> DifficultySelectionWidgetClass;

	// Instance of difficulty selection widget
	UPROPERTY()
	UUserWidget* DifficultyWidget;

	// Shows or hides the difficulty selection widget
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowDifficultyWidget(bool bShow);

	// Hides the difficulty selection widget */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideDifficultyWidget();

	// ----------------
	// UI Getters for Blueprint Binding
	// ----------------

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetPlayerSniperHPText() const { return FText::AsNumber(PlayerSniperHP); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetPlayerBrawlerHPText() const { return FText::AsNumber(PlayerBrawlerHP); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetAISniperHPText() const { return FText::AsNumber(AISniperHP); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetAIBrawlerHPText() const { return FText::AsNumber(AIBrawlerHP); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetPlayerSniperPosText() const { return FText::FromString(PlayerSniperPos); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetPlayerBrawlerPosText() const { return FText::FromString(PlayerBrawlerPos); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetAISniperPosText() const { return FText::FromString(AISniperPos); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetAIBrawlerPosText() const { return FText::FromString(AIBrawlerPos); }

	UFUNCTION(BlueprintCallable, Category = "UI")
	FText GetTurnText() const { return FText::FromString(TurnText); }

	// ----------------
	// Game Actions
	// ----------------

	/*
	 * Add a formatted move entry to the game log history
	 *
	 * @param bIsPlayerUnit Whether the unit belongs to the human player (true) or AI (false)
	 * @param UnitType The type of unit making the move ("Sniper" or "Brawler")
	 * @param ActionType The type of action ("Move", "Attack", "Skip", "Place")
	 * @param FromPosition The starting position for a move, or the attacker position for an attack
	 * @param ToPosition The destination position for a move, or the target position for an attack
	 * @param Damage The amount of damage dealt in an attack (0 for non-attack actions)
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Log")
	void AddFormattedMoveToLog(bool bIsPlayerUnit, const FString& UnitType, const FString& ActionType,
		const FVector2D& FromPosition, const FVector2D& ToPosition, int32 Damage = 0);

	// Get the formatted move history as a string
	UFUNCTION(BlueprintCallable, Category = "Game Log")
	FString GetFormattedGameLog() const;

	// Called when a unit dies to update game state
	void NotifyUnitDeath(AUnit* DeadUnit);

	// Resets the game to its initial state
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResetGame();

	// Handles the easy mode selection 
	UFUNCTION(BlueprintCallable, Category = "Game")
	void OnEasyModeSelected();

	// Handles the hard mode selection 
	UFUNCTION(BlueprintCallable, Category = "Game")
	void OnHardModeSelected();

protected:

	// Initializes all players at the start of the game 
	void InitializePlayers();

	// Notifies the current player that it's their turn 
	void NotifyCurrentPlayerTurn();

};