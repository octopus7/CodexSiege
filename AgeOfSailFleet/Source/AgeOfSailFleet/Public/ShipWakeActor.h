#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipWakeActor.generated.h"

class ASailShip;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

UCLASS()
class AGEOFSAILFLEET_API AShipWakeActor : public AActor
{
    GENERATED_BODY()

public:
    AShipWakeActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void FollowShip(ASailShip* InShip);

private:
    struct FWakePoint
    {
        FVector Center = FVector::ZeroVector;
        FVector Right = FVector::RightVector;
        float Age = 0.0f;
        float Width = 100.0f;
    };

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> WakeMesh;

    UPROPERTY()
    TWeakObjectPtr<ASailShip> Ship;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> WakeMaterial;

    TArray<FWakePoint> Points;
    float SampleTime = 0.0f;

    void RebuildWake();
};
