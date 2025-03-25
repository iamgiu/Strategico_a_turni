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
	static constexpr int8 MIN_NUMBER_SPAWN_PLAYERS = 2;

	ASaT_GameMode();

	virtual void BeginPlay() override;

	/* GAME MANAGEMENT */
	void StartGame();
	void StartFirstTurn();
	void FlipCoinToDecideFirstPlayer();
	void TurnNextPlayer();
	void EndTurn();
	bool CheckGameOver();

	void UpdateGameHUD();

	/* ATTRIBUTES */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGridManager> GameManagerClass;

	UPROPERTY(VisibleAnywhere)
	AGridManager* Gmanager;

	TArray<ISaT_PlayerInterface*> Players;
	int32 CurrentPlayer;
	bool bIsGameOver;

    UPROPERTY(BlueprintReadOnly, Category = "Game")
    EPlayerType CurrentPlayerType;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    AUnit* SomeUnitVariable;

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
    bool IsPlayerTurn;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    int32 CurrentTurnNumber;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> MainGameHUDClass;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString PlayerSniperHPFormatted;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString PlayerBrawlerHPFormatted;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString AISniperHPFormatted;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString AIBrawlerHPFormatted;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString TurnText;

    /* Getters for UI Binding */
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

    UFUNCTION(BlueprintCallable, Category = "UI")
    FString GetSelectedUnitInfoText() const;

    AUnit* CurrentlySelectedUnit;

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SetSelectedUnit(AUnit* Unit);

    // Maximum number of log entries to keep
    UPROPERTY(EditDefaultsOnly, Category = "Game Log")
    int32 MaxGameLogEntries = 50;

    // Game log array for storing entries
    UPROPERTY(BlueprintReadOnly, Category = "Game Log")
    TArray<FString> GameLog;

    UPROPERTY(BlueprintReadOnly, Category = "Game Log")
    FString FormattedEntry;

    UPROPERTY(BlueprintReadOnly, Category = "Game Log")
    TArray<FString> RawMoveHistory;

    /**
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

    void NotifyUnitDeath(AUnit* DeadUnit);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> AIThinkingWidgetClass;

    UPROPERTY()
    UUserWidget* AIThinkingWidget;

    // Method to show/hide the widget
    void ShowAIThinkingWidget(bool bShow);

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    FString CoinflipResult;

    UPROPERTY()
    UUserWidget* CoinFlipResultWidget;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> CoinFlipResultWidgetClass;

    void HideCoinFlipWidget();

    void ShowCoinFlipResultWidget(bool bShow);

protected:

	void InitializePlayers();

	void NotifyCurrentPlayerTurn();

};
