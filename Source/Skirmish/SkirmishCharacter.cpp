// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkirmishCharacter.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Skirmish.h"
#include "Net/UnrealNetwork.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SkirmishGameMode.h"
#include "Core/SkirmishGameState.h"
#include "Weapons/SkirmishWeapon.h"
#include "Animation/AnimMontage.h"
#include "Core/SkirmishPlayerState.h"

ASkirmishCharacter::ASkirmishCharacter()
{
	// Tick after physics, when this frame's animation is finished
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true; // Always face where the camera looks, like Fortnite
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if(Movement)
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
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

	// Montages play on the hidden mannequin - the VisualMesh copies its pose
	USkeletalMeshComponent* CharacterMesh = GetMesh();

	if (CharacterMesh)
	{
		CharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	VisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMesh"));

	if (VisualMesh && CharacterMesh)
	{
		VisualMesh->SetupAttachment(CharacterMesh);
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));

	if (FirstPersonCamera)
	{
		// A steady eye point - only the mouse turns it
		FirstPersonCamera->SetupAttachment(RootComponent);
		FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
		FirstPersonCamera->bUsePawnControlRotation = true;
		FirstPersonCamera->SetAutoActivate(false);
	}
}

void ASkirmishCharacter::BeginPlay()
{
	Super::BeginPlay();

	USkeletalMeshComponent* CharacterMesh = GetMesh();

	if (CharacterMesh && VisualMesh && VisualMesh->GetSkeletalMeshAsset())
	{
		CharacterMesh->SetVisibility(false);

		VisualMesh->AddTickPrerequisiteComponent(CharacterMesh);
	}

	if (HasAuthority())
	{
		for (const TSubclassOf<ASkirmishWeapon>& WeaponClass : DefaultWeapons)
		{
			GiveWeapon(WeaponClass);
		}

		EquipWeapon(0);
	}
}

void ASkirmishCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		// Also catches a respawn : the new character reads the mode from the Player State
		const ESkirmishCameraMode CameraMode = GetCameraMode();

		if (CameraMode != AppliedCameraMode)
		{
			ApplyCameraMode(CameraMode);
		}

		UCameraComponent* ActiveCamera = GetActiveCamera();

		if (ActiveCamera)
		{
			const float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
			ActiveCamera->SetFieldOfView(FMath::FInterpTo(ActiveCamera->FieldOfView, TargetFOV, DeltaSeconds, FOVInterpSpeed));
		}
	}

	// Cosmetic, so a dedicated server skips it
	if (CurrentWeapon && GetNetMode() != NM_DedicatedServer)
	{
		CurrentWeapon->UpdateAimCorrection(GetWeaponAimTarget(), DeltaSeconds);
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
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASkirmishCharacter::Reload);
		EnhancedInputComponent->BindAction(CycleWeaponAction, ETriggerEvent::Triggered, this, &ASkirmishCharacter::CycleWeapon);
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &ASkirmishCharacter::ToggleCameraMode);
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
	DOREPLIFETIME(ASkirmishCharacter, OutlineMaterial);
	DOREPLIFETIME(ASkirmishCharacter, Health);
	DOREPLIFETIME(ASkirmishCharacter, CurrentWeapon);
	DOREPLIFETIME_CONDITION(ASkirmishCharacter, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASkirmishCharacter, Weapons, COND_OwnerOnly);
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
	USkeletalMeshComponent* DisplayMesh = GetDisplayMesh();

	if (SkinMaterial && DisplayMesh)
	{
		DisplayMesh->SetMaterial(0, SkinMaterial);
	}
}

void ASkirmishCharacter::SetOutlineMaterial(UMaterialInterface* NewMaterial)
{
	if (!HasAuthority())
	{
		return;
	}

	OutlineMaterial = NewMaterial;
	ApplyOutline(); // OnRep_OutlineMaterial does not run on the server
}

void ASkirmishCharacter::OnRep_OutlineMaterial()
{
	ApplyOutline();
}

void ASkirmishCharacter::ApplyOutline()
{
	USkeletalMeshComponent* DisplayMesh = GetDisplayMesh();

	if (!DisplayMesh)
	{
		return;
	}

	DisplayMesh->SetOverlayMaterial(OutlineMaterial);
}

USkeletalMeshComponent* ASkirmishCharacter::GetDisplayMesh() const
{
	if (VisualMesh && VisualMesh->GetSkeletalMeshAsset())
	{
		return VisualMesh;
	}

	return GetMesh();
}

