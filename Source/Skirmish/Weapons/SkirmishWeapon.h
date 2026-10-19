#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkirmishWeapon.generated.h"

#define ECC_Weapon ECC_GameTraceChannel1

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponHit, AActor*, HitActor, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloadStarted);

class UStaticMeshComponent;
class USoundBase;
class UAnimMontage;

UCLASS()
class SKIRMISH_API ASkirmishWeapon : public AActor
{
    GENERATED_BODY()

public:
    ASkirmishWeapon();

    UPROPERTY(BlueprintAssignable, Category = "Weapon")
    FOnWeaponReloadStarted OnReloadStarted;

    UPROPERTY(BlueprintAssignable, Category = "Weapon")
    FOnWeaponHit OnWeaponHit;

    UPROPERTY(BlueprintAssignable, Category = "Weapon")
    FOnWeaponFired OnWeaponFired;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Can be checked on the server and on the owning client */
    bool CanFire() const;

    /** Server only */
    void ConsumeAmmo();
    void AddAmmo(int32 Amount);
    void Fire(const FVector& AimTarget, bool bAiming);

    UAnimMontage* GetFireMontage() const { return FireMontage; }
    bool IsAutomatic() const { return bAutomatic; }
    int32 GetAmmo() const { return Ammo; }
    USceneComponent* GetMuzzle() const { return Muzzle; }
    float GetTimeBetweenShots() const { return 1.f / FMath::Max(FireRate, 0.1f); }
    void NotifyAmmoChanged() const;

    void StartReload();
    void CancelReload();

    bool CanReload() const;
    bool IsReloading() const { return bIsReloading; }
    int32 GetReserveAmmo() const { return ReserveAmmo; }
    int32 GetMagazineSize() const { return MagazineSize; }

    UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
    UAnimMontage* GetEquipMontage() const { return EquipMontage; }

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

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    bool bAutomatic = true;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float Damage = 15.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float Range = 10000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float HipSpread = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float AimSpread = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    TObjectPtr<USoundBase> ImpactSound;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage> FireMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage> EquipMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    int32 StartingReserveAmmo = 90;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    int32 MaxReserveAmmo = 180;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float ReloadTime = 1.5f;

    UPROPERTY(ReplicatedUsing = OnRep_ReserveAmmo, VisibleAnywhere, Category = "Weapon")
    int32 ReserveAmmo = 0;

    UPROPERTY(ReplicatedUsing = OnRep_IsReloading, VisibleAnywhere, Category = "Weapon")
    bool bIsReloading = false;

    UFUNCTION()
    void OnRep_ReserveAmmo();

    UFUNCTION()
    void OnRep_IsReloading();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastFireEffects(FVector_NetQuantize TraceEnd, bool bHit);

	UFUNCTION()
	void OnRep_Ammo();

private:
    float LastFireTime = -100.f;
    FTimerHandle ReloadTimer; 

    void FinishReload();
};
