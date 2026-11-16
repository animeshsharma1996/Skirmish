#include "SkirmishAnimInstance.h"
#include "../Weapons/WeaponHolder.h"
#include "../Weapons/SkirmishWeapon.h"
#include "Animation/BlendSpace.h"
#include "Engine/SkeletalMeshSocket.h"
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
        bIsFirstPerson = WeaponHolder->IsFirstPerson();
        UpdateWeaponAnimation(WeaponHolder);
        UpdateHandIK(WeaponHolder, DeltaSeconds);
    }
}

void USkirmishAnimInstance::UpdateWeaponAnimation(const IWeaponHolder* WeaponHolder)
{
    MovementBlendSpace = DefaultMovementBlendSpace;
    AimOffset = DefaultAimOffset;

    ASkirmishWeapon* Weapon = WeaponHolder ? WeaponHolder->GetCurrentWeapon() : nullptr;

    if (!Weapon)
    {
        return;
    }

    if (Weapon->GetMovementBlendSpace())
    {
        MovementBlendSpace = Weapon->GetMovementBlendSpace();
    }

    if (Weapon->GetAimOffset())
    {
        AimOffset = Weapon->GetAimOffset();
    }
}

void USkirmishAnimInstance::UpdateHandIK(const IWeaponHolder* WeaponHolder, float DeltaSeconds)
{
    float TargetLeftAlpha = 0.f;

    ASkirmishWeapon* Weapon = WeaponHolder ? WeaponHolder->GetCurrentWeapon() : nullptr;
    USkeletalMeshComponent* SkeletalMesh = GetSkelMeshComponent();
    USceneComponent* WeaponRoot = Weapon ? Weapon->GetRootComponent() : nullptr;

    // Only the mesh that actually holds the weapon reaches for it
    const bool bHoldsWeapon = WeaponRoot && SkeletalMesh && WeaponRoot->GetAttachParent() == SkeletalMesh;

    if (bHoldsWeapon)
    {
        // Where the weapon socket sits on the right hand
        const USkeletalMeshSocket* HandSocket = SkeletalMesh->GetSocketByName(WeaponRoot->GetAttachSocketName());
        const FTransform SocketInHand = HandSocket ? HandSocket->GetSocketLocalTransform() : FTransform::Identity;

        FTransform GripInPivot;

        if (!Weapon->IsReloading() && Weapon->GetLeftHandGripInPivot(GripInPivot))
        {
            // The grip in the pivot, the pivot on the socket, the socket on the right hand - the same in both views
            LeftHandTransform = GripInPivot * Weapon->GetAimPivotRelativeTransform() * SocketInHand;
            TargetLeftAlpha = 1.f;
        }
    }

    // The left hand blends on and off the grip (reloads)
    LeftHandIKAlpha = FMath::FInterpTo(LeftHandIKAlpha, TargetLeftAlpha, DeltaSeconds, 15.f);
}
