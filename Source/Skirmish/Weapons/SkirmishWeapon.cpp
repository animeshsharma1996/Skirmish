#include "SkirmishWeapon.h"
#include "../Core/SkirmishEventSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

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
}

void ASkirmishWeapon::BeginPlay()
{
    Super::BeginPlay();

    // Start with a full magazine
    if (HasAuthority())
    {
        Ammo = MagazineSize;
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

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    if (USkirmishEventSubsystem* Events = World->GetSubsystem<USkirmishEventSubsystem>())
    {
        Events->OnLocalAmmoChanged.Broadcast(Ammo);
    }

}

bool ASkirmishWeapon::CanFire() const
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    return Ammo > 0 && World->GetTimeSeconds() - LastFireTime >= GetTimeBetweenShots();
}

void ASkirmishWeapon::Fire(const FVector& AimTarget, bool bAiming)
{
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

    Ammo = FMath::Min(Ammo + Amount, MagazineSize);
    NotifyAmmoChanged();
}

