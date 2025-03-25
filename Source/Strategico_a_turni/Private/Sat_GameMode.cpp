// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_GameMode.h"
#include "GridManager.h"
#include "Unit.h"
#include "Sniper.h"
#include "Brawler.h"
#include "SaT_PlayerInterface.h"
#include "SaT_HumanPlayer.h"
#include "SaT_RandomPlayer.h"
#include "SaT_GameInstance.h"
#include "Blueprint/UserWidget.h"
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

        // Set game to SETUP phase first, not directly to PLAYING
        GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    }

    if (MainGameHUDClass) // You need to set this reference in your Blueprint
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            UUserWidget* GameHUD = CreateWidget<UUserWidget>(PC, MainGameHUDClass);
            if (GameHUD)
            {
                GameHUD->AddToViewport();
            }
        }
    }
}

void ASaT_GameMode::InitializePlayers()
{
    // Find all players implementing the ISaT_PlayerInterface
    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USaT_PlayerInterface::StaticClass(), FoundPlayers);

    // Clear any existing players
    Players.Empty();

    // Find and store human and AI players separately
    ISaT_PlayerInterface* HumanPlayerInterface = nullptr;
    ISaT_PlayerInterface* AIPlayerInterface = nullptr;

    for (AActor* Actor : FoundPlayers)
    {
        ISaT_PlayerInterface* PlayerInterface = Cast<ISaT_PlayerInterface>(Actor);
        if (PlayerInterface)
        {
            if (Cast<ASaT_HumanPlayer>(Actor))
            {
                HumanPlayerInterface = PlayerInterface;
                UE_LOG(LogTemp, Warning, TEXT("Found Human Player: %s"), *Actor->GetName());
            }
            else if (Cast<ASaT_RandomPlayer>(Actor))
            {
                AIPlayerInterface = PlayerInterface;
                UE_LOG(LogTemp, Warning, TEXT("Found AI Player: %s"), *Actor->GetName());
            }
        }
    }

    // Add players to array in the CORRECT ORDER
    // IMPORTANT: Human MUST be index 0, AI MUST be index 1
    if (HumanPlayerInterface)
    {
        Players.Add(HumanPlayerInterface);
        HumanPlayerInterface->PlayerNumber = 0;
        HumanPlayerInterface->Color = EPieceColor::BLUE;
        UE_LOG(LogTemp, Warning, TEXT("Human Player added to game (index 0)"));
    }

    if (AIPlayerInterface)
    {
        Players.Add(AIPlayerInterface);
        AIPlayerInterface->PlayerNumber = 1;
        AIPlayerInterface->Color = EPieceColor::RED;
        UE_LOG(LogTemp, Warning, TEXT("AI Player added to game (index 1)"));
    }

    // Set the GameInstance reference for all players
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        for (auto& Player : Players)
        {
            Player->GameInstance = GameInstance;
        }
    }

    // Verify our player array is correctly initialized
    UE_LOG(LogTemp, Warning, TEXT("Successfully initialized %d players (1 Human, 1 AI)"), Players.Num());
    for (int32 i = 0; i < Players.Num(); i++)
    {
        bool bIsHuman = Cast<ASaT_HumanPlayer>(Players[i]->_getUObject()) != nullptr;
        UE_LOG(LogTemp, Warning, TEXT("Player[%d]: Type=%s, PlayerNumber=%d"),
            i,
            bIsHuman ? TEXT("Human") : TEXT("AI"),
            Players[i]->PlayerNumber);
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

    if (GameInstance) {
        GameInstance->SetGamePhase(EGamePhase::SETUP);
        UE_LOG(LogTemp, Warning, TEXT("Game phase set to SETUP"));
    }

    // Start first turn
    StartFirstTurn();

    UpdateGameHUD();

    UE_LOG(LogTemp, Warning, TEXT("===== GAME SUCCESSFULLY STARTED ====="));

}