ESkirmishCameraMode ASkirmishCharacter::GetCameraMode() const
{
	const ASkirmishPlayerState* SkirmishPlayerState = GetPlayerState<ASkirmishPlayerState>();

	if (!SkirmishPlayerState)
	{
		return ESkirmishCameraMode::ThirdPerson;
	}

	return SkirmishPlayerState->GetCameraMode();
}

UCameraComponent* ASkirmishCharacter::GetActiveCamera() const
{
	if (IsFirstPerson() && FirstPersonCamera)
	{
		return FirstPersonCamera;
	}

	return FollowCamera;
}

void ASkirmishCharacter::ToggleCameraMode()
{
	ASkirmishPlayerState* SkirmishPlayerState = GetPlayerState<ASkirmishPlayerState>();

	if (!SkirmishPlayerState)
	{
		return;
	}

	const ESkirmishCameraMode NewMode = IsFirstPerson() ? ESkirmishCameraMode::ThirdPerson : ESkirmishCameraMode::FirstPerson;

	// Only this player needs to know - nothing is sent to the server
	SkirmishPlayerState->SetCameraMode(NewMode);
}

void ASkirmishCharacter::ApplyCameraMode(ESkirmishCameraMode NewMode)
{
	AppliedCameraMode = NewMode;

	const bool bFirstPerson = NewMode == ESkirmishCameraMode::FirstPerson;

	if (FollowCamera)
	{
		FollowCamera->SetActive(!bFirstPerson);
	}

	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(bFirstPerson);
	}

	USkeletalMeshComponent* DisplayMesh = GetDisplayMesh();

	if (!DisplayMesh)
	{
		return;
	}

	for (const FName& BoneName : FirstPersonHiddenBones)
	{
		if (bFirstPerson)
		{
			DisplayMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
		}
		else
		{
			DisplayMesh->UnHideBoneByName(BoneName);
		}
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

FVector ASkirmishCharacter::GetWeaponAimTarget() const
{
	// The owner knows exactly what is under the crosshair
	if (IsLocallyControlled())
	{
		return GetAimTarget();
	}

	// Everyone else : a point far along where this player is looking
	return GetPawnViewLocation() + GetBaseAimRotation().Vector() * 3000.f;
}

FVector ASkirmishCharacter::GetAimTarget() const
{
	UWorld* World = GetWorld();
	UCameraComponent* ActiveCamera = GetActiveCamera();

	if (!World || !ActiveCamera)
	{
		return GetActorLocation() + GetActorForwardVector() * 10000.f;
	}

	const FVector Start = ActiveCamera->GetComponentLocation();
	const FVector End = Start + ActiveCamera->GetForwardVector() * 10000.f;

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
	UE_LOG(LogTemp, Warning, TEXT("[%s] Fire: Weapon=%s Ammo=%d"), HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*GetNameSafe(CurrentWeapon), CurrentWeapon ? CurrentWeapon->GetAmmo() : -1);

	if (!CurrentWeapon)
	{
		return;
	}

	if (CurrentWeapon->GetAmmo() <= 0)
	{
		Reload();
		return;
	}

	ServerFire(GetAimTarget());
}

void ASkirmishCharacter::ServerFire_Implementation(FVector_NetQuantize AimTarget)
{
	UE_LOG(LogTemp, Warning, TEXT("ServerFire from %s"), *GetName());

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
		UE_LOG(LogTemp, Warning, TEXT("ServerFire rejected: target not in front"));
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

	const ASkirmishGameMode* SkirmishGameMode = World ? World->GetAuthGameMode<ASkirmishGameMode>() : nullptr;

	if (SkirmishGameMode && !SkirmishGameMode->CanDamage(EventInstigator, GetController()))
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

void ASkirmishCharacter::OnRep_CurrentWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] OnRep_CurrentWeapon: %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"), *GetNameSafe(CurrentWeapon));

	AttachWeapon();
}

