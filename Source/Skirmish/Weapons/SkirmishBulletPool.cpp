#include "SkirmishBulletPool.h"
#include "SkirmishBullet.h"

void USkirmishBulletPool::Prewarm(TSubclassOf<ASkirmishBullet> BulletClass, int32 Count)
{
    if (!BulletClass)
    {
        return;
    }

    FSkirmishBulletList& List = SleepingBullets.FindOrAdd(BulletClass);

    while (List.Bullets.Num() < Count)
    {
        ASkirmishBullet* SkirmishBullet = SpawnSleepingBullet(BulletClass);

        if (!SkirmishBullet)
        {
            return;
        }

        List.Bullets.Add(SkirmishBullet);
    }
}

ASkirmishBullet* USkirmishBulletPool::Acquire(TSubclassOf<ASkirmishBullet> BulletClass)
{
    if (!BulletClass)
    {
        return nullptr;
    }

    FSkirmishBulletList& List = SleepingBullets.FindOrAdd(BulletClass);

    while (List.Bullets.Num() > 0)
    {
        ASkirmishBullet* SkirmishBullet = List.Bullets.Pop();

        if (IsValid(SkirmishBullet))
        {
            return SkirmishBullet;
        }
    }

    // The pool ran out - make one more. It joins the pool when it is released
    return SpawnSleepingBullet(BulletClass);
}

void USkirmishBulletPool::Release(ASkirmishBullet* Bullet)
{
    if (!IsValid(Bullet))
    {
        return;
    }

    SleepingBullets.FindOrAdd(Bullet->GetClass()).Bullets.Add(Bullet);
}

ASkirmishBullet* USkirmishBulletPool::SpawnSleepingBullet(TSubclassOf<ASkirmishBullet> BulletClass)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ASkirmishBullet* SkirmishBullet = World->SpawnActor<ASkirmishBullet>(BulletClass, FTransform::Identity, SpawnParameters);

    if (SkirmishBullet)
    {
        SkirmishBullet->Sleep();
    }

    return SkirmishBullet;
}
