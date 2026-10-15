// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkirmishCharacter.h"
#include "Actors/SkirmishProjectile.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Skirmish.h"
#include "Net/UnrealNetwork.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SkirmishGameMode.h"
#include "Core/SkirmishGameState.h"
#include "Weapons/SkirmishWeapon.h"

ASkirmishCharacter::ASkirmishCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PointsLabel"));

	if (StatusLabel)
	{
		StatusLabel->SetupAttachment(RootComponent);
		StatusLabel->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
		StatusLabel->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		StatusLabel->SetHorizontalAlignment(EHTA_Center);
		StatusLabel->SetTextRenderColor(FColor::Yellow);
		StatusLabel->SetText(FText::AsNumber(0));
	}
}

void ASkirmishCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnDefaultWeapon();
	}
}

void ASkirmishCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkirmishCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASkirmishCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkirmishCharacter::Look);

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASkirmishCharacter::Fire);
	}
	else
	{
		UE_LOG(LogSkirmish, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASkirmishCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (CurrentWeapon)
	{
		CurrentWeapon->NotifyAmmoChanged();
	}
}

void ASkirmishCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASkirmishCharacter, SkinMaterial);
	DOREPLIFETIME(ASkirmishCharacter, Health);
	DOREPLIFETIME(ASkirmishCharacter, CurrentWeapon);
}

void ASkirmishCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void ASkirmishCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ASkirmishCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ASkirmishCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ASkirmishCharacter::DoJumpStart()
{
	Jump();
}

void ASkirmishCharacter::DoJumpEnd()
{
	StopJumping();
}

void ASkirmishCharacter::SetSkinMaterial(UMaterialInterface* NewMaterial)
{
	if (!HasAuthority())
	{
		return;
	}

	SkinMaterial = NewMaterial;
	ApplySkin(); 
}

void ASkirmishCharacter::OnRep_SkinMaterial()
{
	ApplySkin();
}
void ASkirmishCharacter::ApplySkin()
{
	if (SkinMaterial)
	{
		GetMesh()->SetMaterial(0, SkinMaterial);
	}
}

void ASkirmishCharacter::Fire()
{
	if (!CurrentWeapon)
	{
		return;
	}

	if (CurrentWeapon->GetAmmo() <= 0)
	{
		UKismetSystemLibrary::PrintString(this, TEXT("Out of ammo!"), true, false, FLinearColor::Yellow, 1.f);
		return;
	}

	ServerFire();

}

void ASkirmishCharacter::ServerFire_Implementation()
{
	if (IsDead() || !CurrentWeapon || !ProjectileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const ASkirmishGameState* SkirmishGameState = World->GetGameState<ASkirmishGameState>();
	if (SkirmishGameState && !SkirmishGameState->CanFire())
	{
		return;
	}

	USceneComponent* Muzzle = CurrentWeapon->GetMuzzle();
	if (!Muzzle || !CurrentWeapon->CanFire())
	{
		return;
	}

	CurrentWeapon->ConsumeAmmo();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASkirmishProjectile* Projectile = World->SpawnActor<ASkirmishProjectile>(ProjectileClass, Muzzle->GetComponentLocation(), GetActorRotation(), Params);

	if (Projectile)
	{
		Projectile->OnProjectileHit.AddDynamic(this, &ASkirmishCharacter::HandleProjectileHit);
	}

}

void ASkirmishCharacter::ClientShowHitMarker_Implementation(const FString& TargetName)
{
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("HIT %s!"), *TargetName), true, false, FLinearColor::Red, 2.f);
}

void ASkirmishCharacter::HandleProjectileHit(AActor* HitActor, FVector Location)
{
	if (Cast<APawn>(HitActor))
	{
		ClientShowHitMarker(HitActor->GetName());
	}
}

float ASkirmishCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || IsDead()) 
	{
		return 0.f;
	}
	
	UWorld* World = GetWorld();

	const ASkirmishGameState* SkirmishGameState = World ? World->GetGameState<ASkirmishGameState>() : nullptr;

	if (SkirmishGameState && !SkirmishGameState->CanDealDamage()) 
	{
		return 0.f;
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	UpdateLabel();

	if (IsDead())
	{
		HandleDeath(); 

		if (World)
		{
			if (ASkirmishGameMode* GameMode = World->GetAuthGameMode<ASkirmishGameMode>())
			{
				GameMode->PlayerEliminated(this, GetController(), EventInstigator);
			}
		}
	}

	return Applied;
}

void ASkirmishCharacter::ReceivePickup(int32 HealthAmount, int32 AmmoAmount)
{
	if (HealthAmount > 0)
	{
		Heal(HealthAmount);
	}

	if (AmmoAmount > 0 && CurrentWeapon)
	{
		CurrentWeapon->AddAmmo(AmmoAmount);
	}
}

void ASkirmishCharacter::Heal(int32 Amount)
{
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	Health = FMath::Min(Health + Amount, MaxHealth);
	UpdateLabel();
}

void ASkirmishCharacter::OnRep_Health()
{
	UpdateLabel();

	if (IsDead())
	{
		HandleDeath();
	}
}

void ASkirmishCharacter::UpdateLabel()
{
	if (StatusLabel)
	{
		StatusLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0f HP"), Health)));
	}
}

void ASkirmishCharacter::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();

	if (!World || !DefaultWeaponClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;        // ownership chain : Controller → Character → Weapon
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = World->SpawnActor<ASkirmishWeapon>(DefaultWeaponClass, GetActorTransform(), Params);
	AttachWeapon(); 
}

void ASkirmishCharacter::OnRep_CurrentWeapon()
{
	AttachWeapon();
}

void ASkirmishCharacter::AttachWeapon()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();

	if (!CurrentWeapon || !CharacterMesh)
	{
		return;
	}

	CurrentWeapon->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
	CurrentWeapon->NotifyAmmoChanged();
}

void ASkirmishCharacter::HandleDeath()
{
	if (bDeathHandled)
	{
		return;
	}

	bDeathHandled = true;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		CharacterMesh->SetSimulatePhysics(true);
	}

	if (StatusLabel)
	{
		StatusLabel->SetVisibility(false);
	}
}

void ASkirmishCharacter::Destroyed()
{
	if (HasAuthority() && CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}

	Super::Destroyed();
}
