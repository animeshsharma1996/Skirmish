// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/SkirmishGameState.h"
#include "Core/SkirmishTypes.h"
#include "SkirmishGameMode.generated.h"

UCLASS(abstract)
class ASkirmishGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASkirmishGameMode();

	void PlayerEliminated(APawn* Victim, AController* VictimController, AController* KillerController);
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	bool CanDamage(AController* InstigatorController, AController* VictimController) const;

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

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	ESkirmishGameType DefaultGameType = ESkirmishGameType::AllVsAll;

	/** Team outlines : [0] Red, [1] Blue */
	UPROPERTY(EditDefaultsOnly, Category = "Players")
	TArray<TObjectPtr<UMaterialInterface>> TeamOutlineMaterials;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 TeamKillLimit = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MinPlayersToStart = 1;

	void RespawnPlayer(TWeakObjectPtr<AController> Controller);

private:
	FTimerHandle MatchTimer;
	int32 SkinCount = 0;
	ESkirmishGameType GameType = ESkirmishGameType::AllVsAll;

	bool IsTeamGame() const { return GameType == ESkirmishGameType::Team; }
	int32 PickTeam() const;
	int32 GetLeadingTeam() const;
	void EndTeamMatch(int32 WinningTeam);

	void EnterPhase(ESkirmishMatchPhase NewPhase);
	void TickMatchTimer();
	void EndMatch(APlayerState* Winner);
	void ResetMatch();
	APlayerState* GetTopPlayer() const;
};



