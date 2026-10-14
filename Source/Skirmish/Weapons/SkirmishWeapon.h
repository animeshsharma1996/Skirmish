#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkirmishWeapon.generated.h"

class UStaticMeshComponent;

UCLASS()
class SKIRMISH_API ASkirmishWeapon : public AActor
{
    GENERATED_BODY()

public:
    ASkirmishWeapon();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Can be checked on the server and on the owning client */
    bool CanFire() const;

    /** Server only */
    void ConsumeAmmo();

    /** Server only */
    void AddAmmo(int32 Amount);

    int32 GetAmmo() const { return Ammo; }
    USceneComponent* GetMuzzle() const { return Muzzle; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<USceneComponent> Muzzle;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    int32 MagazineSize = 30;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float FireCooldown = 0.15f;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Weapon")
    int32 Ammo = 0;

private:
    float LastFireTime = -100.f;
};
