#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlipbookEffectActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

UENUM()
enum class ESailFlipbookEffect : uint8
{
    CannonMuzzle,
    HullImpact,
    WaterImpact
};

UCLASS()
class AGEOFSAILFLEET_API AFlipbookEffectActor : public AActor
{
    GENERATED_BODY()

public:
    AFlipbookEffectActor();
    virtual void Tick(float DeltaSeconds) override;

    void PlayEffect(ESailFlipbookEffect Effect, float WorldSize, float Duration);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> Plane;

    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<UProceduralMeshComponent>> SmokePlanes;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    TArray<FVector> SmokeVertices;
    TArray<int32> SmokeTriangles;
    TArray<FVector> SmokeNormals;
    TArray<FVector2D> SmokeUVs;
    TArray<TArray<FLinearColor>> SmokeBaseColors;
    TArray<float> SmokeDelays;
    TArray<float> SmokeStartRolls;
    TArray<float> SmokeSpinRates;
    TArray<FVector2D> SmokeOffsets;
    TArray<FVector2D> SmokeScales;
    float EffectAge = 0.0f;
    float EffectDuration = 0.9f;
    float FlashDuration = 0.16f;
    int32 CurrentFrame = INDEX_NONE;
    ESailFlipbookEffect ActiveEffect = ESailFlipbookEffect::CannonMuzzle;

    void BuildPowderSmoke();
    void UpdatePowderSmoke();
    void SetFrame(int32 FrameIndex);
};
