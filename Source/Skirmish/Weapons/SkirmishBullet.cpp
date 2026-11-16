#include "SkirmishBullet.h"
#include "SkirmishWeapon.h"
#include "SkirmishBulletPool.h"
#include "../SkirmishCharacter.h"
#include "../Core/SkirmishLagCompensation.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASkirmishBullet::ASkirmishBullet()
{
    PrimaryActorTick.bCanEverTick = false;

    // Every machine simulates its own bullet - nothing replicates
    bReplicates = false;

    Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));

    if (Collision)
    {
        // A thin capsule the size of a bullet - its length runs along local Z
        Collision->InitCapsuleSize(0.5f, 2.f);
        Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Collision->SetCollisionObjectType(ECC_Bullet);
        Collision->SetCollisionResponseToAllChannels(ECR_Block);
        Collision->SetCollisionResponseToChannel(ECC_Bullet, ECR_Ignore);

        // Do not hit other bullets, pickups or physics objects
        Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
        Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
        Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        RootComponent = Collision;
    }

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

    if (Mesh)
    {
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        // Lay the bullet mesh (nose along X) along the capsule (Z)
        Mesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    }

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));

    if (Movement)
    {
        Movement->SetUpdatedComponent(RootComponent);
        Movement->InitialSpeed = 40000.f;
        Movement->MaxSpeed = 40000.f;
        Movement->ProjectileGravityScale = 0.2f;
        // Fire() points the capsule along the shot - following velocity would turn it sideways
        Movement->bRotationFollowsVelocity = false;
        Movement->bAutoActivate = false;
    }
}

void ASkirmishBullet::BeginPlay()
{
    Super::BeginPlay();

    if (Collision)
    {
        Collision->OnComponentHit.AddDynamic(this, &ASkirmishBullet::OnHit);
    }
}

void ASkirmishBullet::Fire(const FVector& Start, const FVector& Direction, AActor* InOwner, APawn* InInstigator, int32 InShotId)
{
    ShotId = InShotId;
    SetOwner(InOwner);
    SetInstigator(InInstigator);

    // Point the capsule's length (Z) along the shot
    const FRotator BulletRotation = FRotationMatrix::MakeFromZ(Direction).Rotator();
    SetActorLocationAndRotation(Start, BulletRotation, false, nullptr, ETeleportType::TeleportPhysics);

    if (Collision)
    {
        // Never hit the shooter or their weapon - they may be different players each time
        Collision->ClearMoveIgnoreActors();
        Collision->IgnoreActorWhenMoving(InInstigator, true);
        Collision->IgnoreActorWhenMoving(InOwner, true);
    }

    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);

    if (Mesh)
    {
        Mesh->SetVisibility(true);
    }

    if (Movement)
    {
        // A hit stops the movement and clears its component, so set both again
        Movement->SetUpdatedComponent(Collision);
        Movement->Velocity = Direction.GetSafeNormal() * Movement->InitialSpeed;
        Movement->Activate(true);
    }

    GetWorldTimerManager().SetTimer(LifetimeTimer, this, &ASkirmishBullet::ReturnToPool, MaxLifetime, false);
}

float ASkirmishBullet::GetSpeed() const
{
    return Movement ? Movement->InitialSpeed : 0.f;
}

void ASkirmishBullet::Sleep()
{
    GetWorldTimerManager().ClearTimer(LifetimeTimer);

    if (Movement)
    {
        Movement->StopMovementImmediately();
        Movement->Deactivate();
    }

    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void ASkirmishBullet::ReturnToPool()
{
    Sleep();

    UWorld* World = GetWorld();
    USkirmishBulletPool* BulletPool = World ? World->GetSubsystem<USkirmishBulletPool>() : nullptr;

    if (!BulletPool)
    {
        Destroy();
        return;
    }

    BulletPool->Release(this);
}

void ASkirmishBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    SpawnImpactEffects(Hit);

    // Only the shooter's own bullet reports - it hit what the shooter saw. The server checks it
    ASkirmishCharacter* HitCharacter = Cast<ASkirmishCharacter>(OtherActor);
    APawn* Shooter = GetInstigator();

    if (ShotId != INDEX_NONE && HitCharacter && Shooter)
    {
        USkirmishLagCompensation* LagCompensation = Shooter->FindComponentByClass<USkirmishLagCompensation>();

        if (LagCompensation)
        {
            LagCompensation->ServerReportHit(ShotId, HitCharacter, Hit.BoneName, Hit.ImpactPoint);
        }
    }

    // Back to the pool instead of Destroy()
    ReturnToPool();
}

void ASkirmishBullet::SpawnImpactEffects(const FHitResult& Hit) const
{
    if (ImpactEffect)
    {
        // Niagara has its own pool - AutoRelease hands the effect back when it finishes
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), FVector(1.f), true, true, ENCPoolMethod::AutoRelease);
    }

    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
    }

    // Bullet holes go on walls and floors, not on players
    if (ImpactDecals.Num() > 0 && !Cast<APawn>(Hit.GetActor()))
    {
        UMaterialInterface* Decal = ImpactDecals[FMath::RandRange(0, ImpactDecals.Num() - 1)];

        if (Decal)
        {
            UGameplayStatics::SpawnDecalAtLocation(this, Decal, DecalSize, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), DecalLifeSpan);
        }
    }
}
