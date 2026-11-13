#include "SkirmishWeapon.h"
#include "../Core/SkirmishEventSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "WeaponHolder.h"

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

    FirstPersonView = CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonView"));

    if (FirstPersonView)
    {
        FirstPersonView->SetupAttachment(Mesh);
    }
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

void ASkirmishWeapon::Fire(const FVector& AimTarget, bool bAiming)
{
    UE_LOG(LogTemp, Warning, TEXT("Weapon Fire: Ammo=%d CanFire=%d"), Ammo, CanFire() ? 1 : 0);

    UWorld* World = GetWorld();
    if (!HasAuthority() || !World || !Muzzle || !CanFire())
    {
        return;
    }

    ConsumeAmmo();

    const FVector Start = Muzzle->GetComponentLocation();
    const FVector AimDirection = (AimTarget - Start).GetSafeNormal();
    const float Spread = bAiming ? AimSpread : HipSpread;
    const FVector ShotDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(Spread));
    const FVector End = Start + ShotDirection * Range;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(GetOwner());

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Weapon, Params);

    AActor* HitActor = Hit.GetActor();
    if (bHit && HitActor)
    {
        APawn* OwnerPawn = Cast<APawn>(GetOwner());
        AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

        UGameplayStatics::ApplyDamage(HitActor, Damage, OwnerController, this, nullptr);
        OnWeaponHit.Broadcast(HitActor, Hit.ImpactPoint);
    }

    MulticastFireEffects(bHit ? Hit.ImpactPoint : End, bHit);
}

void ASkirmishWeapon::MulticastFireEffects_Implementation(FVector_NetQuantize TraceEnd, bool bHit)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] MulticastFireEffects"), HasAuthority() ? TEXT("Server") : TEXT("Client"));

    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World || !Muzzle)
    {
        return;
    }

    const FVector Start = Muzzle->GetComponentLocation();

    DrawDebugLine(World, Start, TraceEnd, FColor::Yellow, false, 0.08f, 0, 1.5f);

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
    }

    if (bHit)
    {
        DrawDebugSphere(World, TraceEnd, 8.f, 8, FColor::Orange, false, 0.5f);

        if (ImpactSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, TraceEnd);
        }
    }

    OnWeaponFired.Broadcast();
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

    // Where the hand holds the weapon this frame
    const FTransform HandTransform = Hand->GetSocketTransform(Root->GetAttachSocketName());

    // Where the muzzle would point without any correction
    const FTransform MuzzleTransform = Muzzle->GetRelativeTransform() * Mesh->GetRelativeTransform() * HandTransform;
    const FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector();
    const FVector ToTarget = (AimTarget - MuzzleTransform.GetLocation()).GetSafeNormal();

    FQuat AimCorrection = FQuat::FindBetweenNormals(MuzzleForward, ToTarget);
    const float Angle = FMath::RadiansToDegrees(AimCorrection.GetAngle());

    // A big turn means the character faces away (running in third person) - fade the correction out
    const bool bCanCorrect = Angle <= MaxAimCorrection;
    AimCorrectionAlpha = FMath::FInterpTo(AimCorrectionAlpha, bCanCorrect ? 1.f : 0.f, DeltaSeconds, AimCorrectionFadeSpeed);

    // Never turn more than MaxAimCorrection, and never lag behind the target - so the weapon never drifts
    const float Limit = Angle > MaxAimCorrection ? MaxAimCorrection / Angle : 1.f;
    AimCorrection = FQuat::Slerp(FQuat::Identity, AimCorrection, Limit * AimCorrectionAlpha);

    // Turn around the hand : the pivot's world rotation becomes AimCorrection * HandRotation
    const FQuat HandRotation = HandTransform.GetRotation();
    AimPivot->SetRelativeLocationAndRotation(FVector::ZeroVector, HandRotation.Inverse() * AimCorrection * HandRotation);
}

void ASkirmishWeapon::UpdateFirstPersonView(const FTransform& CameraTransform)
{
    if (!AimPivot || !Mesh || !FirstPersonView || !Muzzle)
    {
        return;
    }

    // Where the eye sits inside the pivot, looking the same way as the barrel (the Muzzle)
    const FTransform MeshInPivot = Mesh->GetRelativeTransform();
    const FVector ViewLocation = (FirstPersonView->GetRelativeTransform() * MeshInPivot).GetLocation();
    const FQuat ViewRotation = (Muzzle->GetRelativeTransform() * MeshInPivot).GetRotation();
    const FTransform ViewInPivot(ViewRotation, ViewLocation);

    // Place the pivot so the view point lands exactly on the camera
    AimPivot->SetWorldTransform(ViewInPivot.Inverse() * CameraTransform);
}

