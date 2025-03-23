#include "SaT_PlayerController.h"
#include "SaT_HumanPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ASaT_PlayerController::ASaT_PlayerController()
{
    // Costruttore
}

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
            UE_LOG(LogTemp, Warning, TEXT("SaT Input context added successfully"));
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

/*void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Binding per Enhanced Input System
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ASaT_PlayerController::ClickOnGrid);
        }
    }
}*/

void ASaT_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            // Use Triggered instead of Started for more reliable input detection
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &ASaT_PlayerController::ClickOnGrid);

            // Also bind to Completed as a fallback
            EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Completed, this, &ASaT_PlayerController::ClickOnGrid);
        }

        // Keep existing camera panning code
        InputComponent->BindAxis("PanCameraX", this, &ASaT_PlayerController::PanCameraX);
        InputComponent->BindAxis("PanCameraY", this, &ASaT_PlayerController::PanCameraY);
    }
}

// Implementazione delle funzioni di movimento della camera
void ASaT_PlayerController::PanCameraX(float Value)
{
    if (Value != 0.0f)
    {
        ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
        if (HumanPlayer)
        {
            FVector Location = HumanPlayer->GetActorLocation();
            Location.X += Value * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
            HumanPlayer->SetActorLocation(Location);
        }
    }
}

void ASaT_PlayerController::PanCameraY(float Value)
{
    if (Value != 0.0f)
    {
        ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
        if (HumanPlayer)
        {
            FVector Location = HumanPlayer->GetActorLocation();
            Location.Y += Value * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
            HumanPlayer->SetActorLocation(Location);
        }
    }
}

void ASaT_PlayerController::ClickOnGrid(const FInputActionValue& Value)
{
    // Get timestamp to prevent double-processing clicks
    static double LastClickTime = 0.0;
    double CurrentTime = FPlatformTime::Seconds();

    // Ensure we don't process clicks too rapidly (adjust this threshold as needed)
    if (CurrentTime - LastClickTime < 0.25)
    {
        return;
    }

    LastClickTime = CurrentTime;

    // Ottiene il pawn come SaT_HumanPlayer e chiama OnClick
    ASaT_HumanPlayer* HumanPlayer = Cast<ASaT_HumanPlayer>(GetPawn());
    if (HumanPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("Processing click at time: %f"), CurrentTime);
        HumanPlayer->OnClick();
    }
}

void ASaT_PlayerController::DebugInputState()
{
    UE_LOG(LogTemp, Warning, TEXT("===== INPUT DEBUG ====="));
    UE_LOG(LogTemp, Warning, TEXT("MouseVisible: %s"), bShowMouseCursor ? TEXT("TRUE") : TEXT("FALSE"));

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Enhanced Input Subsystem found"));

        if (SaTContext)
        {
            UE_LOG(LogTemp, Warning, TEXT("SaTContext is valid"));

            // Simple check without using potentially unsupported methods
            bool bHasContext = false;

            // Try-catch to avoid any runtime errors
            try
            {
                bHasContext = Subsystem->HasMappingContext(SaTContext);
                UE_LOG(LogTemp, Warning, TEXT("SaTContext is %s"),
                    bHasContext ? TEXT("applied") : TEXT("NOT applied"));
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

    UE_LOG(LogTemp, Warning, TEXT("======================="));
}