// Fill out your copyright notice in the Description page of Project Settings.

#include "SaT_GameInstance.h"
#include "SaT_GameMode.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"

// Constructor
USaT_GameInstance::USaT_GameInstance()
{
    // Default initialization
    bPlayerStartsFirst = true;
    bIsPlayerTurn = true;
    CurrentPhase = EGamePhase::SETUP;
    HumanUnitsPlaced = 0;
    AIUnitsPlaced = 0;
    CurrentTurnNumber = 1;

    AIDifficulty = EAIDifficulty::HARD;
}


// Sets the AI difficulty level and logs the change
void USaT_GameInstance::SetAIDifficulty(EAIDifficulty NewDifficulty)
{
    AIDifficulty = NewDifficulty;
    UE_LOG(LogTemp, Warning, TEXT("AI Difficulty set to: %s"),
        AIDifficulty == EAIDifficulty::EASY ? TEXT("EASY") : TEXT("HARD"));
}

//  Sets up the game with the selected difficulty and regenerates the grid
void USaT_GameInstance::SetupGameWithDifficulty(EAIDifficulty Difficulty)
{
    // Store the difficulty setting
    SetAIDifficulty(Difficulty);

    // Restart the game setup process
    AGameModeBase* GameModeBase = UGameplayStatics::GetGameMode(GetWorld());
    ASaT_GameMode* GameMode = Cast<ASaT_GameMode>(GameModeBase);
    if (GameMode)
    {
        // Clear the difficulty widget and start the game
        GameMode->HideDifficultyWidget();

        // Find the grid manager to adjust obstacle percentage
        AGridManager* GridManager = nullptr;
        TArray<AActor*> FoundGrids;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundGrids);
        if (FoundGrids.Num() > 0)
        {
            GridManager = Cast<AGridManager>(FoundGrids[0]);

            UMaterialInterface* SavedPathMaterial = nullptr;
            if (GridManager)
            {
                SavedPathMaterial = GridManager->PathMaterial;

                // Adjust obstacle percentage based on difficulty
                if (Difficulty == EAIDifficulty::HARD)
                {
                    // More obstacles in Hard mode
                    GridManager->ObstaclePercentage = 0.2f; // 20% obstacles
                }
                else
                {
                    // Fewer obstacles in Easy mode
                    GridManager->ObstaclePercentage = 0.1f; // 10% obstacles
                }

                // Regenerate the grid with the new obstacle percentage
                GridManager->TileArray.Empty();
                GridManager->TileMap.Empty();
                GridManager->HighlightedTiles.Empty();
                GridManager->PathTiles.Empty();
                GridManager->GenerateField();

                if (SavedPathMaterial)
                {
                    GridManager->PathMaterial = SavedPathMaterial;
                }
            }
        }

        // Start the game
        GameMode->StartGame();
    }
}

/*
 * Randomly determines which player starts the game
 * Uses a 50/50 chance to set bPlayerStartsFirst and bIsPlayerTurn
 */
void USaT_GameInstance::TossCoin()
{
    // 50/50 chance to determine who starts first
    bPlayerStartsFirst = FMath::RandBool();
    bIsPlayerTurn = bPlayerStartsFirst;

}

/*
 * Switches the turn between players and handles phase transitions
 * Updates the turn counter and checks if the setup phase is complete
 */
void USaT_GameInstance::SwitchTurn()
{
    // Always toggle the turn flag
    bIsPlayerTurn = !bIsPlayerTurn;

    if (bIsPlayerTurn) 
    {
        // Increment turn counter
        CurrentTurnNumber++;
    }

    // Then handle phase transition if needed
    if (CurrentPhase == EGamePhase::SETUP && IsSetupComplete())
    {
        // Transition to playing phase
        SetGamePhase(EGamePhase::PLAYING);
        UE_LOG(LogTemp, Warning, TEXT("Setup phase complete! Transitioning to PLAYING phase."));

    }
}

/*
 * Checks if the setup phase is complete by verifying both players have placed their units
 * @return True if both players have placed the required number of units
 */
bool USaT_GameInstance::IsSetupComplete() const
{
    // Both players need to place 2 units each
    constexpr int32 UnitsPerPlayer = 2;
    bool result = (HumanUnitsPlaced >= UnitsPerPlayer && AIUnitsPlaced >= UnitsPerPlayer);

    return result;
}

/*
 * Checks if the game is over
 * @return True if the current phase is GAMEOVER
 */
bool USaT_GameInstance::CheckGameOver()
{
    return (CurrentPhase == EGamePhase::GAMEOVER);
}

// Sets the current game phase and logs the change
void USaT_GameInstance::SetGamePhase(EGamePhase NewPhase)
{
    if (CurrentPhase != NewPhase)
    {
        CurrentPhase = NewPhase;

        FString PhaseStr;
        switch (CurrentPhase)
        {
        case EGamePhase::SETUP:
            PhaseStr = "SETUP";
            break;
        case EGamePhase::PLAYING:
            PhaseStr = "PLAYING";
            break;
        case EGamePhase::GAMEOVER:
            PhaseStr = "GAMEOVER";
            break;
        default:
            PhaseStr = "UNKNOWN";
            break;
        }

        UE_LOG(LogTemp, Display, TEXT("Game phase changed to: %s"), *PhaseStr);
    }

}

/*
 * Gets the current game phase
 * @return The current game phase enum value
 */
EGamePhase USaT_GameInstance::GetGamePhase() const
{
    return CurrentPhase;
}