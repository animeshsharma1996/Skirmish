#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkirmishPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SKIRMISH_API ASkirmishPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	ASkirmishPickup();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	int32 HealthAmount = 0;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	int32 AmmoAmount = 5;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);
};
