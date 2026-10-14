// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SkirmishPlayerState.generated.h"

class UMaterialInterface;

UCLASS()
class SKIRMISH_API ASkirmishPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void AddKill() { Kills++; }
    void AddDeath() { Deaths++; }
    void ResetStats() { Kills = 0; Deaths = 0; }
    void SetSkinMaterial(UMaterialInterface* NewMaterial) { SkinMaterial = NewMaterial; }

    int32 GetKills() const { return Kills; }
    int32 GetDeaths() const { return Deaths; }
    UMaterialInterface* GetSkinMaterial() const { return SkinMaterial; }

protected:
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Score")
    int32 Kills = 0;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Score")
    int32 Deaths = 0;

    UPROPERTY(Replicated)
    TObjectPtr<UMaterialInterface> SkinMaterial;
};