void ASaT_GameMode::FlipCoinToDecideFirstPlayer()
{
    UE_LOG(LogTemp, Warning, TEXT("===== FLIPPING COIN TO DECIDE FIRST PLAYER ====="));

    ShowCoinFlipResultWidget(true);

    // Simple random 50/50 chance to determine who goes first
    bool bHumanWinsFlip = FMath::RandBool();

    // Make sure we have valid players
    if (Players.Num() < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("Not enough players for coin flip! Found %d, need at least 2"), Players.Num());
        return;
    }

    // Get the GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in FlipCoinToDecideFirstPlayer!"));
        return;
    }

    // CRITICAL FIX: Update GameInstance's turn state and call its built-in coin toss function
    GameInstance->bPlayerStartsFirst = bHumanWinsFlip;
    GameInstance->bIsPlayerTurn = bHumanWinsFlip;

    // Sync GameMode's state with GameInstance
    CurrentPlayer = bHumanWinsFlip ? 0 : 1; // 0 = Human, 1 = AI
    CurrentPlayerType = bHumanWinsFlip ? EPlayerType::Human : EPlayerType::AI;

    // Log the result more clearly
    UE_LOG(LogTemp, Warning, TEXT("*** COIN FLIP RESULT: %s player goes first (Player index: %d) ***"),
        bHumanWinsFlip ? TEXT("HUMAN") : TEXT("AI"),
        CurrentPlayer);

    // Log the GameInstance state after coin flip
    UE_LOG(LogTemp, Warning, TEXT("GameInstance bPlayerStartsFirst = %s, bIsPlayerTurn = %s, GameMode CurrentPlayer = %d"),
        GameInstance->bPlayerStartsFirst ? TEXT("TRUE (Human)") : TEXT("FALSE (AI)"),
        GameInstance->bIsPlayerTurn ? TEXT("TRUE (Human)") : TEXT("FALSE (AI)"),
        CurrentPlayer);

    CoinflipResult = bHumanWinsFlip
        ? FString::Printf(TEXT("YOU go first"))
        : FString::Printf(TEXT("AI goes first"));

    FTimerHandle HideWidgetTimerHandle;
    GetWorldTimerManager().SetTimer(
        HideWidgetTimerHandle,   // Timer handle
        [this]()                 // Lambda function for hiding
        {
            // Ensure we're on the game thread
            if (IsValid(this))
            {
                ShowCoinFlipResultWidget(false);
            }
        },
        2.0f,                    // Delay in seconds
        false                    // Do not loop
    );

}

void ASaT_GameMode::StartFirstTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("===== STARTING FIRST TURN ====="));

    // Get the game instance to verify turn state
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in StartFirstTurn!"));
        return;
    }

    // CRITICAL FIX: Ensure GameMode's CurrentPlayer is synchronized with GameInstance
    bool bHumanGoesFirst = GameInstance->bPlayerStartsFirst;
    bool bIsHumanTurn = GameInstance->bIsPlayerTurn;

    // Verify these values are consistent
    if (bHumanGoesFirst != bIsHumanTurn)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance turn state inconsistency! bPlayerStartsFirst=%s but bIsPlayerTurn=%s"),
            bHumanGoesFirst ? TEXT("TRUE") : TEXT("FALSE"),
            bIsHumanTurn ? TEXT("TRUE") : TEXT("FALSE"));

        // Correct the inconsistency by making bIsPlayerTurn match bPlayerStartsFirst
        GameInstance->bIsPlayerTurn = GameInstance->bPlayerStartsFirst;
        bIsHumanTurn = GameInstance->bIsPlayerTurn;
    }

    // Update GameMode state to match GameInstance
    CurrentPlayer = bIsHumanTurn ? 0 : 1;
    CurrentPlayerType = bIsHumanTurn ? EPlayerType::Human : EPlayerType::AI;

    UE_LOG(LogTemp, Warning, TEXT("First turn starting for %s player (index %d)"),
        CurrentPlayerType == EPlayerType::Human ? TEXT("Human") : TEXT("AI"),
        CurrentPlayer);

    // Update the player IsMyTurn flags based on CurrentPlayer
    for (int32 i = 0; i < Players.Num(); i++)
    {
        // Only the current player should have IsMyTurn = true
        Players[i]->IsMyTurn = (i == CurrentPlayer);

        UE_LOG(LogTemp, Warning, TEXT("StartFirstTurn: Player %d IsMyTurn set to %s"),
            i, Players[i]->IsMyTurn ? TEXT("TRUE") : TEXT("FALSE"));
    }

    // Clear any pending timers
    GetWorldTimerManager().ClearAllTimersForObject(this);

    // Add a short delay before notifying the current player
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ASaT_GameMode::NotifyCurrentPlayerTurn, 0.5f, false);
}

