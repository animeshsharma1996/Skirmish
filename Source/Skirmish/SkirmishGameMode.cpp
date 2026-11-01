// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkirmishGameMode.h"
#include "Core/SkirmishPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "SkirmishCharacter.h"

ASkirmishGameMode::ASkirmishGameMode()
{
    bUseSeamlessTravel = true;

    PlayerStateClass = ASkirmishPlayerState::StaticClass();
    GameStateClass = ASkirmishGameState::StaticClass();
}

void ASkirmishGameMode::BeginPlay()
{
    Super::BeginPlay();

    EnterPhase(ESkirmishMatchPhase::Warmup);
    GetWorldTimerManager().SetTimer(MatchTimer, this, &ASkirmishGameMode::TickMatchTimer, 1.f, true);
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

        // Team games get a Red or Blue outline, All vs All gets none
        UMaterialInterface* OutlineMaterial = nullptr;
        const int32 Team = PlayerState->GetTeamIndex();

        if (IsTeamGame() && TeamOutlineMaterials.IsValidIndex(Team))
        {
            OutlineMaterial = TeamOutlineMaterials[Team];
        }

        SkirmishCharacter->SetOutlineMaterial(OutlineMaterial);
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

    ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (IsTeamGame())
    {
        const bool bEnemyKill = KillerState && VictimState && KillerState != VictimState && KillerState->GetTeamIndex() != VictimState->GetTeamIndex();

        if (bEnemyKill && SkirmishGameState && SkirmishGameState->TeamScores.IsValidIndex(KillerState->GetTeamIndex()))
        {
            int32& TeamScore = SkirmishGameState->TeamScores[KillerState->GetTeamIndex()];
            TeamScore++;

            if (TeamScore >= TeamKillLimit)
            {
                EndTeamMatch(KillerState->GetTeamIndex());
            }
        }
    }
    else if (KillerState && KillerState->GetKills() >= KillLimit)
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

    if (SkirmishGameState)
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
        case ESkirmishMatchPhase::InProgress:
        {
            if (IsTeamGame())
            {
                EndTeamMatch(GetLeadingTeam());
            }
            else
            {
                EndMatch(GetTopPlayer());
            }
            break;
        }
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

        if (APawn* Pawn = PlayerController->GetPawn())
        {
            PlayerController->UnPossess();
            Pawn->Destroy();
        }

        RestartPlayer(PlayerController);
    }

    ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (SkirmishGameState)
    {
        SkirmishGameState->TeamScores.Init(0, 2);
        SkirmishGameState->WinningTeam = INDEX_NONE;
    }

    EnterPhase(ESkirmishMatchPhase::Warmup);
}

void ASkirmishGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    const int32 GameTypeValue = UGameplayStatics::GetIntOption(Options, TEXT("GameType"), static_cast<int32>(DefaultGameType));
    GameType = GameTypeValue == static_cast<int32>(ESkirmishGameType::Team) ? ESkirmishGameType::Team : ESkirmishGameType::AllVsAll;

    UE_LOG(LogTemp, Warning, TEXT("Game type : %s"), IsTeamGame() ? TEXT("Team") : TEXT("All vs All"));

    // The Game State already exists here - share the game type with every client
    ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (SkirmishGameState)
    {
        SkirmishGameState->GameType = GameType;
        SkirmishGameState->TeamScores.Init(0, 2);
        SkirmishGameState->WinningTeam = INDEX_NONE;
    }
}

void ASkirmishGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
    Super::GenericPlayerInitialization(NewPlayer);

    ASkirmishPlayerState* SkirmishPlayerState = NewPlayer ? NewPlayer->GetPlayerState<ASkirmishPlayerState>() : nullptr;

    if (!SkirmishPlayerState)
    {
        return;
    }

    if (IsTeamGame())
    {
        const int32 Team = PickTeam();
        SkirmishPlayerState->SetTeamIndex(Team);

        // The team shows as an outline, so the mesh keeps its own material
        SkirmishPlayerState->SetSkinMaterial(nullptr);

        return;
    }

    // All vs All : the Week 4 colour per player
    SkirmishPlayerState->SetTeamIndex(INDEX_NONE);

    if (SkinMaterials.Num() > 0 && SkinMaterials[SkinCount % SkinMaterials.Num()])
    {
        SkirmishPlayerState->SetSkinMaterial(SkinMaterials[SkinCount % SkinMaterials.Num()]);
        SkinCount++;
    }
}

int32 ASkirmishGameMode::PickTeam() const
{
    int32 TeamSizes[2] = { 0, 0 };

    if (GameState)
    {
        for (APlayerState* PlayerState : GameState->PlayerArray)
        {
            const ASkirmishPlayerState* SkirmishPlayerState = Cast<ASkirmishPlayerState>(PlayerState);

            if (SkirmishPlayerState && (SkirmishPlayerState->GetTeamIndex() == 0 || SkirmishPlayerState->GetTeamIndex() == 1))
            {
                TeamSizes[SkirmishPlayerState->GetTeamIndex()]++;
            }
        }
    }

    // Join the smaller team (Red when equal)
    return TeamSizes[1] < TeamSizes[0] ? 1 : 0;
}

bool ASkirmishGameMode::CanDamage(AController* InstigatorController, AController* VictimController) const
{
    if (!IsTeamGame() || !InstigatorController || !VictimController || InstigatorController == VictimController)
    {
        return true;
    }

    const ASkirmishPlayerState* InstigatorState = InstigatorController->GetPlayerState<ASkirmishPlayerState>();
    const ASkirmishPlayerState* VictimState = VictimController->GetPlayerState<ASkirmishPlayerState>();

    if (!InstigatorState || !VictimState)
    {
        return true;
    }

    // No friendly fire
    return InstigatorState->GetTeamIndex() != VictimState->GetTeamIndex();
}

int32 ASkirmishGameMode::GetLeadingTeam() const
{
    const ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (!SkirmishGameState || SkirmishGameState->TeamScores.Num() < 2 || SkirmishGameState->TeamScores[0] == SkirmishGameState->TeamScores[1])
    {
        return INDEX_NONE; // Draw
    }

    return SkirmishGameState->TeamScores[0] > SkirmishGameState->TeamScores[1] ? 0 : 1;
}

void ASkirmishGameMode::EndTeamMatch(int32 WinningTeam)
{
    ASkirmishGameState* SkirmishGameState = GetGameState<ASkirmishGameState>();

    if (SkirmishGameState)
    {
        SkirmishGameState->WinningTeam = WinningTeam;
    }

    EnterPhase(ESkirmishMatchPhase::MatchOver);
}

