#include "SkirmishPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../Weapons/SkirmishWeapon.h"
#include "SkirmishCharacter.h"

ASkirmishPickup::ASkirmishPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	if (IsValid(Mesh))
	{
		Mesh->SetSimulatePhysics(true);
		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		RootComponent = Mesh;
	}

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));

	if (IsValid(Trigger))
	{
		Trigger->SetupAttachment(Mesh);
		Trigger->SetSphereRadius(60.f);
		Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	}

	// Networking
	bReplicates = true;
	SetReplicateMovement(true);
}

void ASkirmishPickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && IsValid(Trigger))
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASkirmishPickup::OnTriggerOverlap);
	}
}

void ASkirmishPickup::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	IPickupReceiver* PickupReceiver = Cast<IPickupReceiver>(OtherActor);
	if (PickupReceiver)
	{
		if (WeaponClass)
		{
			PickupReceiver->ReceiveWeapon(WeaponClass);
		}
		else
		{
			PickupReceiver->ReceivePickup(HealthAmount, AmmoAmount);
		}

		Destroy();
	}
}