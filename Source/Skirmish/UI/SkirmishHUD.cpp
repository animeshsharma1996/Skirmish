#include "SkirmishHUD.h"
#include "SkirmishHUDWidget.h"
#include "SkirmishPauseMenuWidget.h"

void ASkirmishHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PlayerController = GetOwningPlayerController();

    if (!HUDWidgetClass || !PlayerController)
    {
        return;
    }

    HUDWidget = CreateWidget<USkirmishHUDWidget>(PlayerController, HUDWidgetClass);

    if (HUDWidget)
    {
        HUDWidget->AddToViewport();
    }
}

void ASkirmishHUD::TogglePauseMenu()
{
    APlayerController* PlayerController = GetOwningPlayerController();

    if (!PlayerController || !PauseMenuClass)
    {
        return;
    }

    if (PauseMenu && PauseMenu->IsInViewport())
    {
        ClosePauseMenu();
        return;
    }

    if (!PauseMenu)
    {
        PauseMenu = CreateWidget<USkirmishPauseMenuWidget>(PlayerController, PauseMenuClass);

        if (!PauseMenu)
        {
            return;
        }

        PauseMenu->OnResumeRequested.AddDynamic(this, &ASkirmishHUD::ClosePauseMenu);
    }

    PauseMenu->AddToViewport(10);

    // Online games never pause - the menu only frees the mouse
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(PauseMenu->TakeWidget());
    PlayerController->SetInputMode(InputMode);
    PlayerController->SetShowMouseCursor(true);
}

void ASkirmishHUD::ClosePauseMenu()
{
    APlayerController* PlayerController = GetOwningPlayerController();

    if (PauseMenu)
    {
        PauseMenu->RemoveFromParent();
    }

    if (PlayerController)
    {
        PlayerController->SetInputMode(FInputModeGameOnly());
        PlayerController->SetShowMouseCursor(false);
    }
}
