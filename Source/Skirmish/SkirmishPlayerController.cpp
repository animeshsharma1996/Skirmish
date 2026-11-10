// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkirmishPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Skirmish.h"
#include "Online/SkirmishSessionSubsystem.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Core/SkirmishPlayerState.h"
#include "Core/SkirmishLobbyGameMode.h"

void ASkirmishPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		} 
		else 
		{
			UE_LOG(LogSkirmish, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void ASkirmishPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ASkirmishPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ASkirmishPlayerController::EOSLogin(const FString& CredentialName)
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		SkirmishSession->Login(CredentialName);
	}
}

USkirmishSessionSubsystem* ASkirmishPlayerController::GetSkirmishSession() const
{
	UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<USkirmishSessionSubsystem>();
}

void ASkirmishPlayerController::HostGame(const FString& GameTypeName)
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		const bool bTeam = GameTypeName.Equals(TEXT("Team"), ESearchCase::IgnoreCase);
		SkirmishSession->HostGame(bTeam ? ESkirmishGameType::Team : ESkirmishGameType::AllVsAll);
	}
}

void ASkirmishPlayerController::FindGames()
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		SkirmishSession->FindGames();
	}
}

void ASkirmishPlayerController::FindServers()
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		SkirmishSession->FindServers();
	}
}

void ASkirmishPlayerController::JoinGame(int32 Index)
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		SkirmishSession->JoinGame(Index);
	}
}

void ASkirmishPlayerController::Ready()
{
	ASkirmishPlayerState* SkirmishPlayerState = GetPlayerState<ASkirmishPlayerState>();

	if (SkirmishPlayerState)
	{
		ServerSetReady(!SkirmishPlayerState->IsReady());
	}
}

void ASkirmishPlayerController::QuickMatch(const FString& GameTypeName)
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		const bool bTeam = GameTypeName.Equals(TEXT("Team"), ESearchCase::IgnoreCase);
		SkirmishSession->QuickMatch(bTeam ? ESkirmishGameType::Team : ESkirmishGameType::AllVsAll);
	}
}

void ASkirmishPlayerController::LeaveGame()
{
	USkirmishSessionSubsystem* SkirmishSession = GetSkirmishSession();

	if (SkirmishSession)
	{
		SkirmishSession->LeaveGame();
	}
}

void ASkirmishPlayerController::ServerSetReady_Implementation(bool bReady)
{
	ASkirmishPlayerState* SkirmishPlayerState = GetPlayerState<ASkirmishPlayerState>();

	if (!SkirmishPlayerState)
	{
		return;
	}

	SkirmishPlayerState->SetReady(bReady);

	UWorld* World = GetWorld();
	ASkirmishLobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<ASkirmishLobbyGameMode>() : nullptr;

	if (LobbyGameMode)
	{
		LobbyGameMode->PlayerReadyChanged();
	}
}

