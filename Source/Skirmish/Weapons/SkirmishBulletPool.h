#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SkirmishBulletPool.generated.h"

class ASkirmishBullet;

/** The sleeping bullets of one class */
USTRUCT()
struct FSkirmishBulletList
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<ASkirmishBullet>> Bullets;
};

/** Keeps used bullets and hands them out again, instead of spawning and destroying one per shot */
UCLASS()
class SKIRMISH_API USkirmishBulletPool : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Makes sleeping bullets up front, so the first shots do not spawn anything */
    void Prewarm(TSubclassOf<ASkirmishBullet> BulletClass, int32 Count);

    /** A sleeping bullet of this class - or a new one if the pool has run out */
    ASkirmishBullet* Acquire(TSubclassOf<ASkirmishBullet> BulletClass);

    /** Keeps a finished bullet for the next shot */
    void Release(ASkirmishBullet* Bullet);

private:
    /** One list per bullet class - an AK bullet is not an M4 bullet */
    UPROPERTY()
    TMap<TSubclassOf<ASkirmishBullet>, FSkirmishBulletList> SleepingBullets;

    ASkirmishBullet* SpawnSleepingBullet(TSubclassOf<ASkirmishBullet> BulletClass);
};
