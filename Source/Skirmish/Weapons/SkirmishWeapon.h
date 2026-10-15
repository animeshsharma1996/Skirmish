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
    float GetTimeBetweenShots() const { return 1.f / FMath::Max(FireRate, 0.1f); }
    void NotifyAmmoChanged() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<USceneComponent> Muzzle;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    int32 MagazineSize = 30;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0.1"))
    float FireRate = 8.f;

    UPROPERTY(ReplicatedUsing = OnRep_Ammo, VisibleAnywhere, Category = "Weapon")
    int32 Ammo = 0;

	UFUNCTION()
	void OnRep_Ammo();

private:
    float LastFireTime = -100.f;
};
