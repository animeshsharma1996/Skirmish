#include "SkirmishMainMenuWidget.h"
#include "SkirmishSessionEntryWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Online/SkirmishSessionSubsystem.h"
#include "Core/SkirmishPlayerSettings.h"

void USkirmishMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (GameTypeBox)
    {
        GameTypeBox->ClearOptions();
        GameTypeBox->AddOption(TEXT("All vs All"));
        GameTypeBox->AddOption(TEXT("Team"));
        GameTypeBox->SetSelectedIndex(0);
    }

    if (CameraModeBox)
    {
        CameraModeBox->ClearOptions();
        CameraModeBox->AddOption(TEXT("Third Person"));
        CameraModeBox->AddOption(TEXT("First Person"));
        CameraModeBox->SetSelectedIndex(0);
        CameraModeBox->OnSelectionChanged.AddDynamic(this, &USkirmishMainMenuWidget::HandleCameraModeChanged);
    }

    if (LoginButton)
    {
        LoginButton->OnClicked.AddDynamic(this, &USkirmishMainMenuWidget::HandleLoginClicked);
    }

    if (HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &USkirmishMainMenuWidget::HandleHostClicked);
    }

    if (QuickMatchButton)
    {
        QuickMatchButton->OnClicked.AddDynamic(this, &USkirmishMainMenuWidget::HandleQuickMatchClicked);
    }

    if (FindGamesButton)
    {
        FindGamesButton->OnClicked.AddDynamic(this, &USkirmishMainMenuWidget::HandleFindGamesClicked);
    }

    if (FindServersButton)
    {
        FindServersButton->OnClicked.AddDynamic(this, &USkirmishMainMenuWidget::HandleFindServersClicked);
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &USkirmishMainMenuWidget::HandleQuitClicked);
    }

    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (!SkirmishSession)
    {
        return;
    }

    SkirmishSession->OnLoginComplete.AddDynamic(this, &USkirmishMainMenuWidget::HandleLoginComplete);
    SkirmishSession->OnSessionsFound.AddDynamic(this, &USkirmishMainMenuWidget::HandleSessionsFound);
    SkirmishSession->OnHostComplete.AddDynamic(this, &USkirmishMainMenuWidget::HandleHostComplete);
    SkirmishSession->OnJoinComplete.AddDynamic(this, &USkirmishMainMenuWidget::HandleJoinComplete);

    // Already logged in from an earlier visit to the menu
    if (SkirmishSession->IsLoggedIn())
    {
        HandleLoginComplete(true, SkirmishSession->GetPlayerName());
    }
    else
    {
        SetStatus(TEXT("Log in to play"));
    }
}

void USkirmishMainMenuWidget::NativeDestruct()
{
    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (SkirmishSession)
    {
        SkirmishSession->OnLoginComplete.RemoveDynamic(this, &USkirmishMainMenuWidget::HandleLoginComplete);
        SkirmishSession->OnSessionsFound.RemoveDynamic(this, &USkirmishMainMenuWidget::HandleSessionsFound);
        SkirmishSession->OnHostComplete.RemoveDynamic(this, &USkirmishMainMenuWidget::HandleHostComplete);
        SkirmishSession->OnJoinComplete.RemoveDynamic(this, &USkirmishMainMenuWidget::HandleJoinComplete);
    }

    Super::NativeDestruct();
}

USkirmishSessionSubsystem* USkirmishMainMenuWidget::GetSkirmishSession() const
{
    UGameInstance* GameInstance = GetGameInstance();

    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<USkirmishSessionSubsystem>();
}

ESkirmishGameType USkirmishMainMenuWidget::GetSelectedGameType() const
{
    if (GameTypeBox && GameTypeBox->GetSelectedIndex() == 1)
    {
        return ESkirmishGameType::Team;
    }

    return ESkirmishGameType::AllVsAll;
}

void USkirmishMainMenuWidget::SetStatus(const FString& Message)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Message));
    }
}

void USkirmishMainMenuWidget::HandleLoginClicked()
{
    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (!SkirmishSession)
    {
        return;
    }

    // Empty : the Epic account page. A name : the Developer Authentication Tool
    const FString CredentialName = CredentialBox ? CredentialBox->GetText().ToString() : FString();

    SetStatus(TEXT("Logging in..."));
    SkirmishSession->Login(CredentialName);
}

void USkirmishMainMenuWidget::HandleHostClicked()
{
    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (SkirmishSession)
    {
        SetStatus(TEXT("Hosting..."));
        SkirmishSession->HostGame(GetSelectedGameType());
    }
}

void USkirmishMainMenuWidget::HandleQuickMatchClicked()
{
    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (SkirmishSession)
    {
        SetStatus(TEXT("Looking for a game..."));
        SkirmishSession->QuickMatch(GetSelectedGameType());
    }
}

void USkirmishMainMenuWidget::HandleFindGamesClicked()
{
    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (SkirmishSession)
    {
        SetStatus(TEXT("Searching..."));
        SkirmishSession->FindGames();
    }
}

void USkirmishMainMenuWidget::HandleFindServersClicked()
{
    USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

    if (SkirmishSession)
    {
        SetStatus(TEXT("Searching for servers..."));
        SkirmishSession->FindServers();
    }
}

void USkirmishMainMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void USkirmishMainMenuWidget::HandleCameraModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    UGameInstance* GameInstance = GetGameInstance();
    USkirmishPlayerSettings* SkirmishPlayerSettings = GameInstance ? GameInstance->GetSubsystem<USkirmishPlayerSettings>() : nullptr;

    if (!SkirmishPlayerSettings || !CameraModeBox)
    {
        return;
    }

    const bool bFirstPerson = CameraModeBox->GetSelectedIndex() == 1;
    SkirmishPlayerSettings->SetStartCameraMode(bFirstPerson ? ESkirmishCameraMode::FirstPerson : ESkirmishCameraMode::ThirdPerson);
}

void USkirmishMainMenuWidget::HandleLoginComplete(bool bWasSuccessful, const FString& PlayerName)
{
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(bWasSuccessful ? PlayerName : TEXT("Not logged in")));
    }

    SetStatus(bWasSuccessful ? TEXT("Ready to play") : TEXT("Login failed"));
}

void USkirmishMainMenuWidget::HandleSessionsFound(const TArray<FSkirmishSessionResult>& Results)
{
    if (!SessionList)
    {
        return;
    }

    SessionList->ClearChildren();

    for (const FSkirmishSessionResult& Result : Results)
    {
        USkirmishSessionEntryWidget* SessionEntry = CreateWidget<USkirmishSessionEntryWidget>(this, SessionEntryClass);

        if (SessionEntry)
        {
            SessionEntry->Setup(Result);
            SessionList->AddChild(SessionEntry);
        }
    }

    SetStatus(FString::Printf(TEXT("Found %d game(s)"), Results.Num()));
}

void USkirmishMainMenuWidget::HandleHostComplete(bool bWasSuccessful)
{
    SetStatus(bWasSuccessful ? TEXT("Opening the lobby...") : TEXT("Could not host"));
}

void USkirmishMainMenuWidget::HandleJoinComplete(bool bWasSuccessful)
{
    SetStatus(bWasSuccessful ? TEXT("Joining...") : TEXT("Could not join"));
}
