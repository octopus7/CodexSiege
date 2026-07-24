#include "FlipbookEffectActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

AFlipbookEffectActor::AFlipbookEffectActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Plane = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Plane"));
    Plane->SetupAttachment(SceneRoot);
    Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Plane->SetCastShadow(false);
    Plane->SetBoundsScale(2.0f);

    constexpr int32 SmokeLayerCount = 4;
    SmokePlanes.Reserve(SmokeLayerCount);
    for (int32 LayerIndex = 0; LayerIndex < SmokeLayerCount; ++LayerIndex)
    {
        UProceduralMeshComponent* SmokePlane =
            CreateDefaultSubobject<UProceduralMeshComponent>(
                *FString::Printf(TEXT("PowderSmoke%d"), LayerIndex));
        SmokePlane->SetupAttachment(SceneRoot);
        SmokePlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SmokePlane->SetCastShadow(false);
        SmokePlane->SetBoundsScale(3.0f);
        SmokePlane->SetTranslucentSortPriority(LayerIndex + 1);
        SmokePlane->SetVisibility(false, true);
        SmokePlanes.Add(SmokePlane);
    }

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

void AFlipbookEffectActor::BuildPowderSmoke()
{
    constexpr int32 SegmentCount = 16;
    SmokeVertices.Reset(1 + SegmentCount * 2);
    SmokeTriangles.Reset(SegmentCount * 9);
    SmokeNormals.Reset(1 + SegmentCount * 2);
    SmokeUVs.Reset(1 + SegmentCount * 2);

    SmokeVertices.Add(FVector::ZeroVector);
    SmokeNormals.Add(FVector::ForwardVector);
    SmokeUVs.Add(FVector2D(0.5f, 0.5f));
    for (int32 Ring = 0; Ring < 2; ++Ring)
    {
        const float RingScale = Ring == 0 ? 0.52f : 1.0f;
        for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
        {
            const float Angle =
                2.0f * PI * static_cast<float>(Segment) /
                static_cast<float>(SegmentCount);
            const float Irregularity =
                1.0f +
                0.13f * FMath::Sin(Angle * 3.0f + 0.7f) +
                0.09f * FMath::Sin(Angle * 7.0f + 1.9f);
            const float Radius = 50.0f * RingScale * Irregularity;
            const float Y = FMath::Cos(Angle) * Radius;
            const float Z = FMath::Sin(Angle) * Radius;
            SmokeVertices.Add(FVector(0.0f, Y, Z));
            SmokeNormals.Add(FVector::ForwardVector);
            SmokeUVs.Add(FVector2D(Y / 100.0f + 0.5f, 0.5f - Z / 100.0f));
        }
    }

    const int32 InnerStart = 1;
    const int32 OuterStart = 1 + SegmentCount;
    for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
    {
        const int32 Next = (Segment + 1) % SegmentCount;
        const int32 Inner = InnerStart + Segment;
        const int32 InnerNext = InnerStart + Next;
        const int32 Outer = OuterStart + Segment;
        const int32 OuterNext = OuterStart + Next;
        SmokeTriangles.Append({
            0, Inner, InnerNext,
            Inner, Outer, OuterNext,
            Inner, OuterNext, InnerNext
        });
    }

    SmokeBaseColors.Reset(SmokePlanes.Num());
    SmokeDelays.Reset(SmokePlanes.Num());
    SmokeStartRolls.Reset(SmokePlanes.Num());
    SmokeSpinRates.Reset(SmokePlanes.Num());
    SmokeOffsets.Reset(SmokePlanes.Num());
    SmokeScales.Reset(SmokePlanes.Num());

    FRandomStream RandomStream(GetUniqueID() * 196613 + 17);
    UMaterialInterface* SmokeMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_ShipWake.M_ShipWake"));
    for (int32 LayerIndex = 0; LayerIndex < SmokePlanes.Num(); ++LayerIndex)
    {
        const float Shade = RandomStream.FRandRange(0.82f, 1.08f);
        const FLinearColor SmokeColor(
            0.14f * Shade,
            0.125f * Shade,
            0.105f * Shade,
            1.0f);
        TArray<FLinearColor>& LayerColors = SmokeBaseColors.AddDefaulted_GetRef();
        LayerColors.Reserve(SmokeVertices.Num());
        LayerColors.Add(FLinearColor(
            SmokeColor.R,
            SmokeColor.G,
            SmokeColor.B,
            0.26f));
        for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
        {
            const float Density =
                0.56f + RandomStream.FRandRange(-0.08f, 0.08f);
            LayerColors.Add(FLinearColor(
                SmokeColor.R,
                SmokeColor.G,
                SmokeColor.B,
                Density));
        }
        for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
        {
            LayerColors.Add(FLinearColor(
                SmokeColor.R,
                SmokeColor.G,
                SmokeColor.B,
                0.0f));
        }

        UProceduralMeshComponent* SmokePlane = SmokePlanes[LayerIndex];
        SmokePlane->CreateMeshSection_LinearColor(
            0,
            SmokeVertices,
            SmokeTriangles,
            SmokeNormals,
            SmokeUVs,
            LayerColors,
            {},
            false);
        if (SmokeMaterial)
        {
            SmokePlane->SetMaterial(0, SmokeMaterial);
        }
        SmokePlane->SetVisibility(true, true);

        SmokeDelays.Add(static_cast<float>(LayerIndex) * 0.055f);
        SmokeStartRolls.Add(RandomStream.FRandRange(-175.0f, 175.0f));
        SmokeSpinRates.Add(
            RandomStream.FRandRange(24.0f, 58.0f) *
            (RandomStream.RandRange(0, 1) == 0 ? -1.0f : 1.0f));
        SmokeOffsets.Add(FVector2D(
            RandomStream.FRandRange(-14.0f, 14.0f),
            RandomStream.FRandRange(-7.0f, 10.0f)));
        SmokeScales.Add(FVector2D(
            RandomStream.FRandRange(0.52f, 0.78f),
            RandomStream.FRandRange(0.42f, 0.64f)));
    }
}

