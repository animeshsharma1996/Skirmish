#include "SkirmishProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ASkirmishProjectile::ASkirmishProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(10.f);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    RootComponent = Collision;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->InitialSpeed = 2500.f;
    Movement->MaxSpeed = 2500.f;
    Movement->ProjectileGravityScale = 0.f;
    Movement->bRotationFollowsVelocity = true;

    // Destroy automatically after 3 seconds
    InitialLifeSpan = 3.f;

    // Networking
    bReplicates = true;
    SetReplicateMovement(true);
}

void ASkirmishProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Never hit the player who fired
    if (GetOwner())
    {
        Collision->IgnoreActorWhenMoving(GetOwner(), true);
    }

    // Only the server reacts to hits
    if (HasAuthority())
    {
        Collision->OnComponentHit.AddDynamic(this, &ASkirmishProjectile::OnHit);
    }
}

void ASkirmishProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != GetOwner())
    {
        UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, nullptr);
    }

    OnProjectileHit.Broadcast(OtherActor, Hit.ImpactPoint);

    MulticastImpact(Hit.ImpactPoint);

    if (Collision)
    {
        Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    SetLifeSpan(0.2f);
}

void ASkirmishProjectile::MulticastImpact_Implementation(FVector_NetQuantize Location)
{
    // Temporary impact effect
    DrawDebugSphere(GetWorld(), Location, 30.f, 12, FColor::Orange, false, 1.f);

    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
    }

    // Hide the projectile on every machine
    Mesh->SetVisibility(false);
}



