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
    float AimPitch = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    float SpinePitch = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    bool bIsAiming = false;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    bool bHasWeapon = false;

    UPROPERTY(BlueprintReadOnly, Category = "Skirmish")
    float RecoilAlpha = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "Skirmish")
    float AimBendAmount = 0.8f;

    UPROPERTY(EditDefaultsOnly, Category = "Skirmish")
    float RecoilKick = 4.f;

    UPROPERTY(EditDefaultsOnly, Category = "Skirmish")
    float RecoilRecoverySpeed = 12.f;

private:
    UPROPERTY()
    TObjectPtr<APawn> OwnerPawn;
};
