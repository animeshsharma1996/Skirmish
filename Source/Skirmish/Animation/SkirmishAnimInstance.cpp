#include "SkirmishAnimInstance.h"
#include "../Weapons/WeaponHolder.h"
#include "GameFramework/PawnMovementComponent.h"

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

    Velocity = OwnerPawn->GetVelocity();
    GroundSpeed = Velocity.Size2D();
    bShouldMove = GroundSpeed > 3.f;

    if (bShouldMove)
    {
        const float MoveYaw = Velocity.ToOrientationRotator().Yaw;
        Direction = FRotator::NormalizeAxis(MoveYaw - OwnerPawn->GetActorRotation().Yaw);
    }

    UPawnMovementComponent* MovementComponent = OwnerPawn->GetMovementComponent();
    if (MovementComponent)
    {
        bIsFalling = MovementComponent->IsFalling();
    }

    const FRotator AimRotation = OwnerPawn->GetBaseAimRotation();
    AimPitch = FMath::Clamp(FRotator::NormalizeAxis(AimRotation.Pitch), -90.f, 90.f);
    AimOffsetPitch = AimPitch / 90.f;

    const IWeaponHolder* WeaponHolder = Cast<IWeaponHolder>(OwnerPawn);
    if (WeaponHolder)
    {
        bIsAiming = WeaponHolder->IsAiming();
        bHasWeapon = WeaponHolder->GetCurrentWeapon() != nullptr;
    }
}
