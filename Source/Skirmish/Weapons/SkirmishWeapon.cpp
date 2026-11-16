#include "SkirmishWeapon.h"
#include "../Core/SkirmishEventSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "WeaponHolder.h"
#include "SkirmishBullet.h"
#include "SkirmishBulletPool.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "../SkirmishCharacter.h"
#include "../Core/SkirmishLagCompensation.h"
#include "Engine/DamageEvents.h"

ASkirmishWeapon::ASkirmishWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    AimPivot = CreateDefaultSubobject<USceneComponent>(TEXT("AimPivot"));
    AimPivot->SetupAttachment(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

    if (Mesh)
    {
        Mesh->SetupAttachment(AimPivot);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));

    if (Muzzle)
    {
        Muzzle->SetupAttachment(Mesh);
    }

    SetNetUpdateFrequency(10.f);
}

void ASkirmishWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ASkirmishWeapon, Ammo, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ASkirmishWeapon, ReserveAmmo, COND_OwnerOnly);
    DOREPLIFETIME(ASkirmishWeapon, bIsReloading);
}

void ASkirmishWeapon::BeginPlay()
{
    Super::BeginPlay();

    // Start with a full magazine
    if (HasAuthority())
    {
        Ammo = MagazineSize;
        ReserveAmmo = StartingReserveAmmo;
		NotifyAmmoChanged();
    }

    // Make this weapon's bullets now, not in the middle of a fight
    USkirmishBulletPool* BulletPool = GetWorld() ? GetWorld()->GetSubsystem<USkirmishBulletPool>() : nullptr;

    if (BulletPool)
    {
        BulletPool->Prewarm(BulletClass, BulletPoolSize);
    }
}

void ASkirmishWeapon::OnRep_Ammo()
{
	NotifyAmmoChanged();
}

void ASkirmishWeapon::NotifyAmmoChanged() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    const IWeaponHolder* WeaponHolder = Cast<IWeaponHolder>(OwnerPawn);
    if (WeaponHolder && WeaponHolder->GetCurrentWeapon() != this)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    USkirmishEventSubsystem* SkirmishEvent = World->GetSubsystem<USkirmishEventSubsystem>();
    if (SkirmishEvent)
    {
        SkirmishEvent->OnLocalAmmoChanged.Broadcast(Ammo, ReserveAmmo, bIsReloading);
    }
}

bool ASkirmishWeapon::CanFire() const
{
    const UWorld* World = GetWorld();

    if (!World || bIsReloading)
    {
        return false;
    }

    return Ammo > 0 && World->GetTimeSeconds() - LastFireTime >= GetTimeBetweenShots();
}

void ASkirmishWeapon::Fire(const FVector& AimTarget, const FVector& ShotDirection, bool bAiming, int32 ShotId)
{
    UWorld* World = GetWorld();

    if (!HasAuthority() || !World || !Muzzle || !CanFire())
    {
        return;
    }

    ConsumeAmmo();

    const FVector Start = Muzzle->GetComponentLocation();
    const FVector AimDirection = (AimTarget - Start).GetSafeNormal();

    // Never trust the client : its direction must be inside this weapon's spread (plus the most a burst can add)
    const float Spread = (bAiming ? AimSpread : HipSpread) + MaxBurstSpread;
    const float Dot = FMath::Clamp(FVector::DotProduct(AimDirection, ShotDirection.GetSafeNormal()), -1.f, 1.f);
    const float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
    const FVector Direction = Angle <= Spread + 1.f ? ShotDirection.GetSafeNormal() : AimDirection;

    // No server bullet : the shooter's bullet reports its hits, and the server rewinds to check them
    USkirmishLagCompensation* LagCompensation = GetOwner() ? GetOwner()->FindComponentByClass<USkirmishLagCompensation>() : nullptr;

    if (LagCompensation)
    {
        LagCompensation->RecordShot(ShotId, Start, Direction, this);
    }

    MulticastFireEffects(Direction);
}

void ASkirmishWeapon::ApplyConfirmedHit(ASkirmishCharacter* HitCharacter, FName BoneName, const FVector& HitLocation, const FVector& ShotDirection)
{
    if (!HitCharacter)
    {
        return;
    }

    // Point damage carries the bone, so TakeDamage can pick the hit zone
    FHitResult Hit(HitCharacter, HitCharacter->GetDisplayMesh(), HitLocation, -ShotDirection);
    Hit.BoneName = BoneName;

    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    UGameplayStatics::ApplyPointDamage(HitCharacter, Damage, ShotDirection, Hit, OwnerPawn ? OwnerPawn->GetController() : nullptr, this, nullptr);

    OnWeaponHit.Broadcast(HitCharacter, HitLocation);
}

