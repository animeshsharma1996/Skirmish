#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SkirmishHUD.generated.h"

class USkirmishHUDWidget;

UCLASS()
class SKIRMISH_API ASkirmishHUD : public AHUD
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USkirmishHUDWidget> HUDWidgetClass;

    UPROPERTY()
    TObjectPtr<USkirmishHUDWidget> HUDWidget;
};
