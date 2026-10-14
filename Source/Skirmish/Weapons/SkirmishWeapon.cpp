#include "SkirmishWeapon.h"
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
    }
}

bool ASkirmishWeapon::CanFire() const
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    return Ammo > 0 && World->GetTimeSeconds() - LastFireTime >= FireCooldown;
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
}

void ASkirmishWeapon::AddAmmo(int32 Amount)
{
    if (!HasAuthority())
    {
        return;
    }

    Ammo = FMath::Min(Ammo + Amount, MagazineSize);
}

