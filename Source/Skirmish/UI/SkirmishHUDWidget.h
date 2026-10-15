#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkirmishHUDWidget.generated.h"

class UTextBlock;

UCLASS()
class SKIRMISH_API USkirmishHUDWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PhaseText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ScoreboardText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> KillFeedText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> AmmoText;

    UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
    int32 MaxFeedLines = 5;

private:
    void UpdateMatchInfo();
    void UpdateAmmo();

    UFUNCTION()
    void HandleElimination(const FString& KillerName, const FString& VictimName);

    UFUNCTION()
    void HandleAmmoChanged(int32 Ammo);

    TArray<FString> FeedLines;
    FTimerHandle RefreshTimer;
};