void ASkirmishCharacter::AttachWeapon()
{
	USkeletalMeshComponent* DisplayMesh = GetDisplayMesh();

	if (!CurrentWeapon || !DisplayMesh)
	{
		return;
	}

	CurrentWeapon->AttachToComponent(DisplayMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
	CurrentWeapon->NotifyAmmoChanged();
	CurrentWeapon->OnWeaponFired.AddUniqueDynamic(this, &ASkirmishCharacter::PlayFireAnimation);
	CurrentWeapon->OnReloadStarted.AddUniqueDynamic(this, &ASkirmishCharacter::PlayReloadAnimation);

	PlayWeaponMontage(CurrentWeapon->GetEquipMontage());
}

void ASkirmishCharacter::PlayFireAnimation()
{
	if (!CurrentWeapon)
	{
		return;
	}

	PlayWeaponMontage(CurrentWeapon->GetFireMontage());
}

void ASkirmishCharacter::PlayReloadAnimation()
{
	if (!CurrentWeapon)
	{
		return;
	}

	PlayWeaponMontage(CurrentWeapon->GetReloadMontage());
}

void ASkirmishCharacter::PlayWeaponMontage(UAnimMontage* Montage)
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!Montage || !CharacterMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Play(Montage);
}

void ASkirmishCharacter::GiveWeapon(TSubclassOf<ASkirmishWeapon> WeaponClass)
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !WeaponClass)
	{
		return;
	}

	for (ASkirmishWeapon* Weapon : Weapons)
	{
		if (Weapon && Weapon->GetClass() == WeaponClass)
		{
			Weapon->AddAmmo(Weapon->GetMagazineSize());
			return;
		}
	}

	if (Weapons.Num() >= MaxWeapons && CurrentWeapon)
	{
		Weapons.Remove(CurrentWeapon);
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASkirmishWeapon* NewWeapon = World->SpawnActor<ASkirmishWeapon>(WeaponClass, GetActorTransform(), Params);
	if (!NewWeapon)
	{
		return;
	}

	NewWeapon->OnWeaponHit.AddDynamic(this, &ASkirmishCharacter::HandleWeaponHit);
	NewWeapon->SetActorHiddenInGame(true);

	const int32 Index = Weapons.Add(NewWeapon);
	EquipWeapon(Index);
}

void ASkirmishCharacter::EquipWeapon(int32 Index)
{
	// The client sends the index - always check it
	if (!HasAuthority() || !Weapons.IsValidIndex(Index))
	{
		return;
	}

	ASkirmishWeapon* NewWeapon = Weapons[Index];
	if (!NewWeapon || NewWeapon == CurrentWeapon)
	{
		return;
	}

	// Put the old weapon away
	if (CurrentWeapon)
	{
		CurrentWeapon->CancelReload();
		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentWeapon = NewWeapon;
	CurrentWeapon->SetActorHiddenInGame(false);
	AttachWeapon(); // server does not get OnRep_CurrentWeapon
}

void ASkirmishCharacter::ReceiveWeapon(TSubclassOf<ASkirmishWeapon> WeaponClass)
{
	if (IsDead())
	{
		return;
	}

	GiveWeapon(WeaponClass);
}

void ASkirmishCharacter::CycleWeapon(const FInputActionValue& Value)
{
	const float WheelValue = Value.Get<float>();

	if (FMath::IsNearlyZero(WheelValue) || Weapons.Num() < 2)
	{
		return;
	}

	StopFire();

	// Send which way the player scrolled - the server works out the weapon
	ServerCycleWeapon(WheelValue > 0.f ? 1 : -1);
}

void ASkirmishCharacter::ServerCycleWeapon_Implementation(int8 Step)
{
	if (IsDead() || Weapons.Num() == 0)
	{
		return;
	}

	// Never trust the client : only ever move one weapon at a time
	const int32 SafeStep = Step > 0 ? 1 : -1;
	const int32 CurrentIndex = FMath::Max(Weapons.IndexOfByKey(CurrentWeapon), 0);

	// Wraps around : after the last weapon comes the first
	const int32 NewIndex = (CurrentIndex + SafeStep + Weapons.Num()) % Weapons.Num();

	EquipWeapon(NewIndex);
}

void ASkirmishCharacter::Reload()
{
	if (CurrentWeapon && CurrentWeapon->CanReload())
	{
		StopFire();
		ServerReload();
	}
}

void ASkirmishCharacter::ServerReload_Implementation()
{
	if (IsDead() || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->StartReload();
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

	USkeletalMeshComponent* DisplayMesh = GetDisplayMesh();

	if (DisplayMesh && GetNetMode() != NM_DedicatedServer)
	{
		DisplayMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		DisplayMesh->SetSimulatePhysics(true);
	}

	if (StatusLabel)
	{
		StatusLabel->SetVisibility(false);
	}
}

void ASkirmishCharacter::Destroyed()
{
	if (HasAuthority())
	{
		for (ASkirmishWeapon* Weapon : Weapons)
		{
			if (Weapon)
			{
				Weapon->Destroy();
			}
		}
	}

	Super::Destroyed();
}
