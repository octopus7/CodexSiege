#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeDefenderArtilleryActor.generated.h"

class ASiegeAssetProxyActor;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class IRONWALLSIEGE_API ASiegeDefenderArtilleryActor : public AActor
{
    GENERATED_BODY()

public:
    ASiegeDefenderArtilleryActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void InitializeBattery(int32 NewBatteryIndex);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Carriage;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LeftWheel;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> RightWheel;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> AimPivot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Barrel;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MuzzleFlash;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> FlashLight;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CarriageMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> MetalMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;

    TWeakObjectPtr<ASiegeAssetProxyActor> CurrentTarget;
    int32 BatteryIndex = 0;
    float TargetScanRemaining = 0.0f;
    float FireCooldownRemaining = 0.0f;
    float RecoilRemaining = 0.0f;
    bool bPositionedOnWall = false;
    bool bObservedBattleStart = false;

    bool TryPositionOnWall();
    bool IsBattleActive() const;
    ASiegeAssetProxyActor* FindTarget() const;
    void AimAtTarget(const ASiegeAssetProxyActor& Target, float DeltaSeconds);
    void FireAt(ASiegeAssetProxyActor& Target);
    void UpdateFireFeedback(float DeltaSeconds);
};
