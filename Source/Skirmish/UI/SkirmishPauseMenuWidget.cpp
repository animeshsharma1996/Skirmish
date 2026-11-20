#include "SkirmishPauseMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Online/SkirmishSessionSubsystem.h"
#include "SkirmishPlayerController.h"

void USkirmishPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ResumeButton)
    {
        ResumeButton->OnClicked.AddDynamic(this, &USkirmishPauseMenuWidget::HandleResumeClicked);
    }

    if (ReadyButton)
    {
        ReadyButton->OnClicked.AddDynamic(this, &USkirmishPauseMenuWidget::HandleReadyClicked);
    }

    if (LeaveButton)
    {
        LeaveButton->OnClicked.AddDynamic(this, &USkirmishPauseMenuWidget::HandleLeaveClicked);
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &USkirmishPauseMenuWidget::HandleQuitClicked);
    }
}

void USkirmishPauseMenuWidget::HandleResumeClicked()
{
    // The HUD owns the menu, so it decides how to close it
    OnResumeRequested.Broadcast();
}

void USkirmishPauseMenuWidget::HandleReadyClicked()
{
    ASkirmishPlayerController* SkirmishPlayerController = Cast<ASkirmishPlayerController>(GetOwningPlayer());

    if (SkirmishPlayerController)
    {
        SkirmishPlayerController->Ready();
    }
}

void USkirmishPauseMenuWidget::HandleLeaveClicked()
{
    UGameInstance* GameInstance = GetGameInstance();
    USkirmishSessionSubsystem* SkirmishSession = GameInstance ? GameInstance->GetSubsystem<USkirmishSessionSubsystem>() : nullptr;

    if (SkirmishSession)
    {
        SkirmishSession->LeaveGame();
    }
}

void USkirmishPauseMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
