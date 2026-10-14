#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkirmishProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileHit, AActor*, HitActor, FVector, Location);

UCLASS()
class SKIRMISH_API ASkirmishProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASkirmishProjectile();

    UPROPERTY(BlueprintAssignable, Category = "Projectile")
    FOnProjectileHit OnProjectileHit;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    TObjectPtr<USoundBase> ImpactSound;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    float Damage = 20.f;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastImpact(FVector_NetQuantize Location);

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
