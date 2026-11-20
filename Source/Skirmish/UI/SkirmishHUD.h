#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SkirmishHUD.generated.h"

class USkirmishHUDWidget;
class USkirmishPauseMenuWidget;

UCLASS()
class SKIRMISH_API ASkirmishHUD : public AHUD
{
    GENERATED_BODY()

public:
    void TogglePauseMenu();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USkirmishHUDWidget> HUDWidgetClass;

    UPROPERTY()
    TObjectPtr<USkirmishHUDWidget> HUDWidget;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USkirmishPauseMenuWidget> PauseMenuClass;

    UPROPERTY()
    TObjectPtr<USkirmishPauseMenuWidget> PauseMenu;

    UFUNCTION()
    void ClosePauseMenu();
};
