#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SaT_Enums.h"
#include "SaT_GameMode.generated.h"

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

protected:

	void InitializePlayers();

	void NotifyCurrentPlayerTurn();

};
