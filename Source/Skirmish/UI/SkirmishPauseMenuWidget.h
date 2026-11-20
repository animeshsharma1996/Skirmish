#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkirmishPauseMenuWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResumeRequested);

/** Resume, Ready (lobby), Leave Game and Quit. The game keeps running underneath */
UCLASS()
class SKIRMISH_API USkirmishPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Menu")
    FOnResumeRequested OnResumeRequested;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ResumeButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ReadyButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> LeaveButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

private:
    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleReadyClicked();

    UFUNCTION()
    void HandleLeaveClicked();

    UFUNCTION()
    void HandleQuitClicked();
};
