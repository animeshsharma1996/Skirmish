#include "SkirmishHUDWidget.h"
#include "Components/TextBlock.h"
#include "../Core/SkirmishEventSubsystem.h"
#include "../Core/SkirmishGameState.h"

void USkirmishHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    // Subscribe to the event bus (Bind Event to On Elimination)
    if (USkirmishEventSubsystem* Events = World->GetSubsystem<USkirmishEventSubsystem>())
    {
        Events->OnElimination.AddDynamic(this, &USkirmishHUDWidget::HandleElimination);
    }

    // Refresh the phase and scoreboard 4 times per second
    World->GetTimerManager().SetTimer(RefreshTimer, this, &USkirmishHUDWidget::UpdateMatchInfo, 0.25f, true);
    UpdateMatchInfo();
}

void USkirmishHUDWidget::NativeDestruct()
{
    UWorld* World = GetWorld();

    if (World)
    {
        World->GetTimerManager().ClearTimer(RefreshTimer);

        // Unsubscribe so the bus never calls a destroyed widget
        if (USkirmishEventSubsystem* Events = World->GetSubsystem<USkirmishEventSubsystem>())
        {
            Events->OnElimination.RemoveDynamic(this, &USkirmishHUDWidget::HandleElimination);
        }
    }

    Super::NativeDestruct();
}

void USkirmishHUDWidget::UpdateMatchInfo()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    // May be null for a moment on clients, before the GameState has replicated
    const ASkirmishGameState* SkirmishGameState = World->GetGameState<ASkirmishGameState>();

    if (!SkirmishGameState)
    {
        return;
    }

    if (PhaseText)
    {
        PhaseText->SetText(FText::FromString(SkirmishGameState->GetPhaseText()));
    }

    if (ScoreboardText)
    {
        ScoreboardText->SetText(FText::FromString(SkirmishGameState->GetScoreboardText()));
    }
}

void USkirmishHUDWidget::HandleElimination(const FString& KillerName, const FString& VictimName)
{
    // Newest message at the top
    FeedLines.Insert(FString::Printf(TEXT("%s eliminated %s"), *KillerName, *VictimName), 0);

    if (FeedLines.Num() > MaxFeedLines)
    {
        FeedLines.SetNum(MaxFeedLines);
    }

    if (KillFeedText)
    {
        KillFeedText->SetText(FText::FromString(FString::Join(FeedLines, TEXT("\n"))));
    }
}
