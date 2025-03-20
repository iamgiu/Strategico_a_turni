// Fill out your copyright notice in the Description page of Project Settings.


#include "SaT_GameInstance.h"
#include "Kismet/GameplayStatics.h"

USaT_GameInstance::USaT_GameInstance()
{
    // Default initialization
    bPlayerStartsFirst = true;
    bIsPlayerTurn = true;
    CurrentPhase = EGamePhase::SETUP;
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
    // Only switch turns if we're in the playing phase
    if (CurrentPhase == EGamePhase::PLAYING)
    {
        bIsPlayerTurn = !bIsPlayerTurn;
        UE_LOG(LogTemp, Display, TEXT("Turn switched to %s"),
            bIsPlayerTurn ? TEXT("Human Player") : TEXT("AI Player"));
    }
}

bool USaT_GameInstance::CheckGameOver()
{
    // This method should be implemented with the specific game over conditions
    // For now, it always returns false (game not over)
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