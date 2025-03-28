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
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

//  Constructor - initializes default values and widget classes
ASaT_GameMode::ASaT_GameMode()
{
    // Default initialization
    CurrentPlayer = 0;
    bIsGameOver = false;
    CurrentPlayerType = EPlayerType::Human;

    // UI Widget classes
    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultMainGameHUDClass(TEXT("/Game/UI/WBP_MainGameHUD"));
    if (DefaultMainGameHUDClass.Succeeded())
    {
        MainGameHUDClass = DefaultMainGameHUDClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultAIThinkingWidgetClass(TEXT("/Game/UI/WBP_AIThinking"));
    if (DefaultAIThinkingWidgetClass.Succeeded())
    {
        AIThinkingWidgetClass = DefaultAIThinkingWidgetClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultCoinFlipResultWidgetClass(TEXT("/Game/UI/WBP_CoinFlipResult"));
    if (DefaultCoinFlipResultWidgetClass.Succeeded())
    {
        CoinFlipResultWidgetClass = DefaultCoinFlipResultWidgetClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultGameOverWidgetClass(TEXT("/Game/UI/WBP_GameOver"));
    if (DefaultGameOverWidgetClass.Succeeded())
    {
        GameOverWidgetClass = DefaultGameOverWidgetClass.Class;
    }

    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultDifficultySelectionWidgetClass(TEXT("/Game/UI/WBP_DifficultySelection"));
    if (DefaultDifficultySelectionWidgetClass.Succeeded())
    {
        DifficultySelectionWidgetClass = DefaultDifficultySelectionWidgetClass.Class;
    }

}

// Called when the game starts - initializes the grid, players, and UI
void ASaT_GameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!Gmanager && GameManagerClass)
    {
        Gmanager = GetWorld()->SpawnActor<AGridManager>(GameManagerClass);
    }

    if (!Gmanager)
    {
        TArray<AActor*> FoundGrids;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);

        if (FoundGrids.Num() > 0)
        {
            Gmanager = Cast<AGridManager>(FoundGrids[0]);
        }
    }

    // Initialize players
    InitializePlayers();

    if (Gmanager && Players.IsValidIndex(0))
    {
        float CameraPosX = ((Gmanager->TileSize * Gmanager->Size) + ((Gmanager->Size - 1) * Gmanager->TileSize * Gmanager->CellPadding)) * 0.5f;
        float Zposition = 2500.0f;
        FVector CameraPos(CameraPosX, CameraPosX, Zposition);

        ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(Players[0]->_getUObject());
        if (HumanPlayer)
        {
            HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());
        }
    }

    // Get game instance and set game phase to SETUP
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        GameInstance->SetGamePhase(EGamePhase::SETUP);
    }

    // Schedule game start after a short delay
    if (Gmanager)
    {
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, this, &ASaT_GameMode::StartGame, 2.0f, false);
    }

    // Create game HUD if class is set
    if (MainGameHUDClass)
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

    ShowDifficultyWidget(true);
}

