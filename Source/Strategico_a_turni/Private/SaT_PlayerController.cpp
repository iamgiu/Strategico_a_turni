#include "SaT_PlayerController.h"
#include "SaT_HumanPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// Constructor - initializes default values
ASaT_PlayerController::ASaT_PlayerController()
{
    // Constructor implementation
}

/*
 * Called when the controller begins play
 * Sets up input mode and enhanced input system
 */
void ASaT_PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Enable mouse cursor and set default input mode
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;

    // Set input mode to game and UI
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    // Enhanced Input setup with verification
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (SaTContext)
        {
            Subsystem->AddMappingContext(SaTContext, 0);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SaTContext is not set in PlayerController"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get EnhancedInputSubsystem"));
    }

    DebugInputState();
}

/*
 * Sets up input component bindings
 * Configures enhanced input actions
 */
void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ASaT_PlayerController::ClickOnGrid);
        }
    }
}

/*
 * Processes click input on the grid
 * Includes debouncing to prevent double-clicks
 * @param Value The input action value
 */
void ASaT_PlayerController::ClickOnGrid(const FInputActionValue& Value)
{
    // Get timestamp to prevent double-processing clicks
    static double LastClickTime = 0.0;
    double CurrentTime = FPlatformTime::Seconds();

    if (CurrentTime - LastClickTime < 0.25)
    {
        return;
    }

    LastClickTime = CurrentTime;

    // Get the pawn as SaT_HumanPlayer and call OnClick
    ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
    if (HumanPlayer)
    {
        HumanPlayer->OnClick();
    }
}

/*
 * Utility to log the current state of the input system
 * Useful for debugging input configuration issues
 */
void ASaT_PlayerController::DebugInputState()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (SaTContext)
        {
            // Simple check
            bool bHasContext = false;

            // Try-catch to avoid any runtime errors
            try
            {
                bHasContext = Subsystem->HasMappingContext(SaTContext);
            }
            catch (...)
            {
                UE_LOG(LogTemp, Error, TEXT("Error checking mapping context"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SaTContext is NULL"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Enhanced Input Subsystem NOT found!"));
    }
}