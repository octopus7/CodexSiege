#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DefenderArtilleryWorldSubsystem.generated.h"

UCLASS()
class IRONWALLSIEGE_API UDefenderArtilleryWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
