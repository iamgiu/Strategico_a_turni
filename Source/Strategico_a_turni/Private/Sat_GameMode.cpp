// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_GameMode.h"
#include "GridManager.h"
#include "Unit.h"
#include "SaT_PlayerInterface.h"
#include "SaT_HumanPlayer.h"
#include "SaT_RandomPlayer.h"
#include "SaT_GameInstance.h"
#include "Kismet/GameplayStatics.h"

ASaT_GameMode::ASaT_GameMode()
{
    // Default initialization
    CurrentPlayer = 0;
    bIsGameOver = false;
    CurrentPlayerType = EPlayerType::Human;
}

void ASaT_GameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("===== GAMEMODE BEGIN PLAY ====="));

    // Instantiate GridManager if not already present
    if (!Gmanager && GameManagerClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Creating new GridManager instance..."));
        Gmanager = GetWorld()->SpawnActor<AGridManager>(GameManagerClass);

        if (Gmanager)
        {
            UE_LOG(LogTemp, Warning, TEXT("GridManager successfully created!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create GridManager! GameManagerClass valid: %s"),
                GameManagerClass ? TEXT("YES") : TEXT("NO"));
        }
    }
    else if (Gmanager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GridManager already exists."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create GridManager: GameManagerClass is not set!"));
    }

    // As a fallback, try to find an existing GridManager in the world
    if (!Gmanager)
    {
        UE_LOG(LogTemp, Warning, TEXT("Searching for existing GridManager in the world..."));
        TArray<AActor*> FoundGrids;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);

        if (FoundGrids.Num() > 0)
        {
            Gmanager = Cast<AGridManager>(FoundGrids[0]);
            if (Gmanager)
            {
                UE_LOG(LogTemp, Warning, TEXT("Found existing GridManager in the world!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No GridManager found in the world!"));
        }
    }

    // Initialize players
    InitializePlayers();

    // Get game instance and set game phase to SETUP
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        GameInstance->SetGamePhase(EGamePhase::SETUP);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get GameInstance!"));
    }

    // Auto start game after a short delay to ensure everything is initialized
    if (Gmanager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GridManager is valid, scheduling game start..."));
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, this, &ASaT_GameMode::StartGame, 2.0f, false);
        UE_LOG(LogTemp, Warning, TEXT("Game will auto-start in 2 seconds..."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot schedule game start: GridManager is NULL!"));
    }
}

void ASaT_GameMode::InitializePlayers()
{
    // Find all players implementing the ISaT_PlayerInterface
    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USaT_PlayerInterface::StaticClass(), FoundPlayers);

    // Clear any existing players
    Players.Empty();

    // Track if we've already added a human and AI player
    bool bHumanPlayerAdded = false;
    bool bAIPlayerAdded = false;

    // Add one human player and one AI player
    for (AActor* Actor : FoundPlayers)
    {
        ISaT_PlayerInterface* PlayerInterface = Cast<ISaT_PlayerInterface>(Actor);
        if (PlayerInterface)
        {
            if (Cast<ASaT_HumanPlayer>(Actor) && !bHumanPlayerAdded)
            {
                // Add human player
                Players.Add(PlayerInterface);
                PlayerInterface->PlayerNumber = 0;
                PlayerInterface->Color = EPieceColor::BLUE;
                bHumanPlayerAdded = true;
                UE_LOG(LogTemp, Warning, TEXT("Human Player added to game (index 0)"));
            }
            else if (Cast<ASaT_RandomPlayer>(Actor) && !bAIPlayerAdded)
            {
                // Add AI player
                Players.Add(PlayerInterface);
                PlayerInterface->PlayerNumber = 1;
                PlayerInterface->Color = EPieceColor::RED;
                bAIPlayerAdded = true;
                UE_LOG(LogTemp, Warning, TEXT("AI Player added to game (index 1)"));
            }

            // Set the GameInstance reference
            USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
            if (GameInstance)
            {
                PlayerInterface->GameInstance = GameInstance;
            }
        }
    }

    if (Players.Num() >= MIN_NUMBER_SPAWN_PLAYERS)
    {
        UE_LOG(LogTemp, Warning, TEXT("Successfully initialized %d players (1 Human, 1 AI)"), Players.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Not enough players found! Need at least %d players, found %d."),
            MIN_NUMBER_SPAWN_PLAYERS, Players.Num());
    }
}

void ASaT_GameMode::StartGame()
{
    UE_LOG(LogTemp, Warning, TEXT("===== START GAME FUNCTION CALLED ====="));

    // Double-check if GridManager exists
    if (!Gmanager)
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager is NULL, trying to find it..."));

        // Last attempt to find the GridManager
        TArray<AActor*> FoundGrids;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);

        if (FoundGrids.Num() > 0)
        {
            Gmanager = Cast<AGridManager>(FoundGrids[0]);
            UE_LOG(LogTemp, Warning, TEXT("Found GridManager in last attempt!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot start game: GridManager not found. Aborting game start."));
            return;
        }
    }

    if (Players.Num() < MIN_NUMBER_SPAWN_PLAYERS)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot start game: Not enough players. Found: %d, Required: %d"),
            Players.Num(), MIN_NUMBER_SPAWN_PLAYERS);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("GridManager and Players are valid. Starting game..."));

    // Flip a coin to decide who goes first
    FlipCoinToDecideFirstPlayer();

    // Set game to playing phase
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        GameInstance->SetGamePhase(EGamePhase::PLAYING);
        UE_LOG(LogTemp, Warning, TEXT("Game phase set to PLAYING"));
    }

    // Start first turn
    StartFirstTurn();

    UE_LOG(LogTemp, Warning, TEXT("===== GAME SUCCESSFULLY STARTED ====="));
}