void AFlipbookEffectActor::PlayEffect(
    const ESailFlipbookEffect Effect,
    const float WorldSize,
    const float Duration)
{
    ActiveEffect = Effect;
    EffectAge = 0.0f;
    EffectDuration =
        Effect == ESailFlipbookEffect::HullImpact
            ? FMath::Max(1.65f, Duration * 1.65f)
            : FMath::Max(0.1f, Duration);
    FlashDuration = FMath::Min(0.17f, EffectDuration * 0.11f);
    // The authored quad is 100uu wide. Call sites pass a size multiplier
    // (roughly 3-6), producing a readable 300-600uu battle effect. Dividing
    // by 100 here previously reduced the flipbooks to only a few world units.
    SetActorScale3D(FVector(FMath::Max(0.1f, WorldSize)));
    SetLifeSpan(EffectDuration + 0.15f);
    Plane->SetVisibility(true, true);
    Plane->SetRelativeScale3D(
        Effect == ESailFlipbookEffect::HullImpact
            ? FVector(1.0f, 0.48f, 0.48f)
            : FVector::OneVector);
    for (UProceduralMeshComponent* SmokePlane : SmokePlanes)
    {
        SmokePlane->SetVisibility(false, true);
    }

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
    if (Effect == ESailFlipbookEffect::HullImpact)
    {
        BuildPowderSmoke();
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

    if (ActiveEffect == ESailFlipbookEffect::HullImpact)
    {
        if (EffectAge < FlashDuration)
        {
            const int32 FlashFrame = FMath::Clamp(
                FMath::FloorToInt(EffectAge / FlashDuration * 4.0f),
                0,
                3);
            Plane->SetVisibility(true, true);
            SetFrame(FlashFrame);
        }
        else
        {
            Plane->SetVisibility(false, true);
        }
        UpdatePowderSmoke();
    }
    else
    {
        const int32 Frame = FMath::Clamp(
            FMath::FloorToInt(EffectAge / EffectDuration * 16.0f),
            0,
            15);
        SetFrame(Frame);
    }

    if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        SetActorRotation((CameraManager->GetCameraLocation() - GetActorLocation()).Rotation());
    }
}

void AFlipbookEffectActor::UpdatePowderSmoke()
{
    for (int32 LayerIndex = 0; LayerIndex < SmokePlanes.Num(); ++LayerIndex)
    {
        const float LayerDuration =
            FMath::Max(0.1f, EffectDuration - SmokeDelays[LayerIndex]);
        const float LayerTime = FMath::Clamp(
            (EffectAge - SmokeDelays[LayerIndex]) / LayerDuration,
            0.0f,
            1.0f);
        UProceduralMeshComponent* SmokePlane = SmokePlanes[LayerIndex];
        SmokePlane->SetVisibility(EffectAge >= SmokeDelays[LayerIndex], true);
        if (EffectAge < SmokeDelays[LayerIndex])
        {
            continue;
        }

        const float FastExpansion =
            1.0f - FMath::Exp(-5.0f * LayerTime);
        const FVector2D BaseScale = SmokeScales[LayerIndex];
        const float LayerBias = 1.0f + static_cast<float>(LayerIndex) * 0.055f;
        SmokePlane->SetRelativeScale3D(FVector(
            1.0f,
            BaseScale.X * FMath::Lerp(0.34f, 1.34f, FastExpansion) * LayerBias,
            BaseScale.Y * FMath::Lerp(0.30f, 1.48f, FastExpansion) * LayerBias));
        SmokePlane->SetRelativeLocation(FVector(
            static_cast<float>(LayerIndex) * 0.12f,
            SmokeOffsets[LayerIndex].X,
            SmokeOffsets[LayerIndex].Y + 42.0f * LayerTime));
        SmokePlane->SetRelativeRotation(FRotator(
            0.0f,
            0.0f,
            SmokeStartRolls[LayerIndex] +
            SmokeSpinRates[LayerIndex] * LayerTime));

        const float FadeIn = FMath::Clamp(LayerTime / 0.09f, 0.0f, 1.0f);
        const float FadeOut = FMath::Pow(1.0f - LayerTime, 1.35f);
        const float Alpha = FadeIn * FadeOut;
        TArray<FLinearColor> FadedColors = SmokeBaseColors[LayerIndex];
        for (FLinearColor& Color : FadedColors)
        {
            Color.A *= Alpha;
        }
        SmokePlane->UpdateMeshSection_LinearColor(
            0,
            SmokeVertices,
            SmokeNormals,
            SmokeUVs,
            FadedColors,
            {});
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
