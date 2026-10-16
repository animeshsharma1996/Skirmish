#include "SkirmishAnimInstance.h"
#include "../Weapons/WeaponHolder.h"

void USkirmishAnimInstance::TriggerRecoil()
{
    RecoilAlpha = 1.f;
}

void USkirmishAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!OwnerPawn)
    {
        OwnerPawn = TryGetPawnOwner();
    }

    if (!OwnerPawn)
    {
        return;
    }

    const FRotator AimRotation = OwnerPawn->GetBaseAimRotation();
    AimPitch = FRotator::NormalizeAxis(AimRotation.Pitch);

    const IWeaponHolder* WeaponHolder = Cast<IWeaponHolder>(OwnerPawn);
    if (WeaponHolder)
    {
        bIsAiming = WeaponHolder->IsAiming();
        bHasWeapon = WeaponHolder->GetCurrentWeapon() != nullptr;
    }

    RecoilAlpha = FMath::FInterpTo(RecoilAlpha, 0.f, DeltaSeconds, RecoilRecoverySpeed);

    SpinePitch = FMath::Clamp(AimPitch * AimBendAmount, -60.f, 60.f) + RecoilAlpha * RecoilKick;
}
