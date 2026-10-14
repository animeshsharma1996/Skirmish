// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PickupReceiver.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPickupReceiver : public UInterface
{
	GENERATED_BODY()
};

class SKIRMISH_API IPickupReceiver
{
	GENERATED_BODY()

public:
	virtual void ReceivePickup(int32 HealthAmount, int32 AmmoAmount) = 0;
};
