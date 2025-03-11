// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameMode.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "SaT_PlayerInterface.h"
#include "GridManager.h"

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

void ASaT_GameMode::FlipCoinToDecideFirstPlayer()
{
	CurrentPlayer = FMath::RandBool() ? 0 : 1;
}

void ASaT_GameMode::TurnNextPlayer()
{
	CurrentPlayer = (CurrentPlayer + 1) % Players.Num();
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