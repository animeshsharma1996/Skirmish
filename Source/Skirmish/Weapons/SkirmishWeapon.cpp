#include "SkirmishWeapon.h"
#include "../Core/SkirmishEventSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

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

