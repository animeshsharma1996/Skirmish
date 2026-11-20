#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Core/SkirmishTypes.h"
#include "SkirmishWeapon.generated.h"

#define ECC_Weapon ECC_GameTraceChannel1
#define ECC_Bullet ECC_GameTraceChannel2

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponHit, AActor*, HitActor, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReloadStarted);

class UStaticMeshComponent;
class USoundBase;
class UAnimMontage;
class UBlendSpace;
class ASkirmishBullet;
class UNiagaraSystem;
class UCameraShakeBase;
class ASkirmishCharacter;

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
    void Fire(const FVector& AimTarget, const FVector& ShotDirection, bool bAiming, int32 ShotId);

    /** Server : a hit that passed the lag compensation check */
    void ApplyConfirmedHit(ASkirmishCharacter* HitCharacter, FName BoneName, const FVector& HitLocation, const FVector& ShotDirection);
    FVector GetShotDirection(const FVector& AimTarget, bool bAiming) const;
    void PlayLocalFireEffects(const FVector& ShotDirection, int32 ShotId);
    int32 TakeShotId() { return ++LastShotId; }
    float GetRange() const { return Range; }
    float GetBulletSpeed() const;
    void UpdateAimCorrection(const FVector& AimTarget, float DeltaSeconds);

    UAnimMontage* GetFireMontage() const { return FireMontage; }
    bool IsAutomatic() const { return bAutomatic; }
    int32 GetAmmo() const { return Ammo; }
    USceneComponent* GetMuzzle() const { return Muzzle; }
    float GetTimeBetweenShots() const { return 1.f / FMath::Max(FireRate, 0.1f); }
    float GetDamageMultiplier(ESkirmishHitZone HitZone) const;
    void NotifyAmmoChanged() const;

    FRotator AddRecoil();
    bool IsBurstOver() const;
    float GetMaxRecoilPitch() const { return MaxRecoilPitch; }
    float GetRecoilRecoverySpeed() const { return RecoilRecoverySpeed; }

    void StartReload();
    void CancelReload();
    bool CanReload() const;
    bool IsReloading() const { return bIsReloading; }
    int32 GetReserveAmmo() const { return ReserveAmmo; }
    int32 GetMagazineSize() const { return MagazineSize; }
    UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
    UAnimMontage* GetEquipMontage() const { return EquipMontage; }
    UBlendSpace* GetMovementBlendSpace() const { return MovementBlendSpace; }
    UBlendSpace* GetAimOffset() const { return AimOffset; }

    bool GetLeftHandTransform(FTransform& OutTransform) const;
    bool GetLeftHandGripInPivot(FTransform& OutTransform) const;

    /** AimPivot relative to the Root - the aim correction tilt in third person */
    FTransform GetAimPivotRelativeTransform() const { return AimPivot ? AimPivot->GetRelativeTransform() : FTransform::Identity; }

protected:
    float AimCorrectionAlpha = 0.f;
    int32 ShotsInBurst = 0;
    float LastLocalShotTime = -100.f;
    float GetBurstSpread() const;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    TObjectPtr<USceneComponent> AimPivot; /** Moved locally for aiming. Not replicated, unlike the Root's attachment */
   
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
    float HeadMultiplier = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float LegsMultiplier = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float Range = 10000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float HipSpread = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    float AimSpread = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Bullet")
    TSubclassOf<ASkirmishBullet> BulletClass;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Bullet")
    bool bShowTracer = true;

    /** Bullets made at the start, so firing never has to spawn one */
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Bullet")
    int32 BulletPoolSize = 30;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float RecoilPitch = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float RecoilYaw = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float MaxRecoilPitch = 6.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float SpreadPerShot = 0.3f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float MaxBurstSpread = 3.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float RecoilResetTime = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float RecoilRecoverySpeed = 12.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    TObjectPtr<UNiagaraSystem> MuzzleFlash;

    /** A short streak that flies from the muzzle to where the shot stops, like the Vandal's */
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    TObjectPtr<UNiagaraSystem> TracerEffect;

    /** Must match Velocity X in the tracer's Add Velocity */
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    float TracerSpeed = 30000.f;

    /** Must match Sprite Size Y in the tracer's Initialize Particle */
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    float TracerLength = 300.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    TSubclassOf<UCameraShakeBase> FireCameraShake;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    float RecoilShakeScale = 1.f;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    TObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage> FireMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage> ReloadMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UAnimMontage> EquipMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
    TObjectPtr<UBlendSpace> MovementBlendSpace; 
    
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")  
    TObjectPtr<UBlendSpace> AimOffset; 

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation") 
    bool bUseLeftHandIK = true; /** Untick for one-handed weapons */

    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation", meta = (EditCondition = "bUseLeftHandIK"))
    FName LeftHandSocketName = TEXT("LeftHandIK");

    /** Bigger turns than this fade out - the character is not facing the target */
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
    float MaxAimCorrection = 20.f;

    /** How quickly the correction fades in and out when it starts or stops being used */
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Aim")
    float AimCorrectionFadeSpeed = 10.f;

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

	UFUNCTION()
	void OnRep_Ammo();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastFireEffects(FVector_NetQuantizeNormal ShotDirection);

    virtual void BeginPlay() override;

private:
    float LastFireTime = -100.f;
    int32 LastShotId = 0;
    FTimerHandle ReloadTimer; 

    void FinishReload();
    void PlayFireEffects(const FVector& ShotDirection, int32 ShotId);
    void SpawnBullet(const FVector& Start, const FVector& Direction, int32 ShotId);
    void SpawnTracer(const FVector& Start, const FVector& Direction);
};