void ASaT_GameMode::ShowDifficultyWidget(bool bShow)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (!DifficultyWidget && DifficultySelectionWidgetClass)
    {
        DifficultyWidget = CreateWidget<UUserWidget>(PC, DifficultySelectionWidgetClass);
    }

    if (DifficultyWidget)
    {
        if (bShow)
        {
            // Explicitly set visibility and add to viewport
            DifficultyWidget->SetVisibility(ESlateVisibility::Visible);

            // Ensure it's not already in viewport before adding
            if (!DifficultyWidget->IsInViewport())
            {
                DifficultyWidget->AddToViewport(10000); // High Z-order to be above everything
            }

            // Find and bind the buttons
            UButton* EasyButton = Cast<UButton>(DifficultyWidget->GetWidgetFromName(TEXT("EasyModeButton")));
            UButton* HardButton = Cast<UButton>(DifficultyWidget->GetWidgetFromName(TEXT("HardModeButton")));

            if (EasyButton)
            {
                EasyButton->OnClicked.Clear();
                EasyButton->OnClicked.AddDynamic(this, &ASaT_GameMode::OnEasyModeSelected);
                UE_LOG(LogTemp, Warning, TEXT("Easy Mode Button bound successfully"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to find Easy Mode button in widget!"));
            }

            if (HardButton)
            {
                HardButton->OnClicked.Clear();
                HardButton->OnClicked.AddDynamic(this, &ASaT_GameMode::OnHardModeSelected);
                UE_LOG(LogTemp, Warning, TEXT("Hard Mode Button bound successfully"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to find Hard Mode button in widget!"));
            }

            // Set input mode to UI mode
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(DifficultyWidget->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
        else
        {
            // Hide widget
            HideDifficultyWidget();
        }
    }
}

void ASaT_GameMode::HideDifficultyWidget()
{
    if (DifficultyWidget)
    {
        DifficultyWidget->RemoveFromParent();
        DifficultyWidget = nullptr;

        // Switch back to game input mode
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
    }
}

void ASaT_GameMode::OnEasyModeSelected()
{
    UE_LOG(LogTemp, Warning, TEXT("Easy Mode Selected"));
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        GameInstance->SetupGameWithDifficulty(EAIDifficulty::EASY);
    }
}

void ASaT_GameMode::OnHardModeSelected()
{
    UE_LOG(LogTemp, Warning, TEXT("Hard Mode Selected"));
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GameInstance)
    {
        GameInstance->SetupGameWithDifficulty(EAIDifficulty::HARD);
    }
}

/*
 * Initializes all players at the start of the game
 * Finds human and AI players and sets their properties
 */
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
}

/*
 * Initializes and starts the game
 * Sets up the grid and initiates the first turn
 */
void ASaT_GameMode::StartGame()
{
    // Double-check if GridManager exists
    if (!Gmanager)
    {
        // Last attempt to find the GridManager
        TArray<AActor*> FoundGrids;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);

        if (FoundGrids.Num() > 0)
        {
            Gmanager = Cast<AGridManager>(FoundGrids[0]);
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

    // Flip a coin to decide who goes first
    FlipCoinToDecideFirstPlayer();

    // Set game to playing phase
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    if (GameInstance) 
    {
        GameInstance->SetGamePhase(EGamePhase::SETUP);
    }

    // Start first turn
    StartFirstTurn();

    UpdateGameHUD();

}

/*
 * Randomly determines which player starts the game
 * Updates game state and shows the coin flip result
 */
void ASaT_GameMode::FlipCoinToDecideFirstPlayer()
{
    ShowCoinFlipResultWidget(true);

    // Random 50/50 chance to determine who goes first
    bool bHumanWinsFlip = FMath::RandBool();

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

    GameInstance->bPlayerStartsFirst = bHumanWinsFlip;
    GameInstance->bIsPlayerTurn = bHumanWinsFlip;

    // Sync GameMode's state with GameInstance
    CurrentPlayer = bHumanWinsFlip ? 0 : 1; // 0 = Human, 1 = AI
    CurrentPlayerType = bHumanWinsFlip ? EPlayerType::Human : EPlayerType::AI;

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

/*
 * Initializes the first turn after game start
 * Syncs with GameInstance and notifies the starting player
 */
void ASaT_GameMode::StartFirstTurn()
{

    // Get the game instance to verify turn state
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in StartFirstTurn!"));
        return;
    }

    bool bHumanGoesFirst = GameInstance->bPlayerStartsFirst;
    bool bIsHumanTurn = GameInstance->bIsPlayerTurn;

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

    // Update the player IsMyTurn flags based on CurrentPlayer
    for (int32 i = 0; i < Players.Num(); i++)
    {
        Players[i]->IsMyTurn = (i == CurrentPlayer);

    }

    GetWorldTimerManager().ClearAllTimersForObject(this);

    // Add a short delay before notifying the current player
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ASaT_GameMode::NotifyCurrentPlayerTurn, 0.5f, false);
}

/*
 * Ends the current player's turn
 * Switches to the next player and updates game state
 */
void ASaT_GameMode::EndTurn()
{
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

    if (Players.IsValidIndex(CurrentPlayer))
    {
        Players[CurrentPlayer]->IsMyTurn = false;
        UE_LOG(LogTemp, Warning, TEXT("Setting Player %d IsMyTurn to FALSE"), CurrentPlayer);
    }

    // Let GameInstance handle turn switching
    GameInstance->SwitchTurn();

    // After switching, update our CurrentPlayer based on the new GameInstance state
    bool bIsHumanTurn = GameInstance->bIsPlayerTurn;
    CurrentPlayer = bIsHumanTurn ? 0 : 1;
    CurrentPlayerType = bIsHumanTurn ? EPlayerType::Human : EPlayerType::AI;

    UpdateGameHUD();

    // Schedule notification for the next player
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ASaT_GameMode::NotifyCurrentPlayerTurn, 0.1f, false);
}

/*
 * Checks if the game is over and handles win/lose conditions
 * @return True if the game is over, false otherwise
 */
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
        // Count units for each player
        int32 HumanUnitCount = 0;
        int32 AIUnitCount = 0;
        TArray<AUnit*> HumanUnits;
        TArray<AUnit*> AIUnits;

        // Get all units
        TArray<AActor*> AllUnits;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

        for (AActor* UnitActor : AllUnits)
        {
            AUnit* Unit = Cast<AUnit>(UnitActor);
            if (Unit && Unit->IsAlive())
            {
                if (Unit->bIsPlayerUnit)
                {
                    HumanUnitCount++;
                    HumanUnits.Add(Unit);
                }
                else
                {
                    AIUnitCount++;
                    AIUnits.Add(Unit);
                }
            }
        }

        // Check for standard win/lose conditions
        if (HumanUnitCount == 0 || AIUnitCount == 0)
        {
            // Set game phase to GAMEOVER
            GameInstance->SetGamePhase(EGamePhase::GAMEOVER);

            // Determine the winner
            bool bHumanWins = (AIUnitCount == 0);

            // Set winner text for UI
            WinnerText = bHumanWins ? TEXT("YOU WIN!") : TEXT("AI WINS!");

            // Show the game over widget
            ShowGameOverWidget(true);

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

        // Only check for potential draw if we're in the PLAYING phase
        bool bPotentialDraw = false;
        if (HumanUnitCount == 1 && AIUnitCount == 1)
        {
            bPotentialDraw = CheckPotentialDraw(HumanUnits, AIUnits);
        }

        if (bPotentialDraw)
        {
            // Set game phase to GAMEOVER
            GameInstance->SetGamePhase(EGamePhase::GAMEOVER);

            // Set draw text
            WinnerText = TEXT("DRAW!");

            // Show the game over widget
            ShowGameOverWidget(true);

            // Notify players of the draw
            for (auto PlayerInterface : Players)
            {
                if (PlayerInterface)
                {
                    PlayerInterface->OnDraw();
                }
            }

            return true;
        }
    }

    return false;
}

