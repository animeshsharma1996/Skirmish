#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SkirmishLobbyGameMode.generated.h"

UCLASS()
class SKIRMISH_API ASkirmishLobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASkirmishLobbyGameMode();

    /** Called by the Player Controller after a player changes their ready flag */
    void PlayerReadyChanged();

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FString MatchMapPath = TEXT("/Game/Maps/StylizedEgypt");

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MinPlayers = 2;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    float StartDelay = 5.f;

private:
    bool AreAllPlayersReady() const;
    void StartMatch();

    FTimerHandle StartTimer;
};
