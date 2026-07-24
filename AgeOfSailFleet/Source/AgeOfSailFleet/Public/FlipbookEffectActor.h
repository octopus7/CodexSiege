#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlipbookEffectActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

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
    TObjectPtr<UProceduralMeshComponent> Plane;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    float EffectAge = 0.0f;
    float EffectDuration = 0.9f;
    int32 CurrentFrame = INDEX_NONE;

    void SetFrame(int32 FrameIndex);
};