/*
 * Checks for a potential draw between two units
 * @param HumanUnits Array of human player units
 * @param AIUnits Array of AI player units
 * @return True if a draw condition is detected
 */
bool ASaT_GameMode::CheckPotentialDraw(const TArray<AUnit*>& HumanUnits, const TArray<AUnit*>& AIUnits)
{
    if (HumanUnits.Num() != 1 || AIUnits.Num() != 1)
        return false;

    // Get the remaining units
    AUnit* HumanUnit = HumanUnits[0];
    AUnit* AIUnit = AIUnits[0];

    // Verify units are still alive
    if (!HumanUnit->IsAlive() || !AIUnit->IsAlive())
        return false;

    ASniper* HumanSniper = Cast<ASniper>(HumanUnit);
    ASniper* AISniper = Cast<ASniper>(AIUnit);

    if (!HumanSniper || !AISniper)
        return false;

    // For mutual destruction, BOTH need:
    // 1. To be within attack range of each other
    // 2. Have HP so low that counterattack would kill them

    // First, check if they can attack each other
    bool HumanCanAttackAI = HumanSniper->IsTargetInRange(AISniper);
    bool AICanAttackHuman = AISniper->IsTargetInRange(HumanSniper);

    if (!HumanCanAttackAI || !AICanAttackHuman)
        return false;

    // Simulate counterattack damage 
    int32 CounterAttackDamage = 1; // Minimum counterattack damage

    // Only declare draw if both units would kill each other with minimum counterattack
    bool HumanWouldDie = (HumanSniper->Hp <= CounterAttackDamage);
    bool AIWouldDie = (AISniper->Hp <= CounterAttackDamage);

    return HumanWouldDie && AIWouldDie;
}

/*
 * Handles special draw conditions in the game
 * Checks for mutual destruction or stalemate scenarios
 */
