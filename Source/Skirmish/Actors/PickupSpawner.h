// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawner.generated.h"

class UBoxComponent;
class ASkirmishPickup;

UCLASS()
class SKIRMISH_API APickupSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupSpawner();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Spawner")
	TObjectPtr<UBoxComponent> SpawnArea;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	TArray<TSubclassOf<ASkirmishPickup>> SpawnableClasses;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	float SpawnInterval = 2.f;

private:
	void SpawnPickup();
	FTimerHandle SpawnTimer;

};
