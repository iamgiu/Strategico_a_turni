// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameMode.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "SaT_PlayerInterface.h"

ASaT_GameMode::ASaT_GameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsGameOver = false;
	CurrentPlayer = 0;
}

void ASaT_GameMode::BeginPlay()
{
	Super::BeginPlay();
	StartGame();
}

void ASaT_GameMode::StartGame()
{
	InitializePlayers();
	FlipCoinToDecideFirstPlayer();

	UE_LOG(LogTemp, Warning, TEXT("SaT_GameMode: Game started, Current player index: %d"), CurrentPlayer);
}

void ASaT_GameMode::InitializePlayers()
{
	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsWithInterface(this, USaT_PlayerInterface::StaticClass(), FoundPlayers);

	for (AActor* Actor : FoundPlayers)
	{
		ISaT_PlayerInterface* Player = Cast<ISaT_PlayerInterface>(Actor);
		if (Player)
		{
			Players.Add(Player);
		}
	}
}

void ASaT_GameMode::StartFirstTurn()
{
	if (Players.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("SaT_GameMode: Nessun giocatore disponibile per iniziare il turno!"));
		return;
	}

	// Notifica al giocatore corrente che è il suo turno
	UE_LOG(LogTemp, Warning, TEXT("SaT_GameMode: Inizio il primo turno per il giocatore %d"), CurrentPlayer);
	Players[CurrentPlayer]->OnTurn();
}

// Modifica il tuo metodo FlipCoinToDecideFirstPlayer per chiamare StartFirstTurn
void ASaT_GameMode::FlipCoinToDecideFirstPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("SaT_GameMode: FlipCoinToDecideFirstPlayer called"));

	if (Players.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("SaT_GameMode: Not enough players to flip coin! Only %d players found."), Players.Num());
		return;
	}

	// Utilizziamo FMath::RandBool() per decidere casualmente chi inizia
	bool bHumanGoesFirst = FMath::RandBool();

	if (bHumanGoesFirst)
	{
		CurrentPlayer = 0; // Indice del giocatore umano
		CurrentPlayerType = EPlayerType::Human;
		UE_LOG(LogTemp, Warning, TEXT("SaT_GameMode: Coin flip result - Human player goes first"));
	}
	else
	{
		CurrentPlayer = 1; // Indice dell'IA
		CurrentPlayerType = EPlayerType::AI;
		UE_LOG(LogTemp, Warning, TEXT("SaT_GameMode: Coin flip result - AI player goes first"));
	}

	// Inizia il primo turno
	StartFirstTurn();
}

void ASaT_GameMode::TurnNextPlayer()
{
	if (Players.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("SaT_GameMode: Cannot turn to next player, no players available!"));
		return;
	}

	CurrentPlayer = (CurrentPlayer + 1) % Players.Num();

	// Alterna tra Human e AI
	CurrentPlayerType = (CurrentPlayerType == EPlayerType::Human) ? EPlayerType::AI : EPlayerType::Human;

	UE_LOG(LogTemp, Warning, TEXT("SaT_GameMode: Turned to next player, Current player type: %s"),
		(CurrentPlayerType == EPlayerType::Human) ? TEXT("Human") : TEXT("AI"));
}

void ASaT_GameMode::EndTurn()
{
	if (CheckGameOver())
	{
		bIsGameOver = true;
	}
	else
	{
		TurnNextPlayer();

		// Notifica al prossimo giocatore che è il suo turno
		if (Players.Num() > CurrentPlayer)
		{
			Players[CurrentPlayer]->OnTurn();
		}
	}
}

bool ASaT_GameMode::CheckGameOver()
{
	//for (ISaT_PlayerInterface* Player : Players)
	//{
		//if (Player && Player->OnLose())
		//{
			//return true;
		//}
	//}

	return false;
}