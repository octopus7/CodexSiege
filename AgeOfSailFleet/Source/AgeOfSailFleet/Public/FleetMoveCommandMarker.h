#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FleetMoveCommandMarker.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Short-lived world-space acknowledgement for a fleet move command.
 *
 * Eight horizontal arrows converge on the ordered position while descending
 * to the water. The marker is presentation-only and never participates in
 * collision or ship movement.
 */
UCLASS()
class AGEOFSAILFLEET_API AFleetMoveCommandMarker : public AActor
{
    GENERATED_BODY()

public:
    AFleetMoveCommandMarker();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> ArrowShafts;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> ArrowHeads;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MarkerMaterial;

    float EffectAge = 0.0f;

    void UpdateMarker(float NormalizedAge);
};