void ASaT_GameMode::EndTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("===== END TURN CALLED ====="));

    // Check if game is over before switching turns
    if (CheckGameOver())
    {
        UE_LOG(LogTemp, Display, TEXT("Game is over! Not switching turns."));
        return;
    }

    // Get the current game phase from GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get GameInstance in EndTurn!"));
        return;
    }

    // First, make sure current player is no longer in their turn
    if (Players.IsValidIndex(CurrentPlayer))
    {
        Players[CurrentPlayer]->IsMyTurn = false;
        UE_LOG(LogTemp, Warning, TEXT("Setting Player %d IsMyTurn to FALSE"), CurrentPlayer);
    }

    // Let GameInstance handle turn switching
    UE_LOG(LogTemp, Warning, TEXT("Telling GameInstance to switch turns"));
    GameInstance->SwitchTurn();

    // After switching, update our CurrentPlayer based on the new GameInstance state
    bool bIsHumanTurn = GameInstance->bIsPlayerTurn;
    CurrentPlayer = bIsHumanTurn ? 0 : 1;
    CurrentPlayerType = bIsHumanTurn ? EPlayerType::Human : EPlayerType::AI;

    UE_LOG(LogTemp, Warning, TEXT("After turn switch - Current turn is now: %s (player index %d)"),
        bIsHumanTurn ? TEXT("HUMAN") : TEXT("AI"),
        CurrentPlayer);

    UpdateGameHUD();

    // Schedule notification for the next player
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ASaT_GameMode::NotifyCurrentPlayerTurn, 0.1f, false);
}

bool ASaT_GameMode::CheckGameOver()
{
    // Get the current game phase from GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get GameInstance in CheckGameOver!"));
        return false;
    }

    // Only check for game over during the playing phase
    if (GameInstance->GetGamePhase() == EGamePhase::PLAYING)
    {
        // Check if any player has lost all units
        int32 HumanUnitCount = 0;
        int32 AIUnitCount = 0;

        // Count units for each player
        TArray<AActor*> AllUnits;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

        for (AActor* UnitActor : AllUnits)
        {
            AUnit* Unit = Cast<AUnit>(UnitActor);
            if (Unit && Unit->IsAlive())
            {
                if (Unit->bIsPlayerUnit)
                    HumanUnitCount++;
                else
                    AIUnitCount++;
            }
        }

        // Check if either player has no units left
        if (HumanUnitCount == 0 || AIUnitCount == 0)
        {
            // Set game phase to GAMEOVER
            GameInstance->SetGamePhase(EGamePhase::GAMEOVER);

            // Determine the winner
            bool bHumanWins = (AIUnitCount == 0);

            // Notify players of the result
            for (auto PlayerInterface : Players)
            {
                if (PlayerInterface)
                {
                    if (bHumanWins && PlayerInterface->PlayerNumber == 0)
                        PlayerInterface->OnWin();
                    else if (!bHumanWins && PlayerInterface->PlayerNumber == 1)
                        PlayerInterface->OnWin();
                    else
                        PlayerInterface->OnLose();
                }
            }

            return true;
        }
    }

    return false;
}

