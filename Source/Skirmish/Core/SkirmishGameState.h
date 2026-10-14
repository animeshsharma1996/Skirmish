// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SkirmishGameState.generated.h"

UENUM(BlueprintType)
enum class ESkirmishMatchPhase : uint8
{
    Warmup,
    InProgress,
    MatchOver
};

UCLASS()
class SKIRMISH_API ASkirmishGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetPhase(ESkirmishMatchPhase NewPhase);

    ESkirmishMatchPhase GetPhase() const { return Phase; }

    bool CanDealDamage() const { return Phase == ESkirmishMatchPhase::InProgress; }
    bool CanFire() const { return Phase != ESkirmishMatchPhase::MatchOver; }

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 RemainingSeconds = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    TObjectPtr<APlayerState> Winner;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastElimination(const FString& KillerName, const FString& VictimName);

    UFUNCTION(BlueprintPure, Category = "Match")
    FString GetPhaseText() const;

    UFUNCTION(BlueprintPure, Category = "Match")
    FString GetScoreboardText() const;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "Match")
    ESkirmishMatchPhase Phase = ESkirmishMatchPhase::Warmup;

    UFUNCTION()
    void OnRep_Phase();

    void HandlePhaseChanged();
};
