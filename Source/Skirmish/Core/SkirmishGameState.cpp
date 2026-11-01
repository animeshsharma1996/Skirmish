#include "SkirmishGameState.h"
#include "SkirmishEventSubsystem.h"
#include "SkirmishPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

void ASkirmishGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASkirmishGameState, Phase);
    DOREPLIFETIME(ASkirmishGameState, RemainingSeconds);
    DOREPLIFETIME(ASkirmishGameState, Winner);
    DOREPLIFETIME(ASkirmishGameState, GameType);
    DOREPLIFETIME(ASkirmishGameState, TeamScores);
    DOREPLIFETIME(ASkirmishGameState, WinningTeam);
}

void ASkirmishGameState::SetPhase(ESkirmishMatchPhase NewPhase)
{
    if (!HasAuthority()) 
    {
        return;
    }

    Phase = NewPhase;
    HandlePhaseChanged(); // OnRep_Phase does not run on the server
}

void ASkirmishGameState::OnRep_Phase()
{
    HandlePhaseChanged();
}

void ASkirmishGameState::HandlePhaseChanged()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    if (USkirmishEventSubsystem* Events = World->GetSubsystem<USkirmishEventSubsystem>())
    {
        Events->OnPhaseChanged.Broadcast(Phase);
    }
}

void ASkirmishGameState::MulticastElimination_Implementation(const FString& KillerName, const FString& VictimName)
{
    // Network event arrives ──► publish it locally
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    if (USkirmishEventSubsystem* Events = World->GetSubsystem<USkirmishEventSubsystem>())
    {
        Events->OnElimination.Broadcast(KillerName, VictimName);
    }
}

FString ASkirmishGameState::GetTeamName(int32 TeamIndex)
{
    switch (TeamIndex)
    {
        case 0:
        {
            return TEXT("RED");
        }
        case 1:
        {
            return TEXT("BLUE");
        }
    }

    return TEXT("NONE");
}

FString ASkirmishGameState::GetPhaseText() const
{
    const FString Time = FString::Printf(TEXT("%d:%02d"), RemainingSeconds / 60, RemainingSeconds % 60);

    switch (Phase)
    {
        case ESkirmishMatchPhase::Warmup:
        {
            return FString::Printf(TEXT("WARMUP  %s"), *Time);
        }
        case ESkirmishMatchPhase::InProgress:
        {
            return Time;
        }
        case ESkirmishMatchPhase::MatchOver:
        {
            if (IsTeamGame())
            {
                return WinningTeam == INDEX_NONE ? TEXT("DRAW") : FString::Printf(TEXT("%s TEAM WINS"), *GetTeamName(WinningTeam));
            }

            return FString::Printf(TEXT("WINNER : %s"), Winner ? *Winner->GetPlayerName() : TEXT("Nobody"));
        }
    }

    return FString();
}

FString ASkirmishGameState::GetScoreboardText() const
{
    TArray<ASkirmishPlayerState*> Players;

    for (APlayerState* PlayerState : PlayerArray)
    {
        ASkirmishPlayerState* SkirmishPlayerState = Cast<ASkirmishPlayerState>(PlayerState);

        if (SkirmishPlayerState)
        {
            Players.Add(SkirmishPlayerState);
        }
    }

    Players.Sort([](const ASkirmishPlayerState& A, const ASkirmishPlayerState& B) { return A.GetKills() > B.GetKills(); });

    FString Result;

    if (IsTeamGame() && TeamScores.Num() >= 2)
    {
        Result += FString::Printf(TEXT("RED %d  -  %d BLUE\n"), TeamScores[0], TeamScores[1]);
    }

    Result += TEXT("PLAYER   K / D\n");

    for (const ASkirmishPlayerState* SkirmishPlayerState : Players)
    {
        const FString TeamTag = IsTeamGame() ? FString::Printf(TEXT("[%s] "), *GetTeamName(SkirmishPlayerState->GetTeamIndex())) : FString();

        Result += FString::Printf(TEXT("%s%s   %d / %d\n"), *TeamTag, *SkirmishPlayerState->GetPlayerName(),
            SkirmishPlayerState->GetKills(), SkirmishPlayerState->GetDeaths());
    }

    return Result;
}

