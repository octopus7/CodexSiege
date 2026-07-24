#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeProjectileActor.generated.h"

class ASiegeAssetProxyActor;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class IRONWALLSIEGE_API ASiegeProjectileActor : public AActor
{
    GENERATED_BODY()

public:
    ASiegeProjectileActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void Launch(
        const FVector& Start,
        const FVector& End,
        ASiegeAssetProxyActor* NewTarget,
        AActor* NewDamageSource,
        float NewDamage,
        ESiegeFaction NewFaction,
        float NewFlightDuration = 2.2f);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> ProjectileMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ProjectileMaterial;

    TWeakObjectPtr<ASiegeAssetProxyActor> Target;
    TWeakObjectPtr<AActor> DamageSource;
    FVector StartLocation = FVector::ZeroVector;
    FVector EndLocation = FVector::ZeroVector;
    float Damage = 0.0f;
    float FlightDuration = 2.2f;
    float ElapsedTime = 0.0f;
    float ArcHeight = 1350.0f;
    ESiegeFaction Faction = ESiegeFaction::Neutral;
    bool bLaunched = false;
};