float ASkirmishWeapon::GetBulletSpeed() const
{
    const ASkirmishBullet* DefaultBullet = BulletClass ? BulletClass->GetDefaultObject<ASkirmishBullet>() : nullptr;
    return DefaultBullet ? DefaultBullet->GetSpeed() : 0.f;
}

float ASkirmishWeapon::GetDamageMultiplier(ESkirmishHitZone HitZone) const
{
    if (HitZone == ESkirmishHitZone::Head)
    {
        return HeadMultiplier;
    }

    if (HitZone == ESkirmishHitZone::Legs)
    {
        return LegsMultiplier;
    }

    return 1.f;
}

void ASkirmishWeapon::MulticastFireEffects_Implementation(FVector_NetQuantizeNormal ShotDirection)
{
    // The shooter already showed this shot when they fired
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());

    if (OwnerPawn && OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    // Everyone else's copy of the shot never reports hits
    PlayFireEffects(ShotDirection, INDEX_NONE);
}

FRotator ASkirmishWeapon::AddRecoil()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return FRotator::ZeroRotator;
    }

    if (IsBurstOver())
    {
        ShotsInBurst = 0;
    }

    ShotsInBurst++;
    LastLocalShotTime = World->GetTimeSeconds();

    if (ShotsInBurst == 1)
    {
        return FRotator::ZeroRotator;
    }

    return FRotator(RecoilPitch, FMath::FRandRange(-RecoilYaw, RecoilYaw), 0.f);
}

bool ASkirmishWeapon::IsBurstOver() const
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        return true;
    }

    return World->GetTimeSeconds() - LastLocalShotTime > RecoilResetTime;
}

float ASkirmishWeapon::GetBurstSpread() const
{
    const int32 ExtraShots = FMath::Max(ShotsInBurst - 1, 0);
    return FMath::Min(ExtraShots * SpreadPerShot, MaxBurstSpread);
}

FVector ASkirmishWeapon::GetShotDirection(const FVector& AimTarget, bool bAiming) const
{
    if (!Muzzle)
    {
        return GetActorForwardVector();
    }

    const FVector AimDirection = (AimTarget - Muzzle->GetComponentLocation()).GetSafeNormal();

    // The first shot uses the weapon's tightest spread - every shot in a burst adds a little
    const float Spread = (bAiming ? AimSpread : HipSpread) + GetBurstSpread();

    return FMath::VRandCone(AimDirection, FMath::DegreesToRadians(Spread));
}

void ASkirmishWeapon::PlayLocalFireEffects(const FVector& ShotDirection, int32 ShotId)
{
    PlayFireEffects(ShotDirection, ShotId);

    // The camera shake is only for the shooter
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;

    if (PlayerController && FireCameraShake)
    {
        PlayerController->ClientStartCameraShake(FireCameraShake, RecoilShakeScale);
    }
}

void ASkirmishWeapon::PlayFireEffects(const FVector& ShotDirection, int32 ShotId)
{
    // A dedicated server has no screen or speakers
    if (GetNetMode() == NM_DedicatedServer || !Muzzle)
    {
        return;
    }

    const FVector Start = Muzzle->GetComponentLocation();

    if (MuzzleFlash)
    {
        // Pooled by Niagara, like the impacts
        UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlash, Muzzle, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, true, ENCPoolMethod::AutoRelease);
    }

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
    }

    if (bShowTracer)
    {
        SpawnTracer(Start, ShotDirection);
    }

    SpawnBullet(Start, ShotDirection, ShotId);
    OnWeaponFired.Broadcast();
}

void ASkirmishWeapon::SpawnBullet(const FVector& Start, const FVector& Direction, int32 ShotId)
{
    UWorld* World = GetWorld();
    USkirmishBulletPool* BulletPool = World ? World->GetSubsystem<USkirmishBulletPool>() : nullptr;

    if (!BulletPool)
    {
        return;
    }

    // Take a sleeping bullet instead of spawning one
    ASkirmishBullet* SkirmishBullet = BulletPool->Acquire(BulletClass);

    if (!SkirmishBullet)
    {
       return;
    }

    SkirmishBullet->Fire(Start, Direction, this, Cast<APawn>(GetOwner()), ShotId);
}

void ASkirmishWeapon::SpawnTracer(const FVector& Start, const FVector& Direction)
{
    UWorld* World = GetWorld();

    if (!TracerEffect || !World)
    {
        return;
    }

    // Find where this shot stops, so the streak never flies through a wall
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(GetOwner());

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, Start + Direction * Range, ECC_Weapon, Params);
    const float Distance = bHit ? Hit.Distance : Range;

    // Too close for the streak to fit - the impact is enough
    if (Distance <= TracerLength)
    {
        return;
    }

    // The tail starts at the muzzle, and the streak lives until its head reaches the hit
    const FVector SpawnLocation = Start + Direction * (TracerLength * 0.5f);
    const float Lifetime = (Distance - TracerLength) / FMath::Max(TracerSpeed, 1.f);

    UNiagaraComponent* Tracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TracerEffect, SpawnLocation, Direction.Rotation(), FVector(1.f), true, false, ENCPoolMethod::AutoRelease);

    if (!Tracer)
    {
        return;
    }

    Tracer->SetVariableFloat(TEXT("TracerLifetime"), Lifetime);
    Tracer->Activate(true);
}

