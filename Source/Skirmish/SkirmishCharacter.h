// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Actors/PickupReceiver.h"
#include "SkirmishCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class ASkirmishProjectile;
struct FInputActionValue;
class UTextRenderComponent;
class UMaterialInterface;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(config=Game)
class ASkirmishCharacter : public ACharacter, public IPickupReceiver
{
	GENERATED_BODY()

public:
	ASkirmishCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReceivePickup(int32 InPoints, int32 InAmmo) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void SetSkinMaterial(UMaterialInterface* NewMaterial);
	void Heal(int32 Amount);
	void AddAmmo(int32 Amount);
	bool IsDead() const { return Health <= 0.f; }

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
protected:

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

	UPROPERTY(VisibleAnywhere, Category = "Score")
	TObjectPtr<UTextRenderComponent> StatusLabel;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<ASkirmishProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float FireCooldown = 0.25f;

	UPROPERTY(ReplicatedUsing = OnRep_SkinMaterial)
	TObjectPtr<UMaterialInterface> SkinMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_Ammo, VisibleAnywhere, Category = "Weapon")
	int32 Ammo = 10;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Health")
	float Health = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float Damage = 20.f;

	float LastFireTime = -100.f;
	bool bDeathHandled = false;

	UFUNCTION(Server, Reliable)
	void ServerFire();

	UFUNCTION()
	void HandleProjectileHit(AActor* HitActor, FVector Location);

	UFUNCTION()
	void OnRep_SkinMaterial();
	
	UFUNCTION()
	void OnRep_Ammo();

	UFUNCTION()
	void OnRep_Health();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void ApplySkin();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Fire();
	void UpdateLabel();
	void HandleDeath();

};

