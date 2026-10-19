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

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

    if (Mesh)
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RootComponent = Mesh;
    }

    Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));

    if (Muzzle)
    {
        Muzzle->SetupAttachment(RootComponent);
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
    UE_LOG(LogTemp, Warning, TEXT("[%s] MulticastFireEffects"),
        HasAuthority() ? TEXT("Server") : TEXT("Client"));

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
