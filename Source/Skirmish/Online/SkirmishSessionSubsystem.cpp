#include "SkirmishSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "Engine/Engine.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "GameFramework/PlayerController.h"

namespace
{
    const FName SkirmishGameTypeKey(TEXT("SKIRMISHGAMETYPE"));
}

IOnlineIdentityPtr USkirmishSessionSubsystem::GetIdentityInterface() const
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

    if (!OnlineSubsystem)
    {
        return nullptr;
    }

    return OnlineSubsystem->GetIdentityInterface();
}

void USkirmishSessionSubsystem::Login(const FString& CredentialName)
{
    IOnlineIdentityPtr IdentityInterface = GetIdentityInterface();

    if (!IdentityInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("No online identity - is the Online Subsystem EOS plugin enabled?"));
        return;
    }

    if (IsLoggedIn())
    {
        OnLoginComplete.Broadcast(true, GetPlayerName());
        return;
    }

    FOnlineAccountCredentials Credentials;

    if (CredentialName.IsEmpty())
    {
        Credentials.Type = TEXT("accountportal");
    }
    else
    {
        Credentials.Type = TEXT("developer");
        Credentials.Id = DevAuthAddress;
        Credentials.Token = CredentialName;
    }

    LoginCompleteHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &USkirmishSessionSubsystem::HandleLoginComplete));

    IdentityInterface->Login(0, Credentials);
}

void USkirmishSessionSubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    IOnlineIdentityPtr IdentityInterface = GetIdentityInterface();

    if (IdentityInterface.IsValid())
    {
        IdentityInterface->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
    }

    const FString Message = bWasSuccessful
        ? FString::Printf(TEXT("Logged in as %s"), *GetPlayerName())
        : FString::Printf(TEXT("Login failed : %s"), *Error);

    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, bWasSuccessful ? FColor::Green : FColor::Red, Message);
    }

    OnLoginComplete.Broadcast(bWasSuccessful, GetPlayerName());

    if (bWasSuccessful)
    {
        UGameInstance* GameInstance = GetGameInstance();
        APlayerController* PlayerController = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;

        if (PlayerController)
        {
            PlayerController->SetName(GetPlayerName());
        }
    }
}

bool USkirmishSessionSubsystem::IsLoggedIn() const
{
    IOnlineIdentityPtr IdentityInterface = GetIdentityInterface();

    return IdentityInterface.IsValid() && IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

FString USkirmishSessionSubsystem::GetPlayerName() const
{
    IOnlineIdentityPtr IdentityInterface = GetIdentityInterface();

    if (!IdentityInterface.IsValid())
    {
        return FString();
    }

    return IdentityInterface->GetPlayerNickname(0);
}

IOnlineSessionPtr USkirmishSessionSubsystem::GetSessionInterface() const
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

    if (!OnlineSubsystem)
    {
        return nullptr;
    }

    return OnlineSubsystem->GetSessionInterface();
}

void USkirmishSessionSubsystem::ClearSessionThen(TFunction<void()> Next)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid() || !SessionInterface->GetNamedSession(NAME_GameSession))
    {
        Next();
        return;
    }

    AfterDestroy = Next;
    DestroySessionHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
        FOnDestroySessionCompleteDelegate::CreateUObject(this, &USkirmishSessionSubsystem::HandleDestroySessionComplete));

    if (!SessionInterface->DestroySession(NAME_GameSession))
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
        AfterDestroy = nullptr;
        Next();
    }
}

void USkirmishSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
    }

    if (AfterDestroy)
    {
        TFunction<void()> Next = MoveTemp(AfterDestroy);
        AfterDestroy = nullptr;
        Next();
    }
}