void ASaT_GameMode::TurnNextPlayer()
{
    UE_LOG(LogTemp, Warning, TEXT("===== TURNING NEXT PLAYER ====="));

    // This method shouldn't be called anymore - redirect to EndTurn
    UE_LOG(LogTemp, Warning, TEXT("TurnNextPlayer called - redirecting to EndTurn for consistency"));
    EndTurn();
}

void ASaT_GameMode::NotifyCurrentPlayerTurn()
{
    // DEBUGGING
    UE_LOG(LogTemp, Warning, TEXT("==== DEBUG: NotifyCurrentPlayerTurn ENTRY ===="));
    UE_LOG(LogTemp, Warning, TEXT("CurrentPlayer=%d, Players.Num()=%d"), CurrentPlayer, Players.Num());

    // Print info about each player
    for (int32 i = 0; i < Players.Num(); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player[%d]: IsMyTurn=%s, Type=%s"),
            i,
            Players[i]->IsMyTurn ? TEXT("TRUE") : TEXT("FALSE"),
            Cast<ASaT_HumanPlayer>(Players[i]->_getUObject()) ? TEXT("Human") : TEXT("AI"));
    }

    // Get the GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in NotifyCurrentPlayerTurn!"));
        return;
    }

    // Reset movement flags for units of the current player
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    bool bIsHumanTurn = GameInstance->bIsPlayerTurn;

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit && Unit->bIsPlayerUnit == bIsHumanTurn)
        {
            // Reset movement flag at the start of the player's turn
            Unit->bHasMovedThisTurn = false;
            UE_LOG(LogTemp, Warning, TEXT("Reset movement flag for unit: %s"), *Unit->GetName());
        }
    }

    // CRITICAL FIX: Always ensure CurrentPlayer matches GameInstance state
    int32 ExpectedPlayerIndex = bIsHumanTurn ? 0 : 1;

    // If there's a mismatch, correct it
    if (CurrentPlayer != ExpectedPlayerIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("Correcting player index mismatch! CurrentPlayer=%d but GameInstance says %s turn"),
            CurrentPlayer, bIsHumanTurn ? TEXT("Human") : TEXT("AI"));

        CurrentPlayer = ExpectedPlayerIndex;
        CurrentPlayerType = bIsHumanTurn ? EPlayerType::Human : EPlayerType::AI;
    }

    // Safety check on array bounds
    if (!Players.IsValidIndex(CurrentPlayer))
    {
        UE_LOG(LogTemp, Error, TEXT("NotifyCurrentPlayerTurn: Invalid player index %d"), CurrentPlayer);
        return;
    }

    // Update all player turn flags to be consistent
    for (int32 i = 0; i < Players.Num(); i++)
    {
        Players[i]->IsMyTurn = (i == CurrentPlayer);
    }

    ISaT_PlayerInterface* PlayerToNotify = Players[CurrentPlayer];
    if (PlayerToNotify)
    {
        UE_LOG(LogTemp, Warning, TEXT("Directly notifying player at index %d - IsHuman: %s"),
            CurrentPlayer, bIsHumanTurn ? TEXT("TRUE") : TEXT("FALSE"));

        // Double-check one more time - is this really the right player?
        bool bIsPlayerHuman = (Cast<ASaT_HumanPlayer>(PlayerToNotify->_getUObject()) != nullptr);

        if (bIsPlayerHuman == bIsHumanTurn)
        {
            // The player type matches the turn state - go ahead and notify
            PlayerToNotify->OnTurn();
        }
        else
        {
            // We have a mismatch! Log it and abort this notification
            UE_LOG(LogTemp, Error, TEXT("TURN STATE MISMATCH! Trying to notify %s player when it's %s turn!"),
                bIsPlayerHuman ? TEXT("Human") : TEXT("AI"),
                bIsHumanTurn ? TEXT("Human") : TEXT("AI"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerToNotify is null!"));
    }

    UpdateGameHUD();
}

void ASaT_GameMode::NotifyUnitDeath(AUnit* DeadUnit)
{
    if (!DeadUnit)
        return;

    // Log the death
    UE_LOG(LogTemp, Warning, TEXT("Unit death: %s at position (%d,%d)"),
        *DeadUnit->GetName(), DeadUnit->GridX, DeadUnit->GridY);

    // Check if this affects game over conditions
    CheckGameOver();

    // Update the UI
    UpdateGameHUD();
}

void ASaT_GameMode::UpdateGameHUD()
{
    // Find all player units and get their status
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    // Initialize default values
    PlayerSniperHP = 0;
    PlayerBrawlerHP = 0;
    AISniperHP = 0;
    AIBrawlerHP = 0;

    PlayerSniperPos = TEXT("(--,--)");
    PlayerBrawlerPos = TEXT("(--,--)");
    AISniperPos = TEXT("(--,--)");
    AIBrawlerPos = TEXT("(--,--)");

    // Update unit information
    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit && Unit->IsAlive())
        {
            if (Unit->bIsPlayerUnit)
            {
                // Player units
                if (Cast<ASniper>(Unit))
                {
                    // Update player sniper info
                    PlayerSniperHP = Unit->Hp;
                    PlayerSniperPos = FString::Printf(TEXT("(%d,%d)"), Unit->GridX, Unit->GridY);
                }
                else if (Cast<ABrawler>(Unit))
                {
                    // Update player brawler info
                    PlayerBrawlerHP = Unit->Hp;
                    PlayerBrawlerPos = FString::Printf(TEXT("(%d,%d)"), Unit->GridX, Unit->GridY);
                }
            }
            else
            {
                // AI units
                if (Cast<ASniper>(Unit))
                {
                    // Update AI sniper info
                    AISniperHP = Unit->Hp;
                    AISniperPos = FString::Printf(TEXT("(%d,%d)"), Unit->GridX, Unit->GridY);
                }
                else if (Cast<ABrawler>(Unit))
                {
                    // Update AI brawler info
                    AIBrawlerHP = Unit->Hp;
                    AIBrawlerPos = FString::Printf(TEXT("(%d,%d)"), Unit->GridX, Unit->GridY);
                }
            }
        }
    }

    // Format HP values for display
    PlayerSniperHPFormatted = FString::Printf(TEXT("HP: %d/20"), PlayerSniperHP);
    PlayerBrawlerHPFormatted = FString::Printf(TEXT("HP: %d/40"), PlayerBrawlerHP);
    AISniperHPFormatted = FString::Printf(TEXT("HP: %d/20"), AISniperHP);
    AIBrawlerHPFormatted = FString::Printf(TEXT("HP: %d/40"), AIBrawlerHP);

    // Update turn info
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        // Set who's turn it is
        IsPlayerTurn = GameInstance->bIsPlayerTurn;
        // Get the current turn number
        CurrentTurnNumber = GameInstance->CurrentTurnNumber;

        // Update turn text
        if (GameInstance->GetGamePhase() == EGamePhase::SETUP)
        {
            TurnText = IsPlayerTurn
                ? FString::Printf(TEXT("SETUP PHASE: PLAYER TURN %d"), CurrentTurnNumber)
                : FString::Printf(TEXT("SETUP PHASE: AI TURN %d"), CurrentTurnNumber);
        }
        else if (GameInstance->GetGamePhase() == EGamePhase::PLAYING)
        {
            TurnText = IsPlayerTurn
                ? FString::Printf(TEXT("PLAYING PHASE: PLAYER TURN %d"), CurrentTurnNumber)
                : FString::Printf(TEXT("PLAYING PHASE: AI TURN %d"), CurrentTurnNumber);
        }
        else if (GameInstance->GetGamePhase() == EGamePhase::GAMEOVER)
        {
            // Count units to determine winner
            int32 HumanUnits = 0;
            int32 AIUnits = 0;

            for (AActor* UnitActor : AllUnits)
            {
                AUnit* Unit = Cast<AUnit>(UnitActor);
                if (Unit && Unit->IsAlive())
                {
                    if (Unit->bIsPlayerUnit)
                        HumanUnits++;
                    else
                        AIUnits++;
                }
            }

            if (HumanUnits > 0 && AIUnits == 0)
                TurnText = TEXT("GAME OVER - PLAYER WINS");
            else if (HumanUnits == 0 && AIUnits > 0)
                TurnText = TEXT("GAME OVER - AI WINS");
            else
                TurnText = TEXT("GAME OVER");
        }
    }
    else
    {
        TurnText = TEXT("Game Initializing...");
    }

    // Debug log to verify updates
    UE_LOG(LogTemp, Display, TEXT("UI Updated - Turn: %s, PlayerSniper: %s, PlayerBrawler: %s"),
        *TurnText, *PlayerSniperHPFormatted, *PlayerBrawlerHPFormatted);
}

