#include "FlipbookEffectActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

AFlipbookEffectActor::AFlipbookEffectActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Plane = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Plane"));
    SetRootComponent(Plane);
    Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Plane->SetCastShadow(false);
    Plane->SetBoundsScale(2.0f);

    Vertices = {
        FVector(0.0f, -50.0f, -50.0f),
        FVector(0.0f, 50.0f, -50.0f),
        FVector(0.0f, 50.0f, 50.0f),
        FVector(0.0f, -50.0f, 50.0f)
    };
    Triangles = {0, 1, 2, 0, 2, 3};
    Normals.Init(FVector::ForwardVector, 4);
    Colors.Init(FLinearColor::White, 4);
    UVs.Init(FVector2D::ZeroVector, 4);
}

void AFlipbookEffectActor::PlayEffect(
    const ESailFlipbookEffect Effect,
    const float WorldSize,
    const float Duration)
{
    EffectAge = 0.0f;
    EffectDuration = FMath::Max(0.1f, Duration);
    // The authored quad is 100uu wide. Call sites pass a size multiplier
    // (roughly 3-6), producing a readable 300-600uu battle effect. Dividing
    // by 100 here previously reduced the flipbooks to only a few world units.
    SetActorScale3D(FVector(FMath::Max(0.1f, WorldSize)));
    SetLifeSpan(EffectDuration + 0.15f);
    Plane->SetVisibility(true, true);

    const TCHAR* MaterialPath =
        Effect == ESailFlipbookEffect::CannonMuzzle
            ? TEXT("/Game/Art/Materials/M_FX_CannonMuzzle.M_FX_CannonMuzzle")
            : Effect == ESailFlipbookEffect::HullImpact
                ? TEXT("/Game/Art/Materials/M_FX_HullImpact.M_FX_HullImpact")
                : TEXT("/Game/Art/Materials/M_FX_WaterImpact.M_FX_WaterImpact");
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath))
    {
        Plane->SetMaterial(0, Material);
    }
    SetFrame(0);

    // Face the first frame toward the camera immediately instead of waiting
    // for the first tick, which can otherwise show a broadside flash edge-on.
    if (APlayerCameraManager* CameraManager =
        UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        SetActorRotation(
            (CameraManager->GetCameraLocation() - GetActorLocation()).Rotation());
    }
}

void AFlipbookEffectActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    EffectAge += DeltaSeconds;
    if (EffectAge >= EffectDuration)
    {
        Destroy();
        return;
    }

    const int32 Frame = FMath::Clamp(
        FMath::FloorToInt(EffectAge / EffectDuration * 16.0f),
        0,
        15);
    SetFrame(Frame);

    if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        SetActorRotation((CameraManager->GetCameraLocation() - GetActorLocation()).Rotation());
    }
}

void AFlipbookEffectActor::SetFrame(const int32 FrameIndex)
{
    if (FrameIndex == CurrentFrame)
    {
        return;
    }
    CurrentFrame = FrameIndex;
    const int32 Column = FrameIndex % 4;
    const int32 Row = FrameIndex / 4;
    const float U0 = static_cast<float>(Column) * 0.25f;
    const float V0 = static_cast<float>(Row) * 0.25f;
    const float U1 = U0 + 0.25f;
    const float V1 = V0 + 0.25f;
    UVs = {
        FVector2D(U0, V1),
        FVector2D(U1, V1),
        FVector2D(U1, V0),
        FVector2D(U0, V0)
    };

    if (Plane->GetNumSections() == 0)
    {
        Plane->CreateMeshSection_LinearColor(
            0,
            Vertices,
            Triangles,
            Normals,
            UVs,
            Colors,
            {},
            false);
    }
    else
    {
        Plane->UpdateMeshSection_LinearColor(0, Vertices, Normals, UVs, Colors, {});
    }
}