void ASaT_GameMode::HandleDrawCondition()
{
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    // Count living units by team
    int32 HumanUnitsAlive = 0;
    int32 AIUnitsAlive = 0;
    TArray<AUnit*> LivingHumanUnits;
    TArray<AUnit*> LivingAIUnits;

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit && Unit->IsAlive())
        {
            if (Unit->bIsPlayerUnit)
            {
                HumanUnitsAlive++;
                LivingHumanUnits.Add(Unit);
            }
            else
            {
                AIUnitsAlive++;
                LivingAIUnits.Add(Unit);
            }
        }
    }

    bool bValidDrawCondition = false;

    // Case 1: No units left on either side
    if (HumanUnitsAlive == 0 && AIUnitsAlive == 0)
    {
        bValidDrawCondition = true;
        UE_LOG(LogTemp, Warning, TEXT("Draw condition: No units left on either side"));
    }
    // Case 2: One unit each
    else if (HumanUnitsAlive == 1 && AIUnitsAlive == 1)
    {
        // Verify both are snipers
        AUnit* HumanUnit = LivingHumanUnits[0];
        AUnit* AIUnit = LivingAIUnits[0];

        ASniper* HumanSniper = Cast<ASniper>(HumanUnit);
        ASniper* AISniper = Cast<ASniper>(AIUnit);

        if (!HumanSniper || !AISniper)
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid draw: Units are not both snipers"));
            return;
        }

        if (HumanSniper->Hp == 1 && AISniper->Hp == 1 &&
            HumanSniper->IsTargetInRange(AISniper) && AISniper->IsTargetInRange(HumanSniper))
        {
            bValidDrawCondition = true;
        }
        else
        {
            return;
        }
    }
    else
    {
        // If there are still units left on either side, this is not a draw
        UE_LOG(LogTemp, Warning, TEXT("Invalid draw: Units remain on at least one side"));
        return;
    }

    // If we reach here with a valid draw condition, proceed with the draw
    if (bValidDrawCondition)
    {
        USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
        if (GameInstance)
        {
            // Set game phase to GAMEOVER
            GameInstance->SetGamePhase(EGamePhase::GAMEOVER);

            // Set winner text to indicate a draw
            WinnerText = TEXT("DRAW!");

            // Show game over widget
            ShowGameOverWidget(true);

            // Notify players about the draw
            for (auto PlayerInterface : Players)
            {
                if (PlayerInterface)
                {
                    PlayerInterface->OnDraw();
                }
            }
        }
    }
}

/*
 * Legacy method for switching to the next player
 * Redirects to EndTurn for consistency
 */
void ASaT_GameMode::TurnNextPlayer()
{
    EndTurn();
}

/*
 * Notifies the current player that it's their turn
 * Resets unit flags and updates game state
 */
