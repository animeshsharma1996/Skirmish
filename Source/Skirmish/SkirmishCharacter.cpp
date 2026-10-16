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
#include "Animation/SkirmishAnimInstance.h"
#include "Animation/AnimMontage.h"

ASkirmishCharacter::ASkirmishCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if(Movement)
	{
		Movement->bOrientRotationToMovement = false;
		Movement->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		Movement->JumpZVelocity = 500.f;
		Movement->AirControl = 0.35f;
		Movement->MaxWalkSpeed = 500.f;
		Movement->MinAnalogWalkSpeed = 20.f;
		Movement->BrakingDecelerationWalking = 2000.f;
		Movement->BrakingDecelerationFalling = 1500.0f;
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	if (CameraBoom)
	{
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->TargetArmLength = 250.f;
		CameraBoom->SocketOffset = FVector(0.f, 60.f, 60.f);
		CameraBoom->bUsePawnControlRotation = true;
	}

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
		StatusLabel->SetText(FText::FromString(" "));
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

void ASkirmishCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled() && FollowCamera)
	{
		const float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
		FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaSeconds, FOVInterpSpeed));
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
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ASkirmishCharacter::StartAiming);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ASkirmishCharacter::StopAiming);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASkirmishCharacter::StartFire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ASkirmishCharacter::StopFire);
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
	DOREPLIFETIME_CONDITION(ASkirmishCharacter, bIsAiming, COND_SkipOwner);
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

void ASkirmishCharacter::StartAiming()
{
	SetAiming(true);
}

void ASkirmishCharacter::StopAiming()
{
	SetAiming(false);
}

void ASkirmishCharacter::SetAiming(bool bNewAiming)
{
	if (IsDead())
	{
		return;
	}

	bIsAiming = bNewAiming;
	ApplyAimingState();

	if (!HasAuthority())
	{
		ServerSetAiming(bNewAiming);
	}
}

void ASkirmishCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	ApplyAimingState();
}

void ASkirmishCharacter::OnRep_IsAiming()
{
	ApplyAimingState();
}

void ASkirmishCharacter::ApplyAimingState()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : WalkSpeed;
	}
}

FVector ASkirmishCharacter::GetAimTarget() const
{
	UWorld* World = GetWorld();
	if (!World || !FollowCamera)
	{
		return GetActorLocation() + GetActorForwardVector() * 10000.f;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * 10000.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (CurrentWeapon)
	{
		Params.AddIgnoredActor(CurrentWeapon);
	}

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Weapon, Params))
	{
		return Hit.ImpactPoint;
	}

	return End;
}

void ASkirmishCharacter::StartFire()
{
	Fire();

	if (CurrentWeapon && CurrentWeapon->IsAutomatic())
	{
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &ASkirmishCharacter::Fire, CurrentWeapon->GetTimeBetweenShots(), true);
	}
}

void ASkirmishCharacter::StopFire()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimer);
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

	ServerFire(GetAimTarget());
}

void ASkirmishCharacter::ServerFire_Implementation(FVector_NetQuantize AimTarget)
{
	if (IsDead() || !CurrentWeapon)
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

	// Never trust the client : the target must be roughly where this player is looking
	const FVector ToTarget = (FVector(AimTarget) - GetActorLocation()).GetSafeNormal();
	if (FVector::DotProduct(ToTarget, GetBaseAimRotation().Vector()) < 0.5f)
	{
		return;
	}

	CurrentWeapon->Fire(AimTarget, bIsAiming);
}

void ASkirmishCharacter::ClientShowHitMarker_Implementation(const FString& TargetName)
{
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("HIT %s!"), *TargetName), true, false, FLinearColor::Red, 2.f);
}

void ASkirmishCharacter::HandleWeaponHit(AActor* HitActor, FVector Location)
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
	if (CurrentWeapon)
	{
		CurrentWeapon->OnWeaponHit.AddDynamic(this, &ASkirmishCharacter::HandleWeaponHit);
	}
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
	CurrentWeapon->OnWeaponFired.AddUniqueDynamic(this, &ASkirmishCharacter::PlayFireAnimation);
}

void ASkirmishCharacter::PlayFireAnimation()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	USkirmishAnimInstance* AnimInstance = Cast<USkirmishAnimInstance>(CharacterMesh->GetAnimInstance());
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->TriggerRecoil();

	UAnimMontage* FireMontage = CurrentWeapon ? CurrentWeapon->GetFireMontage() : nullptr;
	if (FireMontage)
	{
		AnimInstance->Montage_Play(FireMontage);
	}
}

void ASkirmishCharacter::HandleDeath()
{
	if (bDeathHandled)
	{
		GetWorldTimerManager().ClearTimer(AutoFireTimer);
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
