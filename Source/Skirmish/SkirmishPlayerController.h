// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SkirmishPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class USkirmishSessionSubsystem;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ASkirmishPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UFUNCTION(Exec)
	void EOSLogin(const FString& CredentialName);

	UFUNCTION(Exec)
	void HostGame(const FString& GameTypeName);

	UFUNCTION(Exec)
	void FindGames();

	UFUNCTION(Exec)
	void JoinGame(int32 Index);

	USkirmishSessionSubsystem* GetSkirmishSession() const;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	bool ShouldUseTouchControls() const;
};
