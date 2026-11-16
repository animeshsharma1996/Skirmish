#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkirmishLagCompensation.generated.h"

class ASkirmishCharacter;
class ASkirmishWeapon;

/** Where every hitbox bone was at one moment */
USTRUCT()
struct FSkirmishHitboxFrame
{
    GENERATED_BODY()

    double Time = 0.0;
    TArray<FVector> BoneLocations;
};

/** A shot the server accepted, waiting for the shooter to report a hit */
USTRUCT()
struct FSkirmishShot
{
    GENERATED_BODY()

    int32 Id = INDEX_NONE;
    double RewindTime = 0.0; // The moment the shooter was seeing when they fired
    FVector Start = FVector::ZeroVector;
    FVector Direction = FVector::ForwardVector;
    TWeakObjectPtr<ASkirmishWeapon> Weapon;
    bool bUsed = false;
};

/** Server-side rewind : keeps a short history of this character's hitboxes, and checks the hits its owner reports */
UCLASS(ClassGroup = (Skirmish), meta = (BlueprintSpawnableComponent))
class SKIRMISH_API USkirmishLagCompensation : public UActorComponent
{
    GENERATED_BODY()

public:
    USkirmishLagCompensation();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Server : remembers a shot, rewound by the shooter's ping */
    void RecordShot(int32 ShotId, const FVector& Start, const FVector& Direction, ASkirmishWeapon* Weapon);

    /** Shooter : my bullet hit this character - the server decides if it counts */
    UFUNCTION(Server, Reliable)
    void ServerReportHit(int32 ShotId, ASkirmishCharacter* HitCharacter, FName BoneName, FVector_NetQuantize HitLocation);

    /** Server : where a bone was at a moment in the past */
    bool GetRewoundBoneLocation(FName BoneName, double Time, FVector& OutLocation) const;

protected:
    virtual void BeginPlay() override;

    /** How far back the server can rewind - shots from slower connections are not compensated */
    UPROPERTY(EditDefaultsOnly, Category = "Lag Compensation")
    float MaxRewindTime = 0.5f;

    /** How far a reported hit may be from the line the server accepted for the shot */
    UPROPERTY(EditDefaultsOnly, Category = "Lag Compensation")
    float MaxDistanceFromShot = 30.f;

    /** How far a reported hit may be from the rewound bone - about the length of the biggest body */
    UPROPERTY(EditDefaultsOnly, Category = "Lag Compensation")
    float MaxDistanceFromBone = 60.f;

private:
    TArray<FName> HitboxBones;
    TArray<FSkirmishHitboxFrame> History;
    TArray<FSkirmishShot> Shots;

    void RecordHitboxes();
    bool IsHitValid(const FSkirmishShot& Shot, ASkirmishCharacter* HitCharacter, FName BoneName, const FVector& HitLocation) const;
};
