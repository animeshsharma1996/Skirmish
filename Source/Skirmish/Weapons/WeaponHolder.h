#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponHolder.generated.h"

class ASkirmishWeapon;

UINTERFACE(MinimalAPI)
class UWeaponHolder : public UInterface
{
    GENERATED_BODY()
};

class SKIRMISH_API IWeaponHolder
{
    GENERATED_BODY()

public:
    virtual ASkirmishWeapon* GetCurrentWeapon() const = 0;
    virtual bool IsAiming() const = 0;
};
