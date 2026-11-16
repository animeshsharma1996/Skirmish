#include "SkirmishLagCompensation.h"
#include "../SkirmishCharacter.h"
#include "../Weapons/SkirmishWeapon.h"
#include "GameFramework/PlayerState.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

USkirmishLagCompensation::USkirmishLagCompensation()
{
    // Record after the animation has posed the bones this frame
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;

    // Needed for ServerReportHit
    SetIsReplicatedByDefault(true);
}

void USkirmishLagCompensation::BeginPlay()
{
    Super::BeginPlay();

    const ASkirmishCharacter* SkirmishCharacter = Cast<ASkirmishCharacter>(GetOwner());
    const USkeletalMeshComponent* DisplayMesh = SkirmishCharacter ? SkirmishCharacter->GetDisplayMesh() : nullptr;
    const UPhysicsAsset* PhysicsAsset = DisplayMesh ? DisplayMesh->GetPhysicsAsset() : nullptr;

    // Only the server rewinds
    if (!GetOwner()->HasAuthority() || !PhysicsAsset)
    {
        SetComponentTickEnabled(false);
        return;
    }

    // The hitboxes are the Physics Asset's bodies - remember the bone of each
    for (const USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
    {
        if (BodySetup)
        {
            HitboxBones.Add(BodySetup->BoneName);
        }
    }
}

void USkirmishLagCompensation::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    RecordHitboxes();
}

void USkirmishLagCompensation::RecordHitboxes()
{
    const ASkirmishCharacter* SkirmishCharacter = Cast<ASkirmishCharacter>(GetOwner());
    const USkeletalMeshComponent* DisplayMesh = SkirmishCharacter ? SkirmishCharacter->GetDisplayMesh() : nullptr;
    UWorld* World = GetWorld();

    if (!DisplayMesh || !World)
    {
        return;
    }

    FSkirmishHitboxFrame Frame;
    Frame.Time = World->GetTimeSeconds();
    Frame.BoneLocations.Reserve(HitboxBones.Num());

    for (const FName& BoneName : HitboxBones)
    {
        Frame.BoneLocations.Add(DisplayMesh->GetBoneLocation(BoneName));
    }

    History.Add(MoveTemp(Frame));

    // Forget anything older than the longest rewind
    while (History.Num() > 2 && History[0].Time < World->GetTimeSeconds() - MaxRewindTime)
    {
        History.RemoveAt(0);
    }
}

bool USkirmishLagCompensation::GetRewoundBoneLocation(FName BoneName, double Time, FVector& OutLocation) const
{
    const int32 BoneIndex = HitboxBones.IndexOfByKey(BoneName);

    // Too old to check, or not a hitbox
    if (BoneIndex == INDEX_NONE || History.Num() == 0 || Time < History[0].Time)
    {
        return false;
    }

    // Newer than the last frame (the host's own shots) : use the latest pose
    if (Time >= History.Last().Time)
    {
        OutLocation = History.Last().BoneLocations[BoneIndex];
        return true;
    }

    // Find the two frames either side of that moment and blend between them
    for (int32 Index = History.Num() - 1; Index > 0; --Index)
    {
        const FSkirmishHitboxFrame& Older = History[Index - 1];
        const FSkirmishHitboxFrame& Newer = History[Index];

        if (Older.Time <= Time)
        {
            const float Alpha = static_cast<float>((Time - Older.Time) / FMath::Max(Newer.Time - Older.Time, UE_SMALL_NUMBER));
            OutLocation = FMath::Lerp(Older.BoneLocations[BoneIndex], Newer.BoneLocations[BoneIndex], Alpha);
            return true;
        }
    }

    return false;
}

void USkirmishLagCompensation::RecordShot(int32 ShotId, const FVector& Start, const FVector& Direction, ASkirmishWeapon* Weapon)
{
    UWorld* World = GetWorld();
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    const APlayerState* PlayerState = OwnerPawn ? OwnerPawn->GetPlayerState() : nullptr;

    if (!World)
    {
        return;
    }

    // The shooter sees the world about one round trip behind the server
    const float Ping = PlayerState ? PlayerState->GetPingInMilliseconds() / 1000.f : 0.f;

    FSkirmishShot Shot;
    Shot.Id = ShotId;
    Shot.RewindTime = World->GetTimeSeconds() - FMath::Min(Ping, MaxRewindTime);
    Shot.Start = Start;
    Shot.Direction = Direction.GetSafeNormal();
    Shot.Weapon = Weapon;
    Shots.Add(Shot);

    // Only recent shots can still be reported
    if (Shots.Num() > 64)
    {
        Shots.RemoveAt(0);
    }
}

void USkirmishLagCompensation::ServerReportHit_Implementation(int32 ShotId, ASkirmishCharacter* HitCharacter, FName BoneName, FVector_NetQuantize HitLocation)
{
    FSkirmishShot* Shot = Shots.FindByPredicate([ShotId](const FSkirmishShot& Recorded) { return Recorded.Id == ShotId; });

    // One hit per shot, and never yourself
    if (!Shot || Shot->bUsed || !HitCharacter || HitCharacter == GetOwner() || HitCharacter->IsDead())
    {
        return;
    }

    if (!IsHitValid(*Shot, HitCharacter, BoneName, HitLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit rejected : shot %d on %s (%s)"), ShotId, *HitCharacter->GetName(), *BoneName.ToString());
        return;
    }

    Shot->bUsed = true;

    ASkirmishWeapon* Weapon = Shot->Weapon.Get();

    if (Weapon)
    {
        Weapon->ApplyConfirmedHit(HitCharacter, BoneName, HitLocation, Shot->Direction);
    }
}

bool USkirmishLagCompensation::IsHitValid(const FSkirmishShot& Shot, ASkirmishCharacter* HitCharacter, FName BoneName, const FVector& HitLocation) const
{
    UWorld* World = GetWorld();
    const ASkirmishWeapon* Weapon = Shot.Weapon.Get();
    const USkirmishLagCompensation* TargetHistory = HitCharacter->FindComponentByClass<USkirmishLagCompensation>();

    if (!World || !Weapon || !TargetHistory)
    {
        return false;
    }

    // 1. The hit is on the line the server accepted for this shot
    const FVector ToHit = HitLocation - Shot.Start;
    const float Distance = FVector::DotProduct(ToHit, Shot.Direction);

    if (Distance < 0.f || Distance > Weapon->GetRange() || (ToHit - Shot.Direction * Distance).Size() > MaxDistanceFromShot)
    {
        return false;
    }

    // 2. No wall between the muzzle and the hit
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.AddIgnoredActor(HitCharacter);
    Params.AddIgnoredActor(Weapon);

    if (World->LineTraceTestByObjectType(Shot.Start, HitLocation, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
    {
        return false;
    }

    // 3. Rewind the target : was that bone there when the bullet arrived on the shooter's screen?
    const double HitTime = Shot.RewindTime + Distance / FMath::Max(Weapon->GetBulletSpeed(), 1.f);
    FVector BoneLocation;

    if (!TargetHistory->GetRewoundBoneLocation(BoneName, HitTime, BoneLocation))
    {
        return false;
    }

    return FVector::Dist(BoneLocation, HitLocation) <= MaxDistanceFromBone;
}