void USkirmishSessionSubsystem::HostGame(ESkirmishGameType GameType, int32 MaxPlayers)
{
    if (!GetSessionInterface().IsValid() || !IsLoggedIn())
    {
        UE_LOG(LogTemp, Error, TEXT("HostGame : log in first"));
        OnHostComplete.Broadcast(false);
        return;
    }

    ClearSessionThen([this, GameType, MaxPlayers]()
    {
        IOnlineSessionPtr SessionInterface = GetSessionInterface();

        if (!SessionInterface.IsValid())
        {
            OnHostComplete.Broadcast(false);
            return;
        }

        FOnlineSessionSettings Settings;
        Settings.NumPublicConnections = MaxPlayers;
        Settings.bShouldAdvertise = true;      // Can be found by searches
        Settings.bUsesPresence = true;         // Linked to the player's online presence
        Settings.bUseLobbiesIfAvailable = true; // EOS lobbies
        Settings.bAllowJoinInProgress = true;
        Settings.bAllowJoinViaPresence = true;
        Settings.bIsLANMatch = false;
        Settings.Set(SkirmishGameTypeKey, static_cast<int32>(GameType), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

        HostedGameType = GameType;

        CreateSessionHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
            FOnCreateSessionCompleteDelegate::CreateUObject(this, &USkirmishSessionSubsystem::HandleCreateSessionComplete));

        if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
        {
            SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
            OnHostComplete.Broadcast(false);
        }
    });
}

void USkirmishSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
    }

    UE_LOG(LogTemp, Warning, TEXT("Create session : %s"), bWasSuccessful ? TEXT("success") : TEXT("failed"));
    OnHostComplete.Broadcast(bWasSuccessful);

    if (!bWasSuccessful)
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;

    if (World)
    {
        World->ServerTravel(FString::Printf(TEXT("%s?listen"), *HostMapPath));
    }
}

void USkirmishSessionSubsystem::FindGames()
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid() || !IsLoggedIn())
    {
        UE_LOG(LogTemp, Error, TEXT("FindGames : log in first"));
        OnSessionsFound.Broadcast(TArray<FSkirmishSessionResult>());
        return;
    }

    SessionSearch = MakeShared<FOnlineSessionSearch>();
    SessionSearch->MaxSearchResults = 20;
    SessionSearch->bIsLanQuery = false;
    SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

    FindSessionsHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &USkirmishSessionSubsystem::HandleFindSessionsComplete));

    if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
        OnSessionsFound.Broadcast(TArray<FSkirmishSessionResult>());
    }
}

void USkirmishSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
    }

    TArray<FSkirmishSessionResult> Results;

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
        {
            const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[Index];

            FSkirmishSessionResult SessionResult;
            SessionResult.Index = Index;
            SessionResult.HostName = SearchResult.Session.OwningUserName;
            SessionResult.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
            SessionResult.CurrentPlayers = SessionResult.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
            SessionResult.PingMs = SearchResult.PingInMs;

            int32 GameTypeValue = 0;
            SearchResult.Session.SessionSettings.Get(SkirmishGameTypeKey, GameTypeValue);
            SessionResult.GameType = static_cast<ESkirmishGameType>(GameTypeValue);

            Results.Add(SessionResult);

            const FString Line = FString::Printf(TEXT("[%d] %s  %s  %d/%d"), Index, *SessionResult.HostName,
                SessionResult.GameType == ESkirmishGameType::Team ? TEXT("Team") : TEXT("All vs All"),
                SessionResult.CurrentPlayers, SessionResult.MaxPlayers);

            UE_LOG(LogTemp, Warning, TEXT("%s"), *Line);

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Line);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Found %d game(s)"), Results.Num());
    OnSessionsFound.Broadcast(Results);
}

void USkirmishSessionSubsystem::JoinGame(int32 Index)
{
    if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Error, TEXT("JoinGame : no search result %d"), Index);
        OnJoinComplete.Broadcast(false);
        return;
    }

    ClearSessionThen([this, Index]()
        {
            IOnlineSessionPtr SessionInterface = GetSessionInterface();

            if (!SessionInterface.IsValid() || !SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
            {
                OnJoinComplete.Broadcast(false);
                return;
            }

            JoinSessionHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
                FOnJoinSessionCompleteDelegate::CreateUObject(this, &USkirmishSessionSubsystem::HandleJoinSessionComplete));

            if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]))
            {
                SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
                OnJoinComplete.Broadcast(false);
            }
        });
}

void USkirmishSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        OnJoinComplete.Broadcast(false);
        return;
    }

    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);

    FString Address;
    const bool bJoined = Result == EOnJoinSessionCompleteResult::Success
        && SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

    UE_LOG(LogTemp, Warning, TEXT("Join session : %s %s"), bJoined ? TEXT("success") : TEXT("failed"), *Address);
    OnJoinComplete.Broadcast(bJoined);

    if (!bJoined)
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    APlayerController* PlayerController = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;

    if (PlayerController)
    {
        PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
    }
}