void ASaT_GameMode::SetSelectedUnit(AUnit* Unit)
{

    CurrentlySelectedUnit = Unit;
}

FString ASaT_GameMode::GetSelectedUnitInfoText() const
{
    if (CurrentlySelectedUnit && CurrentlySelectedUnit->IsAlive())
    {
        return FString::Printf(TEXT("%s \nMovement Range: %d  \nAttack Range: %d | MinDamage: %d | MaxDamage: %d"),
            *CurrentlySelectedUnit->UnitTypeDisplayName,
            CurrentlySelectedUnit->Movement,
            CurrentlySelectedUnit->RangeAttack,
            CurrentlySelectedUnit->MinDamage,
            CurrentlySelectedUnit->MaxDamage);
    }

    return TEXT("No Unit Selected");
}


void ASaT_GameMode::AddFormattedMoveToLog(bool bIsPlayerUnit, const FString& UnitType, const FString& ActionType,
    const FVector2D& FromPosition, const FVector2D& ToPosition, int32 Damage)
{
    // Format the player identifier
    FString PlayerIdentifier = bIsPlayerUnit ? TEXT("PLAYER") : TEXT("AI");

    // Format the move entry based on action type
    FString MoveEntry;
    if (ActionType == TEXT("Move"))
    {
        // Format: PLAYER: Moved Sniper from (4,6) to (8,9)
        MoveEntry = FString::Printf(TEXT("%s: Moved %s from (%d,%d) to (%d,%d)"),
            *PlayerIdentifier,
            *UnitType,
            static_cast<int32>(FromPosition.X),
            static_cast<int32>(FromPosition.Y),
            static_cast<int32>(ToPosition.X),
            static_cast<int32>(ToPosition.Y));
    }
    else if (ActionType == TEXT("Attack"))
    {
        if (Damage > 0)
        {
            // Format: PLAYER: Sniper attacks enemy at (8,9) for 7 damage
            MoveEntry = FString::Printf(TEXT("%s: %s attacks enemy at (%d,%d) for %d damage"),
                *PlayerIdentifier,
                *UnitType,
                static_cast<int32>(ToPosition.X),
                static_cast<int32>(ToPosition.Y),
                Damage);
        }
        else
        {
            // Format without damage value: PLAYER: Sniper attacks enemy at (8,9)
            MoveEntry = FString::Printf(TEXT("%s: %s attacks enemy at (%d,%d)"),
                *PlayerIdentifier,
                *UnitType,
                static_cast<int32>(ToPosition.X),
                static_cast<int32>(ToPosition.Y));
        }
    }
    else if (ActionType == TEXT("Skip"))
    {
        // Format: PLAYER: Sniper skips turn
        MoveEntry = FString::Printf(TEXT("%s: %s skips turn"),
            *PlayerIdentifier,
            *UnitType);
    }
    else if (ActionType == TEXT("Place"))
    {
        // Format: PLAYER: Placed Sniper at position (8,9)
        MoveEntry = FString::Printf(TEXT("%s: Placed %s at position (%d,%d)"),
            *PlayerIdentifier,
            *UnitType,
            static_cast<int32>(ToPosition.X),
            static_cast<int32>(ToPosition.Y));
    }

    // IMPROVED: Check for duplicates in the raw move history (without turn numbers)
    bool bEntryExists = false;
    for (const FString& ExistingEntry : RawMoveHistory)
    {
        if (ExistingEntry == MoveEntry)
        {
            bEntryExists = true;
            break;
        }
    }

    // Only add if this is a new entry
    if (!bEntryExists)
    {
        // Get current turn from GameInstance
        USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
        if (GameInstance)
        {
            // Format with turn number for game log
            FString TurnPrefixedEntry = FString::Printf(TEXT("Turn %d: %s"),
                GameInstance->CurrentTurnNumber, *MoveEntry);

            // Add to game log with turn number
            GameLog.Add(TurnPrefixedEntry);
            FormattedEntry = TurnPrefixedEntry;

            // Also add to move history WITHOUT turn number
            RawMoveHistory.Add(MoveEntry);

            // Keep logs from growing too large
            while (GameLog.Num() > MaxGameLogEntries)
            {
                GameLog.RemoveAt(0);
            }

            while (RawMoveHistory.Num() > MaxGameLogEntries)
            {
                RawMoveHistory.RemoveAt(0);
            }

            // Log for debugging
            UE_LOG(LogTemp, Display, TEXT("Game Log: %s"), *TurnPrefixedEntry);

            // Update the UI
            UpdateGameHUD();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Duplicate log entry prevented: %s"), *MoveEntry);
    }
}

// Update GetFormattedGameLog to use the raw history without turn numbers
FString ASaT_GameMode::GetFormattedGameLog() const
{
    FString History;

    for (int32 i = 0; i < RawMoveHistory.Num(); i++)
    {
        if (i > 0)
        {
            History.Append(TEXT("\n"));
        }
        History.Append(RawMoveHistory[i]);
    }

    return History;
}

void ASaT_GameMode::ShowAIThinkingWidget(bool bShow)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (!AIThinkingWidget && AIThinkingWidgetClass)
    {
        AIThinkingWidget = CreateWidget<UUserWidget>(PC, AIThinkingWidgetClass);
    }

    if (AIThinkingWidget)
    {
        if (bShow)
        {
            // Explicitly set visibility and add to viewport
            AIThinkingWidget->SetVisibility(ESlateVisibility::Visible);

            // Ensure it's not already in viewport before adding
            if (!AIThinkingWidget->IsInViewport())
            {
                AIThinkingWidget->AddToViewport(999);
            }
        }
        else
        {
            AIThinkingWidget->SetVisibility(ESlateVisibility::Collapsed);
            AIThinkingWidget->RemoveFromParent();
        }
    }
}

void ASaT_GameMode::ShowCoinFlipResultWidget(bool bShow)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (!CoinFlipResultWidget && CoinFlipResultWidgetClass)
    {
        CoinFlipResultWidget = CreateWidget<UUserWidget>(PC, CoinFlipResultWidgetClass);
    }

    if (CoinFlipResultWidget)
    {
        if (bShow)
        {
            // Explicitly set visibility and add to viewport
            CoinFlipResultWidget->SetVisibility(ESlateVisibility::Visible);

            // Ensure it's not already in viewport before adding
            if (!CoinFlipResultWidget->IsInViewport())
            {
                CoinFlipResultWidget->AddToViewport(999);
            }
        }
        else
        {
            // Explicitly remove from viewport and set to hidden
            CoinFlipResultWidget->RemoveFromParent();
            CoinFlipResultWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void ASaT_GameMode::HideCoinFlipWidget()
{
    UE_LOG(LogTemp, Warning, TEXT("Hiding Coin Flip Result Widget"));
    ShowCoinFlipResultWidget(false);
}