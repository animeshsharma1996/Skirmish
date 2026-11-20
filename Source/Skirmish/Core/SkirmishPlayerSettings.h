#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SkirmishTypes.h"
#include "SkirmishPlayerSettings.generated.h"

/** Choices from the main menu that last for the whole game */
UCLASS()
class SKIRMISH_API USkirmishPlayerSettings : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    void SetStartCameraMode(ESkirmishCameraMode NewMode) { StartCameraMode = NewMode; }
    ESkirmishCameraMode GetStartCameraMode() const { return StartCameraMode; }

private:
    ESkirmishCameraMode StartCameraMode = ESkirmishCameraMode::ThirdPerson;
};
