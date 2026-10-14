// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkirmishGameMode.h"
#include "Core/SkirmishPlayerState.h"
#include "SkirmishCharacter.h"

ASkirmishGameMode::ASkirmishGameMode()
{
    PlayerStateClass = ASkirmishPlayerState::StaticClass();
    GameStateClass = ASkirmishGameState::StaticClass();
}

void ASkirmishGameMode::BeginPlay()
{
    Super::BeginPlay();

    EnterPhase(ESkirmishMatchPhase::Warmup);
    GetWorldTimerManager().SetTimer(MatchTimer, this, &ASkirmishGameMode::TickMatchTimer, 1.f, true);
}

void ASkirmishGameMode::PostLogin(APlayerController* NewPlayer)
{
    ASkirmishPlayerState* PlayerState = NewPlayer ? NewPlayer->GetPlayerState<ASkirmishPlayerState>() : nullptr;

    if (PlayerState)
    {
        if (SkinMaterials.Num() > 0 && SkinMaterials[SkinCount % SkinMaterials.Num()])
        {
            PlayerState->SetSkinMaterial(SkinMaterials[SkinCount % SkinMaterials.Num()]);
            SkinCount++;
        }
    }

    Super::PostLogin(NewPlayer);
}

void ASkirmishGameMode::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);

    if (!NewPlayer) 
    {
        return;
    }

    ASkirmishCharacter* SkirmishCharacter = Cast<ASkirmishCharacter>(NewPlayer->GetPawn());
    ASkirmishPlayerState* PlayerState = NewPlayer->GetPlayerState<ASkirmishPlayerState>();

    if (SkirmishCharacter && PlayerState)
    {
        SkirmishCharacter->SetSkinMaterial(PlayerState->GetSkinMaterial());
    }
}


void ASkirmishGameMode::PlayerEliminated(APawn* Victim, AController* VictimController, AController* KillerController)
{
    UE_LOG(LogTemp, Warning, TEXT("%s eliminated %s"), *GetNameSafe(KillerController), *GetNameSafe(VictimController));

    ASkirmishPlayerState* VictimState = VictimController ? VictimController->GetPlayerState<ASkirmishPlayerState>() : nullptr;
    ASkirmishPlayerState* KillerState = KillerController ? KillerController->GetPlayerState<ASkirmishPlayerState>() : nullptr;

    if (VictimState)
    {
        VictimState->AddDeath();
    }

    // No kill for eliminating yourself
    if (KillerState && KillerState != VictimState)
    {
        KillerState->AddKill();
    }

    if (KillerState && KillerState->GetKills() >= KillLimit)
    {
        EndMatch(KillerState);
    }

    UE_LOG(LogTemp, Warning, TEXT("%s : %d kills | %s : %d deaths"),
        KillerState ? *KillerState->GetPlayerName() : TEXT("None"), KillerState ? KillerState->GetKills() : 0,
        VictimState ? *VictimState->GetPlayerName() : TEXT("None"), VictimState ? VictimState->GetDeaths() : 0);

    if (Victim)
    {
        Victim->DetachFromControllerPendingDestroy();
        Victim->SetLifeSpan(5.f);
    }

    if (VictimController)
    {
        FTimerHandle RespawnTimer;
        FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &ASkirmishGameMode::RespawnPlayer, TWeakObjectPtr<AController>(VictimController));

        GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnDelay, false);
    }

    if (ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>())
    {
        SkirmishGameState->MulticastElimination(KillerState ? KillerState->GetPlayerName() : TEXT("World"), VictimState ? VictimState->GetPlayerName() : TEXT("Unknown"));
    }
}

void ASkirmishGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller)
{
    if (Controller.IsValid() && !Controller->GetPawn())
    {
        RestartPlayer(Controller.Get());
    }
}

void ASkirmishGameMode::EnterPhase(ESkirmishMatchPhase NewPhase)
{
    ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (!SkirmishGameState)
    {
        return;
    }

    SkirmishGameState->SetPhase(NewPhase);

    switch (NewPhase)
    {
        case ESkirmishMatchPhase::Warmup:
            SkirmishGameState->Winner = nullptr;
            SkirmishGameState->RemainingSeconds = WarmupTime;
            break;
        case ESkirmishMatchPhase::InProgress:
            SkirmishGameState->RemainingSeconds = MatchTime;
            break;
        case ESkirmishMatchPhase::MatchOver:
            SkirmishGameState->RemainingSeconds = MatchOverTime;
            break;
    }
}

// LEAVE : what happens when a phase's timer runs out
void ASkirmishGameMode::TickMatchTimer()
{
    ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (!SkirmishGameState) 
    {
        return;
    }

    SkirmishGameState->RemainingSeconds = FMath::Max(SkirmishGameState->RemainingSeconds - 1, 0);

    if (SkirmishGameState->RemainingSeconds > 0) 
    {
        return;
    }

    switch (SkirmishGameState->GetPhase())
    {
        case ESkirmishMatchPhase::Warmup:     EnterPhase(ESkirmishMatchPhase::InProgress); break;
        case ESkirmishMatchPhase::InProgress: EndMatch(GetTopPlayer());                    break;
        case ESkirmishMatchPhase::MatchOver:  ResetMatch();                                break;
    }
}

void ASkirmishGameMode::EndMatch(APlayerState* Winner)
{
    if (ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>())
    {
        SkirmishGameState->Winner = Winner;
    }

    EnterPhase(ESkirmishMatchPhase::MatchOver);
}

APlayerState* ASkirmishGameMode::GetTopPlayer() const
{
    ASkirmishPlayerState* Best = nullptr;

    if (!GameState)
    {
        return Best;
    }

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        ASkirmishPlayerState* SkirmishPlayerState = Cast<ASkirmishPlayerState>(PlayerState);
        if (SkirmishPlayerState && (!Best || SkirmishPlayerState->GetKills() > Best->GetKills()))
        {
            Best = SkirmishPlayerState;
        }
    }

    return Best;
}

void ASkirmishGameMode::ResetMatch()
{
    UWorld* World = GetWorld();

    if (!World) 
    {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();

        if (!PlayerController)
        {
            continue;
        }

        if (ASkirmishPlayerState* PlayerState = PlayerController->GetPlayerState<ASkirmishPlayerState>())
        {
            PlayerState->ResetStats();
        }

        // Fresh character at a Player Start
        if (APawn* Pawn = PlayerController->GetPawn())
        {
            PlayerController->UnPossess();
            Pawn->Destroy();
        }

        RestartPlayer(PlayerController);
    }

    EnterPhase(ESkirmishMatchPhase::Warmup);
}

