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

FString ASkirmishGameState::GetPhaseText() const
{
    const FString Time = FString::Printf(TEXT("%d:%02d"), RemainingSeconds / 60, RemainingSeconds % 60);

    switch (Phase)
    {
        case ESkirmishMatchPhase::Warmup :
        {
            return FString::Printf(TEXT("WARMUP  %s"), *Time);
        }
        case ESkirmishMatchPhase::InProgress :
        {
            return Time;
        }
        case ESkirmishMatchPhase::MatchOver :
        {
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
        if (ASkirmishPlayerState* SkirmishPlayerState = Cast<ASkirmishPlayerState>(PlayerState))
        {
            Players.Add(SkirmishPlayerState);
        }
    }

    Players.Sort([](const ASkirmishPlayerState& A, const ASkirmishPlayerState& B) { return A.GetKills() > B.GetKills(); });

    FString Result = TEXT("PLAYER   K / D\n");
    for (const ASkirmishPlayerState* SkirmishPlayerState : Players)
    {
        Result += FString::Printf(TEXT("%s   %d / %d\n"), *SkirmishPlayerState->GetPlayerName(), SkirmishPlayerState->GetKills(), SkirmishPlayerState->GetDeaths());
    }
    return Result;
}

