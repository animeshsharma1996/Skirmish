#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SkirmishMenuHUD.generated.h"

class USkirmishMainMenuWidget;

/** The HUD of the main menu map : shows the menu and frees the mouse */
UCLASS()
class SKIRMISH_API ASkirmishMenuHUD : public AHUD
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USkirmishMainMenuWidget> MainMenuClass;

    UPROPERTY()
    TObjectPtr<USkirmishMainMenuWidget> MainMenu;
};
