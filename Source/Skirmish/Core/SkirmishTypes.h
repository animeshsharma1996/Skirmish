#pragma once

#include "CoreMinimal.h"
#include "SkirmishTypes.generated.h"

UENUM(BlueprintType)
enum class ESkirmishGameType : uint8
{
    AllVsAll,
    Team
};

UENUM(BlueprintType)
enum class ESkirmishCameraMode : uint8
{
    ThirdPerson,
    FirstPerson
};

UENUM(BlueprintType)
enum class ESkirmishHitDirection : uint8
{
    Front,
    Back,
    Left,
    Right
};

UENUM(BlueprintType)
enum class ESkirmishHitZone : uint8
{
    Head,
    Torso,
    Legs
};

/** One game found by a search - what a server browser row shows */
USTRUCT(BlueprintType)
struct FSkirmishSessionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Online")
    int32 Index = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Online")
    FString HostName;

    UPROPERTY(BlueprintReadOnly, Category = "Online")
    ESkirmishGameType GameType = ESkirmishGameType::AllVsAll;

    UPROPERTY(BlueprintReadOnly, Category = "Online")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Online")
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Online")
    int32 PingMs = 0;
};
