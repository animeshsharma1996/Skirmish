#include "SkirmishLobbyGameMode.h"
#include "SkirmishPlayerState.h"
#include "Online/SkirmishSessionSubsystem.h"
#include "GameFramework/GameStateBase.h"

ASkirmishLobbyGameMode::ASkirmishLobbyGameMode()
{
    PlayerStateClass = ASkirmishPlayerState::StaticClass();

    // Keep the players connected while the map changes
    bUseSeamlessTravel = true;
}

void ASkirmishLobbyGameMode::PlayerReadyChanged()
{
    UE_LOG(LogTemp, Warning, TEXT("PlayerReadyChanged: %d players"), GameState ? GameState->PlayerArray.Num() : -1);

    if (AreAllPlayersReady())
    {
        if (!GetWorldTimerManager().IsTimerActive(StartTimer))
        {
            UE_LOG(LogTemp, Warning, TEXT("Everyone is ready - starting in %.0f seconds"), StartDelay);
            GetWorldTimerManager().SetTimer(StartTimer, this, &ASkirmishLobbyGameMode::StartMatch, StartDelay, false);
        }
    }
    else
    {
        // Someone is not ready any more (or left) - cancel the countdown
        GetWorldTimerManager().ClearTimer(StartTimer);
    }
}

void ASkirmishLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // A new player is not ready yet - this cancels a running countdown
    PlayerReadyChanged();
}

void ASkirmishLobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    // The leaving player's Player State is removed a moment later, so check on the next frame
    GetWorldTimerManager().SetTimerForNextTick(this, &ASkirmishLobbyGameMode::PlayerReadyChanged);
}

bool ASkirmishLobbyGameMode::AreAllPlayersReady() const
{
    if (!GameState || GameState->PlayerArray.Num() < MinPlayers)
    {
        return false;
    }

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        const ASkirmishPlayerState* SkirmishPlayerState = Cast<ASkirmishPlayerState>(PlayerState);

        if (!SkirmishPlayerState || !SkirmishPlayerState->IsReady())
        {
            return false;
        }
    }

    return true;
}

void ASkirmishLobbyGameMode::StartMatch()
{
    // The host chose the game type when creating the session
    ESkirmishGameType GameType = ESkirmishGameType::AllVsAll;

    UGameInstance* GameInstance = GetGameInstance();
    USkirmishSessionSubsystem* SkirmishSession = GameInstance ? GameInstance->GetSubsystem<USkirmishSessionSubsystem>() : nullptr;

    if (SkirmishSession)
    {
        GameType = SkirmishSession->GetHostedGameType();
    }

    UWorld* World = GetWorld();

    if (World)
    {
        // The game type travels to the match Game Mode as a URL option
        World->ServerTravel(FString::Printf(TEXT("%s?GameType=%d"), *MatchMapPath, static_cast<int32>(GameType)));
    }
}
