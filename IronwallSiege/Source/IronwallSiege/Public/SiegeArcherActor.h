#pragma once

#include "CoreMinimal.h"
#include "SiegeAssetProxyActor.h"
#include "SiegeArcherActor.generated.h"

class ASiegeWorldDirector;

UCLASS()
class IRONWALLSIEGE_API ASiegeArcherActor : public ASiegeAssetProxyActor
{
    GENERATED_BODY()

public:
    ASiegeArcherActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void ConfigureArcher(ESiegeFaction NewFaction, bool bNewHoldPosition);

    UFUNCTION(BlueprintPure, Category="Siege|Combat")
    bool IsArcherConfigured() const { return bConfigured; }

private:
    ASiegeAssetProxyActor* FindNearestRangedTarget(float MaxDistance) const;
    void TickMovement(ASiegeAssetProxyActor* CurrentTarget, float DeltaSeconds);
    void FireArrow(ASiegeAssetProxyActor* CurrentTarget);
    bool IsBattleActive();

    TWeakObjectPtr<ASiegeAssetProxyActor> AttackTarget;
    TWeakObjectPtr<ASiegeWorldDirector> BattleDirector;
    float TargetScanRemaining = 0.0f;
    bool bHoldPosition = false;
    bool bConfigured = false;
    bool bLoggedFirstShot = false;
};
