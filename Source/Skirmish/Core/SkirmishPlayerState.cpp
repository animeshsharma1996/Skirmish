// Fill out your copyright notice in the Description page of Project Settings.

#include "SkirmishPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "SkirmishEventSubsystem.h"
#include "Engine/Engine.h"

void ASkirmishPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASkirmishPlayerState, bIsReady);
    DOREPLIFETIME(ASkirmishPlayerState, Kills);
    DOREPLIFETIME(ASkirmishPlayerState, Deaths);
    DOREPLIFETIME(ASkirmishPlayerState, SkinMaterial);
    DOREPLIFETIME(ASkirmishPlayerState, TeamIndex);
}

void ASkirmishPlayerState::SetReady(bool bNewReady)
{
    if (!HasAuthority())
    {
        return;
    }

    bIsReady = bNewReady;
    HandleReadyChanged(); // OnRep_IsReady does not run on the server
}

void ASkirmishPlayerState::OnRep_IsReady()
{
    HandleReadyChanged();
}

void ASkirmishPlayerState::HandleReadyChanged()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
            FString::Printf(TEXT("%s is %s"), *GetPlayerName(), bIsReady ? TEXT("READY") : TEXT("not ready")));
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    USkirmishEventSubsystem* SkirmishEvent = World->GetSubsystem<USkirmishEventSubsystem>();

    if (SkirmishEvent)
    {
        SkirmishEvent->OnLobbyChanged.Broadcast();
    }
}
