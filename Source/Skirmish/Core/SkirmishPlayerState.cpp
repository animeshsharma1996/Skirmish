// Fill out your copyright notice in the Description page of Project Settings.

#include "SkirmishPlayerState.h"
#include "Net/UnrealNetwork.h"

void ASkirmishPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASkirmishPlayerState, Kills);
    DOREPLIFETIME(ASkirmishPlayerState, Deaths);
    DOREPLIFETIME(ASkirmishPlayerState, SkinMaterial);
}