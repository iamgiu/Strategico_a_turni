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

protected:

	void InitializePlayers();

	void NotifyCurrentPlayerTurn();

};
