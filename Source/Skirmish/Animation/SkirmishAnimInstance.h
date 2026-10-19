// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SkirmishAnimInstance.generated.h"

UCLASS()
class SKIRMISH_API USkirmishAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
    void TriggerRecoil();

protected:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    float AimOffsetPitch = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    float GroundSpeed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    float Direction = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    bool bShouldMove = false;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    bool bIsFalling = false;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    float AimPitch = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    bool bIsAiming = false;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    bool bHasWeapon = false;

private:
    UPROPERTY()
    TObjectPtr<APawn> OwnerPawn;
};
