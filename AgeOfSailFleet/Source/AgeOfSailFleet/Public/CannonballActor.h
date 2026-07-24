#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CannonballActor.generated.h"

class ASailShip;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class AGEOFSAILFLEET_API ACannonballActor : public AActor
{
    GENERATED_BODY()

public:
    ACannonballActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void Launch(
        ASailShip* InSourceShip,
        const FVector& Velocity,
        float InDamage,
        float InVisualScale = 1.0f);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> BallLight;

    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<UStaticMeshComponent>> TrailSegments;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BallMaterial;

    UPROPERTY()
    TWeakObjectPtr<ASailShip> SourceShip;

    float Damage = 45.0f;
    float VisualScale = 1.0f;
    float TrailSampleAccumulator = 0.0f;
    bool bImpactPlayed = false;
    TArray<FVector> TrailPositions;

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);

    void SpawnWaterImpact(const FVector& ImpactLocation);
    void UpdateTrail(float DeltaSeconds);
};
