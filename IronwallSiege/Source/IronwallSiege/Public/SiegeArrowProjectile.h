#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeArrowProjectile.generated.h"

class ASiegeAssetProxyActor;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class IRONWALLSIEGE_API ASiegeArrowProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASiegeArrowProjectile();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void Launch(
        const FVector& Start,
        ASiegeAssetProxyActor* NewTarget,
        AActor* NewDamageSource,
        float NewDamage,
        ESiegeFaction NewFaction,
        float NewFlightDuration,
        bool bNewEmitImpactLog = false);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> ArrowMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ArrowMaterial;

    TWeakObjectPtr<ASiegeAssetProxyActor> Target;
    TWeakObjectPtr<AActor> DamageSource;
    FVector StartLocation = FVector::ZeroVector;
    FVector TargetLocation = FVector::ZeroVector;
    FVector PreviousLocation = FVector::ZeroVector;
    float Damage = 0.0f;
    float FlightDuration = 0.8f;
    float ElapsedTime = 0.0f;
    ESiegeFaction Faction = ESiegeFaction::Neutral;
    bool bLaunched = false;
    bool bEmitImpactLog = false;
};
