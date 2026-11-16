// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Actors/PickupReceiver.h"
#include "Weapons/WeaponHolder.h"
#include "Core/SkirmishTypes.h"
#include "SkirmishCharacter.generated.h"

class USpringArmComponent;
class USkirmishLagCompensation;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UTextRenderComponent;
class UMaterialInterface;
class ASkirmishWeapon;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ASkirmishCharacter : public ACharacter, public IPickupReceiver, public IWeaponHolder
{
	GENERATED_BODY()

public:
	ASkirmishCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkirmishLagCompensation> LagCompensation;

	UFUNCTION(Client, Reliable)
	void ClientShowHitMarker(const FString& TargetName);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReceivePickup(int32 InPoints, int32 InAmmo) override;
	virtual void ReceiveWeapon(TSubclassOf<ASkirmishWeapon> WeaponClass) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Destroyed() override;
	virtual ASkirmishWeapon* GetCurrentWeapon() const override { return CurrentWeapon; }
	virtual bool IsAiming() const override { return bIsAiming; }

	void SetSkinMaterial(UMaterialInterface* NewMaterial);
	void SetOutlineMaterial(UMaterialInterface* NewMaterial);
	void Heal(int32 Amount);
	bool IsDead() const { return Health <= 0.f; }
	void GiveWeapon(TSubclassOf<ASkirmishWeapon> WeaponClass);
	virtual bool IsFirstPerson() const override { return GetCameraMode() == ESkirmishCameraMode::FirstPerson; }
	ESkirmishCameraMode GetCameraMode() const;

	USkeletalMeshComponent* GetDisplayMesh() const;

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
protected:
	bool bDeathHandled = false;
	ESkirmishCameraMode AppliedCameraMode = ESkirmishCameraMode::ThirdPerson;
	FRotator RecoilOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CycleWeaponAction;

	UPROPERTY(VisibleAnywhere, Category = "Score")
	TObjectPtr<UTextRenderComponent> StatusLabel;

	UPROPERTY(ReplicatedUsing = OnRep_SkinMaterial)
	TObjectPtr<UMaterialInterface> SkinMaterial;

	/** Team outline, drawn over the mesh as an Overlay Material (works with any mesh) */
	UPROPERTY(ReplicatedUsing = OnRep_OutlineMaterial)
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Aiming")
	float WalkSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aiming")
	float AimWalkSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aiming")
	float DefaultFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aiming")
	float AimFOV = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aiming")
	float FOVInterpSpeed = 10.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsAiming, VisibleAnywhere, Category = "Aiming")
	bool bIsAiming = false;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Health")
	float Health = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TArray<FName> HeadBones = { TEXT("neck_01") };

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TArray<FName> LegBones = { TEXT("thigh_l"), TEXT("thigh_r") };

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float Damage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<ASkirmishWeapon>> DefaultWeapons;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	int32 MaxWeapons = 2;

	// Only the owner needs the whole inventory
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Weapon")
	TArray<TObjectPtr<ASkirmishWeapon>> Weapons;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, VisibleAnywhere, Category = "Weapon")
	TObjectPtr<ASkirmishWeapon> CurrentWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ToggleCameraAction;

	/** Hidden for the owner in first person, so the camera is not inside the head */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TArray<FName> FirstPersonHiddenBones = { TEXT("head") };

	/** First person : the camera follows this bone, so it sees the same hold as third person */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FName FirstPersonCameraBone = TEXT("head");

	/** First person : from the bone to the eyes, turned with the view (X forward, Z up) */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector FirstPersonEyeOffset = FVector(10.f, 0.f, 8.f);

	void UpdateFirstPersonCamera();

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TArray<TObjectPtr<UAnimMontage>> FrontDeathMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TArray<TObjectPtr<UAnimMontage>> BackDeathMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TArray<TObjectPtr<UAnimMontage>> LeftDeathMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TArray<TObjectPtr<UAnimMontage>> RightDeathMontages;

	UPROPERTY(Replicated)
	ESkirmishHitDirection DeathDirection = ESkirmishHitDirection::Front;

	UPROPERTY(Replicated)
	uint8 DeathVariant = 0;

	UFUNCTION()
	void PlayFireAnimation();

	UFUNCTION()
	void PlayReloadAnimation();

	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize AimTarget, FVector_NetQuantizeNormal ShotDirection, int32 ShotId);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	/** +1 for the next weapon, -1 for the previous one */
	UFUNCTION(Server, Reliable)
	void ServerCycleWeapon(int8 Step);

	UFUNCTION()
	void HandleWeaponHit(AActor* HitActor, FVector Location);

	UFUNCTION()
	void OnRep_SkinMaterial();

	UFUNCTION()
	void OnRep_OutlineMaterial();

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_CurrentWeapon();

	UFUNCTION()
	void OnRep_IsAiming();

	void SpawnDefaultWeapon();
	void AttachWeapon();
	void ApplySkin();
	void ApplyOutline();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartAiming();
	void StopAiming();
	void SetAiming(bool bNewAiming);
	void PlayWeaponMontage(UAnimMontage* Montage);
	void ApplyAimingState();
	void Fire();
	void ApplyRecoil(const FRotator& Kick);
	void RecoverRecoil(float DeltaSeconds);
	void UpdateLabel();
	void HandleDeath();
	void StartFire();
	void StopFire();
	void Reload();
	void CycleWeapon(const FInputActionValue& Value);
	void EquipWeapon(int32 Index);
	void ToggleCameraMode();
	void ApplyCameraMode(ESkirmishCameraMode NewMode);
	void ChooseDeathAnimation(const AActor* DamageCauser);

	UAnimMontage* GetDeathMontage() const;
	UCameraComponent* GetActiveCamera() const;
	FVector GetWeaponAimTarget() const;
	FVector GetAimTarget() const;
	FTimerHandle AutoFireTimer;
	ESkirmishHitZone GetHitZone(FName BoneName) const;
	bool IsBoneInZone(FName BoneName, const TArray<FName>& ZoneBones) const;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
};