void ASkirmishWeapon::ConsumeAmmo()
{
    UWorld* World = GetWorld();

    if (!HasAuthority() || !World)
    {
        return;
    }

    Ammo = FMath::Max(Ammo - 1, 0);
    LastFireTime = World->GetTimeSeconds();
    NotifyAmmoChanged();
}

void ASkirmishWeapon::AddAmmo(int32 Amount)
{
    if (!HasAuthority())
    {
        return;
    }

    ReserveAmmo = FMath::Min(ReserveAmmo + Amount, MaxReserveAmmo);
    NotifyAmmoChanged();
}

bool ASkirmishWeapon::CanReload() const
{
    return !bIsReloading && Ammo < MagazineSize && ReserveAmmo > 0;
}

void ASkirmishWeapon::StartReload()
{
    UWorld* World = GetWorld();
    if (!HasAuthority() || !World || !CanReload())
    {
        return;
    }

    bIsReloading = true;
    NotifyAmmoChanged();
    OnReloadStarted.Broadcast(); // server does not get OnRep_IsReloading

    World->GetTimerManager().SetTimer(ReloadTimer, this, &ASkirmishWeapon::FinishReload, ReloadTime, false);
}

void ASkirmishWeapon::FinishReload()
{
    const int32 Needed = MagazineSize - Ammo;
    const int32 Taken = FMath::Min(Needed, ReserveAmmo);

    Ammo += Taken;
    ReserveAmmo -= Taken;
    bIsReloading = false;
    NotifyAmmoChanged();
}

void ASkirmishWeapon::CancelReload()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(ReloadTimer);
    }

    bIsReloading = false;
    NotifyAmmoChanged();
}

void ASkirmishWeapon::OnRep_ReserveAmmo()
{
    NotifyAmmoChanged();
}

void ASkirmishWeapon::OnRep_IsReloading()
{
    NotifyAmmoChanged();

    if (bIsReloading)
    {
        OnReloadStarted.Broadcast();
    }
}

void ASkirmishWeapon::UpdateAimCorrection(const FVector& AimTarget, float DeltaSeconds)
{
    USceneComponent* Hand = Root ? Root->GetAttachParent() : nullptr;

    if (!Hand || !AimPivot || !Mesh || !Muzzle)
    {
        return;
    }

    const FTransform HandTransform = Hand->GetSocketTransform(Root->GetAttachSocketName());
    const FTransform MuzzleTransform = Muzzle->GetRelativeTransform() * Mesh->GetRelativeTransform() * HandTransform;
    const FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector();
    const FVector ToTarget = (AimTarget - MuzzleTransform.GetLocation()).GetSafeNormal();

    FQuat AimCorrection = FQuat::FindBetweenNormals(MuzzleForward, ToTarget);
    const float Angle = FMath::RadiansToDegrees(AimCorrection.GetAngle());

    const bool bCanCorrect = Angle <= MaxAimCorrection;
    AimCorrectionAlpha = FMath::FInterpTo(AimCorrectionAlpha, bCanCorrect ? 1.f : 0.f, DeltaSeconds, AimCorrectionFadeSpeed);

    const float Limit = Angle > MaxAimCorrection ? MaxAimCorrection / Angle : 1.f;
    AimCorrection = FQuat::Slerp(FQuat::Identity, AimCorrection, Limit * AimCorrectionAlpha);

    const FQuat HandRotation = HandTransform.GetRotation();
    AimPivot->SetRelativeLocationAndRotation(FVector::ZeroVector, HandRotation.Inverse() * AimCorrection * HandRotation);
}

bool ASkirmishWeapon::GetLeftHandTransform(FTransform& OutTransform) const
{
    if (!bUseLeftHandIK || !Mesh || !Mesh->DoesSocketExist(LeftHandSocketName))
    {
        return false;
    }

    OutTransform = Mesh->GetSocketTransform(LeftHandSocketName);
    return true;
}

bool ASkirmishWeapon::GetLeftHandGripInPivot(FTransform& OutTransform) const
{
    if (!bUseLeftHandIK || !Mesh || !Mesh->DoesSocketExist(LeftHandSocketName))
    {
        return false;
    }

    // The socket inside the Mesh, then the Mesh inside the pivot
    OutTransform = Mesh->GetSocketTransform(LeftHandSocketName, RTS_Component) * Mesh->GetRelativeTransform();
    OutTransform.SetScale3D(FVector::OneVector);
    return true;
}

