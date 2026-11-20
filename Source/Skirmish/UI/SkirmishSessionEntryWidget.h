#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/SkirmishTypes.h"
#include "SkirmishSessionEntryWidget.generated.h"

class UButton;
class UTextBlock;

/** One row of the server list : host, mode, players, ping and a Join button */
UCLASS()
class SKIRMISH_API USkirmishSessionEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Setup(const FSkirmishSessionResult& Result);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HostNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GameTypeText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PlayersText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PingText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> JoinButton;

private:
    int32 SessionIndex = INDEX_NONE;

    UFUNCTION()
    void HandleJoinClicked();
};
