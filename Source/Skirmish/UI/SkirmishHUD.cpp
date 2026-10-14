#include "SkirmishHUD.h"
#include "SkirmishHUDWidget.h"

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
