#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkirmishBullet.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UMaterialInterface;
class USoundBase;

UCLASS()
class SKIRMISH_API ASkirmishBullet : public AActor
{
    GENERATED_BODY()

public:
    ASkirmishBullet();

    /** Takes a bullet from the pool and fires it from Start along Direction. Only the shooter's own bullets have a ShotId */
    void Fire(const FVector& Start, const FVector& Direction, AActor* InOwner, APawn* InInstigator, int32 InShotId);

    float GetSpeed() const;

    /** Hides and stops the bullet, ready to be used again */
    void Sleep();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Bullet")
    TObjectPtr<UCapsuleComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Bullet")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Bullet")
    TObjectPtr<UProjectileMovementComponent> Movement;

    /** A bullet that hits nothing goes back to the pool after this long */
    UPROPERTY(EditDefaultsOnly, Category = "Bullet")
    float MaxLifetime = 3.f;

    UPROPERTY(EditDefaultsOnly, Category = "Bullet|Impact")
    TObjectPtr<UNiagaraSystem> ImpactEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Bullet|Impact")
    TArray<TObjectPtr<UMaterialInterface>> ImpactDecals;

    UPROPERTY(EditDefaultsOnly, Category = "Bullet|Impact")
    FVector DecalSize = FVector(4.f, 6.f, 6.f);

    UPROPERTY(EditDefaultsOnly, Category = "Bullet|Impact")
    float DecalLifeSpan = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Bullet|Impact")
    TObjectPtr<USoundBase> ImpactSound;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    int32 ShotId = INDEX_NONE;
    FTimerHandle LifetimeTimer;

    void SpawnImpactEffects(const FHitResult& Hit) const;
    void ReturnToPool();
};
