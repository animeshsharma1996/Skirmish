#include "SkirmishMenuHUD.h"
#include "SkirmishMainMenuWidget.h"

void ASkirmishMenuHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PlayerController = GetOwningPlayerController();

    if (!MainMenuClass || !PlayerController)
    {
        return;
    }

    MainMenu = CreateWidget<USkirmishMainMenuWidget>(PlayerController, MainMenuClass);

    if (!MainMenu)
    {
        return;
    }

    MainMenu->AddToViewport();

    // Only the menu gets input, and the mouse cursor is visible
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(MainMenu->TakeWidget());
    PlayerController->SetInputMode(InputMode);
    PlayerController->SetShowMouseCursor(true);
}
