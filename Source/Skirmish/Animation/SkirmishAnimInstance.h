// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SkirmishAnimInstance.generated.h"

class IWeaponHolder;
class UBlendSpace;

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

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Animation")
    TObjectPtr<UBlendSpace> DefaultMovementBlendSpace; /** Used when the weapon does not set its own */

    UPROPERTY(EditDefaultsOnly, Category = "Weapon Animation")
    TObjectPtr<UBlendSpace> DefaultAimOffset;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon Animation")
    TObjectPtr<UBlendSpace> MovementBlendSpace;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon Animation")
    TObjectPtr<UBlendSpace> AimOffset;

    UPROPERTY(EditDefaultsOnly, Category = "Hand IK")
    FName RightHandBone = TEXT("hand_r"); /** The bone that holds the weapon - change it for skeletons with different names */

    UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
    FTransform LeftHandTransform; /** Where the left hand goes, relative to the right hand */

    UPROPERTY(BlueprintReadOnly, Category = "Hand IK")
    float LeftHandIKAlpha = 0.f;

    void UpdateWeaponAnimation(const IWeaponHolder* WeaponHolder);
    void UpdateHandIK(const IWeaponHolder* WeaponHolder, float DeltaSeconds);

private:
    UPROPERTY()
    TObjectPtr<APawn> OwnerPawn;
};
