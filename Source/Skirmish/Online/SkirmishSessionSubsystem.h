#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Core/SkirmishTypes.h"
#include "SkirmishSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkirmishLoginComplete, bool, bWasSuccessful, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkirmishSessionsFound, const TArray<FSkirmishSessionResult>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkirmishSessionAction, bool, bWasSuccessful);

/** Every online feature of Skirmish : login, hosting, finding and joining games */
UCLASS()
class SKIRMISH_API USkirmishSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Online")
    FOnSkirmishSessionsFound OnSessionsFound;

    UPROPERTY(BlueprintAssignable, Category = "Online")
    FOnSkirmishSessionAction OnHostComplete;

    UPROPERTY(BlueprintAssignable, Category = "Online")
    FOnSkirmishSessionAction OnJoinComplete;

    UPROPERTY(BlueprintAssignable, Category = "Online")
    FOnSkirmishLoginComplete OnLoginComplete;

    /** Developer Authentication Tool login, or the Epic account web page when CredentialName is empty */
    UFUNCTION(BlueprintCallable, Category = "Online")
    void Login(const FString& CredentialName);

    UFUNCTION(BlueprintPure, Category = "Online")
    bool IsLoggedIn() const;

    UFUNCTION(BlueprintPure, Category = "Online")
    FString GetPlayerName() const;

    UFUNCTION(BlueprintCallable, Category = "Online")
    void HostGame(ESkirmishGameType GameType, int32 MaxPlayers = 8);

    UFUNCTION(BlueprintCallable, Category = "Online")
    void FindGames();

    UFUNCTION(BlueprintCallable, Category = "Online")
    void FindServers();

    UFUNCTION(BlueprintCallable, Category = "Online")
    void JoinGame(int32 Index);

    UFUNCTION(BlueprintPure, Category = "Online")
    ESkirmishGameType GetHostedGameType() const { return HostedGameType; }

    UFUNCTION(BlueprintCallable, Category = "Online")
    void QuickMatch(ESkirmishGameType GameType);

    UFUNCTION(BlueprintCallable, Category = "Online")
    void LeaveGame();

    void RegisterDedicatedServer(ESkirmishGameType GameType, int32 MaxPlayers);

private:
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    TFunction<void()> AfterDestroy;
    bool bQuickMatchPending = false;
    bool bQuickMatchServers = false;

    ESkirmishGameType HostedGameType = ESkirmishGameType::AllVsAll;
    ESkirmishGameType QuickMatchGameType = ESkirmishGameType::AllVsAll;

    FDelegateHandle LoginCompleteHandle;
    FDelegateHandle DestroySessionHandle;
    FDelegateHandle CreateSessionHandle;
    FDelegateHandle FindSessionsHandle;
    FDelegateHandle JoinSessionHandle;

    const FString DevAuthAddress = TEXT("localhost:6300");
    const FString HostMapPath = TEXT("/Game/Maps/Lvl_Lobby");

    IOnlineIdentityPtr GetIdentityInterface() const;
    IOnlineSessionPtr GetSessionInterface() const;

    void StartSearch(bool bFilterGameType, ESkirmishGameType GameType, bool bDedicatedServers = false);
    void FinishQuickMatch();
    void ClearSessionThen(TFunction<void()> Next);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
};
