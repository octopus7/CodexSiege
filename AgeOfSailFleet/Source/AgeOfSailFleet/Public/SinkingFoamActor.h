#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SinkingFoamActor.generated.h"

class ASailShip;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USceneComponent;

UCLASS()
class AGEOFSAILFLEET_API ASinkingFoamActor : public AActor
{
    GENERATED_BODY()

public:
    ASinkingFoamActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void FollowSinkingShip(ASailShip* InShip, int32 InShipRate);

private:
    struct FFoamPatchState
    {
        FVector LocalCenter = FVector::ZeroVector;
        FVector2D BaseSize = FVector2D(300.0f, 150.0f);
        float Age = 0.0f;
        float Duration = 2.0f;
        float StartRotation = 0.0f;
        float RotationSpeed = 0.0f;
        float Growth = 0.7f;
        float OutlineAngle = 0.0f;
        float OutlineJitter = 1.0f;
    };

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<UProceduralMeshComponent>> FoamPatches;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FoamMaterial;

    UPROPERTY()
    TWeakObjectPtr<ASailShip> Ship;

    TArray<FFoamPatchState> PatchStates;
    FRandomStream RandomStream;
    float EffectAge = 0.0f;
    float HullHalfLength = 1080.0f;
    float HullHalfWidth = 350.0f;
    float HullHalfHeight = 275.0f;
    float NoContactAge = 0.0f;
    float ContactFade = 1.0f;

    static constexpr int32 PatchCount = 16;
    static constexpr float RepeatDuration = 13.5f;
    static constexpr float TotalDuration = 15.5f;
    static constexpr float SurfaceHeight = 14.0f;

    void CreatePatchMesh(UProceduralMeshComponent* Patch);
    void ResetPatch(int32 PatchIndex, bool bInitialPhase);
    void UpdatePatch(int32 PatchIndex, float DeltaSeconds);
    void UpdateSurfaceTransform();
    bool UpdateContactOutline();
};
