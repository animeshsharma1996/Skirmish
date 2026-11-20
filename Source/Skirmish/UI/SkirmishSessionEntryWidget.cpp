#include "SkirmishSessionEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Online/SkirmishSessionSubsystem.h"

void USkirmishSessionEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &USkirmishSessionEntryWidget::HandleJoinClicked);
    }
}

void USkirmishSessionEntryWidget::Setup(const FSkirmishSessionResult& Result)
{
    SessionIndex = Result.Index;

    if (HostNameText)
    {
        HostNameText->SetText(FText::FromString(Result.HostName));
    }

    if (GameTypeText)
    {
        GameTypeText->SetText(FText::FromString(Result.GameType == ESkirmishGameType::Team ? TEXT("Team") : TEXT("All vs All")));
    }

    if (PlayersText)
    {
        PlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Result.CurrentPlayers, Result.MaxPlayers)));
    }

    if (PingText)
    {
        PingText->SetText(FText::FromString(FString::Printf(TEXT("%d ms"), Result.PingMs)));
    }
}

void USkirmishSessionEntryWidget::HandleJoinClicked()
{
    UGameInstance* GameInstance = GetGameInstance();
    USkirmishSessionSubsystem* SkirmishSession = GameInstance ? GameInstance->GetSubsystem<USkirmishSessionSubsystem>() : nullptr;

    if (SkirmishSession)
    {
        SkirmishSession->JoinGame(SessionIndex);
    }
}
