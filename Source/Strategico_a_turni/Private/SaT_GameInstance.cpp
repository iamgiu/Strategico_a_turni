// Fill out your copyright notice in the Description page of Project Settings.


// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameInstance.h"
#include "SaT_GameMode.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"

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

void USaT_GameInstance::SetAIDifficulty(EAIDifficulty NewDifficulty)
{
    AIDifficulty = NewDifficulty;
    UE_LOG(LogTemp, Warning, TEXT("AI Difficulty set to: %s"),
        AIDifficulty == EAIDifficulty::EASY ? TEXT("EASY") : TEXT("HARD"));
}

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

            // IMPORTANT: Save reference to path material before regenerating
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

                // IMPORTANT: Restore path material after regenerating
                if (SavedPathMaterial)
                {
                    GridManager->PathMaterial = SavedPathMaterial;
                    UE_LOG(LogTemp, Warning, TEXT("Path material preserved during grid regeneration"));
                }
            }
        }

        // Start the game
        GameMode->StartGame();
    }

    UE_LOG(LogTemp, Warning, TEXT("Setting up game with difficulty: %s - Obstacle percentage: %f%%"),
        Difficulty == EAIDifficulty::EASY ? TEXT("EASY") : TEXT("HARD"),
        (Difficulty == EAIDifficulty::EASY ? 0.1f : 0.2f) * 100.0f);
}

void USaT_GameInstance::TossCoin()
{
    // Debug log to confirm function is called
    UE_LOG(LogTemp, Warning, TEXT("===== GAMEINSTANCE: TOSS COIN CALLED ====="));

    // Log the initial state
    UE_LOG(LogTemp, Warning, TEXT("Before toss: bPlayerStartsFirst = %s"),
        bPlayerStartsFirst ? TEXT("TRUE (Human)") : TEXT("FALSE (AI)"));

    // 50/50 chance to determine who starts first
    bPlayerStartsFirst = FMath::RandBool();
    bIsPlayerTurn = bPlayerStartsFirst;

    // Log the random value generated
    float RandomValue = FMath::FRand(); // Just for debugging to show the random value
    UE_LOG(LogTemp, Warning, TEXT("Random value generated (for reference): %f"), RandomValue);

    // Log the final result with more attention-grabbing format
    UE_LOG(LogTemp, Warning, TEXT("****** COIN TOSS RESULT: %s GOES FIRST ******"),
        bPlayerStartsFirst ? TEXT("HUMAN PLAYER") : TEXT("AI PLAYER"));

    // Log the final state
    UE_LOG(LogTemp, Warning, TEXT("After toss: bPlayerStartsFirst = %s, bIsPlayerTurn = %s"),
        bPlayerStartsFirst ? TEXT("TRUE (Human)") : TEXT("FALSE (AI)"),
        bIsPlayerTurn ? TEXT("TRUE (Human)") : TEXT("FALSE (AI)"));
}

void USaT_GameInstance::SwitchTurn()
{
    // Log the current state before switching
    UE_LOG(LogTemp, Warning, TEXT("===== SWITCHING TURNS ====="));

    // Always toggle the turn flag
    bIsPlayerTurn = !bIsPlayerTurn;

    // If we've completed a full turn cycle (both players have moved)
    if (bIsPlayerTurn) 
    {
        // Increment turn counter
        CurrentTurnNumber++;
    }

    UE_LOG(LogTemp, Warning, TEXT("BEFORE SWITCH - Current turn: %s, Phase: %s"),
        bIsPlayerTurn ? TEXT("Human Player") : TEXT("AI Player"),
        CurrentPhase == EGamePhase::SETUP ? TEXT("SETUP") :
        CurrentPhase == EGamePhase::PLAYING ? TEXT("PLAYING") : TEXT("GAMEOVER"));

    // Then handle phase transition if needed
    if (CurrentPhase == EGamePhase::SETUP && IsSetupComplete())
    {
        // Transition to playing phase
        SetGamePhase(EGamePhase::PLAYING);
        UE_LOG(LogTemp, Warning, TEXT("Setup phase complete! Transitioning to PLAYING phase."));

    }

    // Log the state after switching
    UE_LOG(LogTemp, Warning, TEXT("AFTER SWITCH - Current turn: %s, Phase: %s"),
        bIsPlayerTurn ? TEXT("Human Player") : TEXT("AI Player"),
        CurrentPhase == EGamePhase::SETUP ? TEXT("SETUP") :
        CurrentPhase == EGamePhase::PLAYING ? TEXT("PLAYING") : TEXT("GAMEOVER"));
}

bool USaT_GameInstance::IsSetupComplete() const
{
    // Both players need to place 2 units each
    constexpr int32 UnitsPerPlayer = 2;
    bool result = (HumanUnitsPlaced >= UnitsPerPlayer && AIUnitsPlaced >= UnitsPerPlayer);

    UE_LOG(LogTemp, Warning, TEXT("IsSetupComplete check: Human units=%d, AI units=%d, Result=%s"),
        HumanUnitsPlaced, AIUnitsPlaced, result ? TEXT("TRUE") : TEXT("FALSE"));

    return result;
}

bool USaT_GameInstance::CheckGameOver()
{
    return (CurrentPhase == EGamePhase::GAMEOVER);
}

void USaT_GameInstance::SetGamePhase(EGamePhase NewPhase)
{
    if (CurrentPhase != NewPhase)
    {
        CurrentPhase = NewPhase;

        // Log phase change for debugging
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

EGamePhase USaT_GameInstance::GetGamePhase() const
{
    return CurrentPhase;
}