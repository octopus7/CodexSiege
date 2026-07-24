#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SailOceanActor.generated.h"

class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

UCLASS()
class AGEOFSAILFLEET_API ASailOceanActor : public AActor
{
    GENERATED_BODY()

public:
    ASailOceanActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> OceanMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> OceanMaterial;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    float OceanTime = 0.0f;

    void BuildOcean();
    void UpdateWaves();
};