void ASaT_GameMode::FlipCoinToDecideFirstPlayer()
{
    UE_LOG(LogTemp, Warning, TEXT("===== FLIPPING COIN TO DECIDE FIRST PLAYER ====="));

    // Simple random 50/50 chance to determine who goes first
    bool bFirstPlayerWinsFlip = FMath::RandBool();

    // Set the current player based on the coin flip
    CurrentPlayer = bFirstPlayerWinsFlip ? 0 : 1;

    // Make sure we have valid players
    if (Players.Num() < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("Not enough players for coin flip! Found %d, need at least 2"), Players.Num());
        return;
    }

    // Determine player type and announce the result
    if (Players.IsValidIndex(CurrentPlayer))
    {
        AActor* PlayerActor = Cast<AActor>(Players[CurrentPlayer]);
        if (Cast<ASaT_HumanPlayer>(PlayerActor))
        {
            CurrentPlayerType = EPlayerType::Human;
            UE_LOG(LogTemp, Warning, TEXT("*** COIN FLIP RESULT: HUMAN player goes first (Player index: %d) ***"), CurrentPlayer);
        }
        else
        {
            CurrentPlayerType = EPlayerType::AI;
            UE_LOG(LogTemp, Warning, TEXT("*** COIN FLIP RESULT: AI player goes first (Player index: %d) ***"), CurrentPlayer);
        }

        // Set the current player as active
        for (int32 i = 0; i < Players.Num(); i++)
        {
            Players[i]->IsMyTurn = (i == CurrentPlayer);
            UE_LOG(LogTemp, Warning, TEXT("Player %d IsMyTurn = %s"), i, Players[i]->IsMyTurn ? TEXT("TRUE") : TEXT("FALSE"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid CurrentPlayer index: %d (Players count: %d)"), CurrentPlayer, Players.Num());
    }
}

void ASaT_GameMode::StartFirstTurn()
{
    // Ensure we have valid players
    if (Players.Num() >= MIN_NUMBER_SPAWN_PLAYERS && Players.IsValidIndex(CurrentPlayer))
    {
        // Set the current player as active
        for (int32 i = 0; i < Players.Num(); i++)
        {
            Players[i]->IsMyTurn = (i == CurrentPlayer);
        }

        // Notify current player it's their turn
        Players[CurrentPlayer]->OnTurn();

        UE_LOG(LogTemp, Display, TEXT("First turn started for Player %d"), CurrentPlayer + 1);
    }
}

void ASaT_GameMode::TurnNextPlayer()
{
    // Make sure current player is no longer in their turn
    if (Players.IsValidIndex(CurrentPlayer))
    {
        Players[CurrentPlayer]->IsMyTurn = false;
    }

    // Get the GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("TurnNextPlayer: GameInstance not found."));
        return;
    }

    // Use GameInstance to switch turns
    GameInstance->SwitchTurn();

    // Update current player based on GameInstance state
    CurrentPlayer = GameInstance->bIsPlayerTurn ? 0 : 1;

    // Update player type
    AActor* PlayerActor = Cast<AActor>(Players[CurrentPlayer]);
    if (Cast<ASaT_HumanPlayer>(PlayerActor))
    {
        CurrentPlayerType = EPlayerType::Human;
    }
    else
    {
        CurrentPlayerType = EPlayerType::AI;
    }

    // Set new player's turn
    Players[CurrentPlayer]->IsMyTurn = true;

    // Notify the player
    Players[CurrentPlayer]->OnTurn();

    UE_LOG(LogTemp, Display, TEXT("Turn switched to Player %d"), CurrentPlayer + 1);
}

void ASaT_GameMode::EndTurn()
{
    // Check if game is over before switching turns
    if (CheckGameOver())
    {
        UE_LOG(LogTemp, Display, TEXT("Game is over!"));
        return;
    }

    // Switch to next player
    TurnNextPlayer();
}

bool ASaT_GameMode::CheckGameOver()
{
    if (bIsGameOver)
    {
        return true;
    }

    // Check if any player has lost all units
    TArray<int32> PlayerUnitCounts;
    PlayerUnitCounts.Init(0, Players.Num());

    // Count units for each player
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit)
        {
            // Count units by player ownership
            int32 PlayerIndex = Unit->bIsPlayerUnit ? 0 : 1; // Assuming player units are human, non-player are AI
            PlayerUnitCounts[PlayerIndex]++;
        }
    }

    // Check if any player has no units left
    for (int32 i = 0; i < PlayerUnitCounts.Num(); i++)
    {
        if (PlayerUnitCounts[i] == 0)
        {
            // This player has lost
            bIsGameOver = true;

            // Determine winner and loser
            int32 WinnerIndex = (i + 1) % Players.Num();

            // Notify players of result
            Players[WinnerIndex]->OnWin();
            Players[i]->OnLose();

            // Update game phase in GameInstance
            USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
            if (GameInstance)
            {
                GameInstance->SetGamePhase(EGamePhase::GAMEOVER);
            }

            UE_LOG(LogTemp, Display, TEXT("Game over! Player %d wins, Player %d loses"), WinnerIndex + 1, i + 1);
            return true;
        }
    }

    return false;
}