void ASaT_GameMode::NotifyCurrentPlayerTurn()
{
    // Get the GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found in NotifyCurrentPlayerTurn!"));
        return;
    }

    // Determine whose turn it is
    bool bIsHumanTurn = GameInstance->bIsPlayerTurn;

    // Reset movement and attack flags for units
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);

    for (AActor* UnitActor : AllUnits)
    {
        AUnit* Unit = Cast<AUnit>(UnitActor);
        if (Unit)
        {
            // Reset flags based on the unit's player type
            if (Unit->bIsPlayerUnit == bIsHumanTurn)
            {
                Unit->bHasMovedThisTurn = false;
                Unit->bHasAttackedThisTurn = false;

                UE_LOG(LogTemp, Warning, TEXT("Reset flags for unit: %s"), *Unit->GetName());
            }
        }
    }

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

    int32 ExpectedPlayerIndex = bIsHumanTurn ? 0 : 1;

    // If there's a mismatch, correct it
    if (CurrentPlayer != ExpectedPlayerIndex)
    {
        CurrentPlayer = ExpectedPlayerIndex;
        CurrentPlayerType = bIsHumanTurn ? EPlayerType::Human : EPlayerType::AI;
    }

    if (!Players.IsValidIndex(CurrentPlayer))
    {
        UE_LOG(LogTemp, Error, TEXT("NotifyCurrentPlayerTurn: Invalid player index %d"), CurrentPlayer);
        return;
    }

    // Update all player turn flags
    for (int32 i = 0; i < Players.Num(); i++)
    {
        Players[i]->IsMyTurn = (i == CurrentPlayer);
    }

    ISaT_PlayerInterface* PlayerToNotify = Players[CurrentPlayer];
    if (PlayerToNotify)
    {
        // Double-check one more time - is this really the right player?
        bool bIsPlayerHuman = (Cast<ASaT_HumanPlayer>(PlayerToNotify->_getUObject()) != nullptr);

        if (bIsPlayerHuman == bIsHumanTurn)
        {
            PlayerToNotify->OnTurn();
        }
        else
        {
            // We have a mismatch!
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

/*
 * Called when a unit dies to update game state
 * @param DeadUnit The unit that died
 */
void ASaT_GameMode::NotifyUnitDeath(AUnit* DeadUnit)
{
    if (!DeadUnit)
        return;

    // Check if this affects game over conditions
    CheckGameOver();

    // Update the UI
    UpdateGameHUD();
}

/*
 * Updates all UI elements with current game state
 * Retrieves unit information and updates HUD displays
 */
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

    PlayerSniperPos = TEXT("--");
    PlayerBrawlerPos = TEXT("--");
    AISniperPos = TEXT("--");
    AIBrawlerPos = TEXT("--");

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
                    PlayerSniperPos = AGridManager::ConvertToLetterNumberFormat(Unit->GridX, Unit->GridY);
                }
                else if (Cast<ABrawler>(Unit))
                {
                    // Update player brawler info
                    PlayerBrawlerHP = Unit->Hp;
                    PlayerBrawlerPos = AGridManager::ConvertToLetterNumberFormat(Unit->GridX, Unit->GridY);
                }
            }
            else
            {
                // AI units
                if (Cast<ASniper>(Unit))
                {
                    // Update AI sniper info
                    AISniperHP = Unit->Hp;
                    AISniperPos = AGridManager::ConvertToLetterNumberFormat(Unit->GridX, Unit->GridY);
                }
                else if (Cast<ABrawler>(Unit))
                {
                    // Update AI brawler info
                    AIBrawlerHP = Unit->Hp;
                    AIBrawlerPos = AGridManager::ConvertToLetterNumberFormat(Unit->GridX, Unit->GridY);
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
}

/*
 * Sets the currently selected unit
 * @param Unit The unit to select
 */
void ASaT_GameMode::SetSelectedUnit(AUnit* Unit)
{

    CurrentlySelectedUnit = Unit;
}

/*
 * Gets information text about the currently selected unit
 * @return String with unit information or "No Unit Selected"
 */
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
void ASaT_GameMode::AddFormattedMoveToLog(bool bIsPlayerUnit, const FString& UnitType, const FString& ActionType,
    const FVector2D& FromPosition, const FVector2D& ToPosition, int32 Damage)
{
    // Format the player identifier
    FString PlayerIdentifier = bIsPlayerUnit ? TEXT("PLAYER") : TEXT("AI");

    // Convert grid coordinates to letter-number format
    FString FromPosStr = AGridManager::ConvertToLetterNumberFormat(FromPosition.X, FromPosition.Y);
    FString ToPosStr = AGridManager::ConvertToLetterNumberFormat(ToPosition.X, ToPosition.Y);

    // Format the move entry based on action type
    FString MoveEntry;
    if (ActionType == TEXT("Move"))
    {
        // Format: PLAYER: Moved Sniper from B4 to D6
        MoveEntry = FString::Printf(TEXT("%s: Moved %s from %s to %s"),
            *PlayerIdentifier,
            *UnitType,
            *FromPosStr,
            *ToPosStr);
    }
    else if (ActionType == TEXT("Attack"))
    {
        if (Damage > 0)
        {
            // Format: PLAYER: Sniper attacks enemy at G8 for 7 damage
            MoveEntry = FString::Printf(TEXT("%s: %s attacks enemy at %s for %d damage"),
                *PlayerIdentifier,
                *UnitType,
                *ToPosStr,
                Damage);
        }
        else
        {
            // Format without damage value: PLAYER: Sniper attacks enemy at G8
            MoveEntry = FString::Printf(TEXT("%s: %s attacks enemy at %s"),
                *PlayerIdentifier,
                *UnitType,
                *ToPosStr);
        }
    }
    else if (ActionType == TEXT("Counterattack"))
    {
        // Format: PLAYER: Brawler counterattacks enemy at G8 for 2 damage
        MoveEntry = FString::Printf(TEXT("%s: %s counterattacks enemy at %s for %d damage"),
            *PlayerIdentifier,
            *UnitType,
            *ToPosStr,
            Damage);
    }
    else if (ActionType == TEXT("Place"))
    {
        // Format: PLAYER: Placed Sniper at position G8
        MoveEntry = FString::Printf(TEXT("%s: Placed %s at position %s"),
            *PlayerIdentifier,
            *UnitType,
            *ToPosStr);
    }

    // IMPROVED: Check for duplicates in the raw move history
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

            // Update the UI
            UpdateGameHUD();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Duplicate log entry prevented: %s"), *MoveEntry);
    }
}

/*
 * Get the formatted move history as a string
 * @return String containing all move history entries
 */
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

/*
 * Shows or hides the AI thinking widget
 * @param bShow Whether to show (true) or hide (false) the widget
 */
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

/*
 * Shows or hides the coin flip result widget
 * @param bShow Whether to show (true) or hide (false) the widget
 */
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

