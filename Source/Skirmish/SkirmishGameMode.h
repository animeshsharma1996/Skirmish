// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/SkirmishGameState.h"
#include "SkirmishGameMode.generated.h"

UCLASS(abstract)
class ASkirmishGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ASkirmishGameMode();

	void PlayerEliminated(APawn* Victim, AController* VictimController, AController* KillerController);
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void BeginPlay() override;

protected :
	UPROPERTY(EditDefaultsOnly, Category = "Players")
	TArray<TObjectPtr<UMaterialInterface>> SkinMaterials;

	UPROPERTY(EditDefaultsOnly, Category = "Players")
	float RespawnDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 WarmupTime = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MatchTime = 180;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MatchOverTime = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 KillLimit = 10;

	void RespawnPlayer(TWeakObjectPtr<AController> Controller);

private:
	int32 SkinCount = 0;

	void EnterPhase(ESkirmishMatchPhase NewPhase);
	void TickMatchTimer();
	void EndMatch(APlayerState* Winner);
	void ResetMatch();
	APlayerState* GetTopPlayer() const;

	FTimerHandle MatchTimer;
};



