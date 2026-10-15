#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SkirmishGameState.h"
#include "SkirmishEventSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEliminationEvent, const FString&, KillerName, const FString&, VictimName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedEvent, ESkirmishMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalAmmoChangedEvent, int32, Ammo);

UCLASS()
class SKIRMISH_API USkirmishEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEliminationEvent OnElimination;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPhaseChangedEvent OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnLocalAmmoChangedEvent OnLocalAmmoChanged;
};
