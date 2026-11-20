#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/SkirmishTypes.h"
#include "Types/SlateEnums.h"
#include "SkirmishMainMenuWidget.generated.h"

class UButton;
class UComboBoxString;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class USkirmishSessionEntryWidget;
class USkirmishSessionSubsystem;

UCLASS()
class SKIRMISH_API USkirmishMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    TSubclassOf<USkirmishSessionEntryWidget> SessionEntryClass;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PlayerNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> CredentialBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> LoginButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> GameTypeBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> CameraModeBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> HostButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuickMatchButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> FindGamesButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> FindServersButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UScrollBox> SessionList;

private:
    USkirmishSessionSubsystem* GetSkirmishSession() const;
    ESkirmishGameType GetSelectedGameType() const;
    void SetStatus(const FString& Message);

    UFUNCTION()
    void HandleLoginClicked();

    UFUNCTION()
    void HandleHostClicked();

    UFUNCTION()
    void HandleQuickMatchClicked();

    UFUNCTION()
    void HandleFindGamesClicked();

    UFUNCTION()
    void HandleFindServersClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UFUNCTION()
    void HandleCameraModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleLoginComplete(bool bWasSuccessful, const FString& PlayerName);

    UFUNCTION()
    void HandleSessionsFound(const TArray<FSkirmishSessionResult>& Results);

    UFUNCTION()
    void HandleHostComplete(bool bWasSuccessful);

    UFUNCTION()
    void HandleJoinComplete(bool bWasSuccessful);
};