// Convenience method to hide the coin flip widget
void ASaT_GameMode::HideCoinFlipWidget()
{
    ShowCoinFlipResultWidget(false);
}


/*
 * Shows or hides the game over widget
 * Sets up the reset button and input mode
 * @param bShow Whether to show (true) or hide (false) the widget
 */
void ASaT_GameMode::ShowGameOverWidget(bool bShow)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (!GameOverWidget && GameOverWidgetClass)
    {
        GameOverWidget = CreateWidget<UUserWidget>(PC, GameOverWidgetClass);
    }

    if (GameOverWidget)
    {
        if (bShow)
        {
            // Explicitly set visibility and add to viewport
            GameOverWidget->SetVisibility(ESlateVisibility::Visible);

            // Ensure it's not already in viewport before adding
            if (!GameOverWidget->IsInViewport())
            {
                GameOverWidget->AddToViewport(10000);
            }

            // Find the Reset button directly
            UButton* ResetButton = Cast<UButton>(GameOverWidget->GetWidgetFromName(TEXT("ResetButton")));

            if (ResetButton)
            {
                ResetButton->OnClicked.Clear(); // Clear any existing bindings
                ResetButton->OnClicked.AddDynamic(this, &ASaT_GameMode::ResetGame);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to find Reset button in widget!"));
            }

            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(GameOverWidget->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
        else
        {
            GameOverWidget->RemoveFromParent();
            GameOverWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Game Over widget"));
    }
}

/*
 * Resets the game to its initial state
 * Destroys all units and tiles, regenerates the grid, and starts a new game
 */
void ASaT_GameMode::ResetGame()
{
    // Ensure we have a valid GameInstance
    USaT_GameInstance* GameInstance = Cast<USaT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot reset game: GameInstance not found!"));
        return;
    }

    // Hide game over widget first
    ShowGameOverWidget(false);

    // Destroy ALL actors of relevant classes
    TArray<AActor*> AllUnits, AllTiles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnit::StaticClass(), AllUnits);
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATile::StaticClass(), AllTiles);

    // Destroy units
    for (AActor* UnitActor : AllUnits)
    {
        if (UnitActor)
        {
            UnitActor->Destroy();
        }
    }

    // Destroy tiles
    for (AActor* TileActor : AllTiles)
    {
        if (TileActor)
        {
            TileActor->Destroy();
        }
    }

    // Reset game state in GameInstance
    GameInstance->CurrentPhase = EGamePhase::SETUP;
    GameInstance->bIsPlayerTurn = true;  // Ensure it starts on player turn
    GameInstance->bPlayerStartsFirst = true;  // Reset coin flip
    GameInstance->HumanUnitsPlaced = 0;
    GameInstance->AIUnitsPlaced = 0;
    GameInstance->CurrentTurnNumber = 1;

    // Clear any stored game data
    GameLog.Empty();
    RawMoveHistory.Empty();
    CurrentlySelectedUnit = nullptr;

    // Reset current player state
    CurrentPlayer = 0;  // Start with human player
    CurrentPlayerType = EPlayerType::Human;

    // Re-initialize players
    InitializePlayers();

    // Find the human and AI players and reset their placement flags
    for (auto PlayerInterface : Players)
    {
        // Reset for Human Player
        ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(PlayerInterface->_getUObject());
        if (HumanPlayer)
        {
            // Reset human player placement flags
            HumanPlayer->PlacedUnitsCount = 0;
            HumanPlayer->bHasPlacedSniper = false;
            HumanPlayer->bHasPlacedBrawler = false;
            HumanPlayer->UnitsToPlace = 2;
        }

        // Reset for AI Player
        ASaT_RandomPlayer* RandomPlayer = Cast<ASaT_RandomPlayer>(PlayerInterface->_getUObject());
        if (RandomPlayer)
        {
            // Reset AI player placement flags
            RandomPlayer->PlacedUnitsCount = 0;
        }
    }

    // Reset and regenerate the grid
    if (Gmanager)
    {
        // Clear all data structures
        Gmanager->ClearAllHighlights();
        Gmanager->ClearPathHighlights();
        Gmanager->TileArray.Empty();
        Gmanager->TileMap.Empty();
        Gmanager->HighlightedTiles.Empty();
        Gmanager->PathTiles.Empty();

        // Regenerate the grid
        Gmanager->GenerateField();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GridManager is NULL during game reset!"));
    }

    // Restart the game
    StartGame();

    // Force update of game HUD
    UpdateGameHUD();
}

