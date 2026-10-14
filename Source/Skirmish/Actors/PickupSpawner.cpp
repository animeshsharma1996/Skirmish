#include "PickupSpawner.h"
#include "SkirmishPickup.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"

APickupSpawner::APickupSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetBoxExtent(FVector(1000.f, 1000.f, 32.f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SpawnArea;
}
void APickupSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &APickupSpawner::SpawnPickup, SpawnInterval, true);
	}
}
void APickupSpawner::SpawnPickup()
{
	if (SpawnableClasses.Num() == 0)
	{
		return;
	}
	const int32 Index = FMath::RandRange(0, SpawnableClasses.Num() - 1);
	const FVector Location = UKismetMathLibrary::RandomPointInBoundingBox(SpawnArea->GetComponentLocation(), SpawnArea->GetScaledBoxExtent());
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<ASkirmishPickup>(SpawnableClasses[Index], Location, FRotator::ZeroRotator,Params);
}