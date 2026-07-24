#include "SailOceanActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace SailOcean
{
    // 90,000uu across, leaving a broad visual margin around the 60,000uu
    // playable ship area even at the fleet-level camera zoom.
    constexpr int32 Resolution = 61;
    constexpr float CellSize = 1500.0f;
}

ASailOceanActor::ASailOceanActor()
{
    PrimaryActorTick.bCanEverTick = true;
    OceanMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OceanMesh"));
    SetRootComponent(OceanMesh);
    OceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OceanMesh->SetCastShadow(false);
}

void ASailOceanActor::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_ProceduralOcean.M_ProceduralOcean"));
    if (!BaseMaterial)
    {
        BaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }
    OceanMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    OceanMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.018f, 0.16f, 0.23f));
    OceanMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.28f);
    BuildOcean();
}

void ASailOceanActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    OceanTime += DeltaSeconds;
    UpdateWaves();
}

void ASailOceanActor::BuildOcean()
{
    const float HalfExtent =
        static_cast<float>(SailOcean::Resolution - 1) * SailOcean::CellSize * 0.5f;
    Vertices.Reserve(SailOcean::Resolution * SailOcean::Resolution);
    Normals.Reserve(SailOcean::Resolution * SailOcean::Resolution);
    UVs.Reserve(SailOcean::Resolution * SailOcean::Resolution);
    Colors.Reserve(SailOcean::Resolution * SailOcean::Resolution);

    for (int32 Y = 0; Y < SailOcean::Resolution; ++Y)
    {
        for (int32 X = 0; X < SailOcean::Resolution; ++X)
        {
            Vertices.Add(FVector(
                static_cast<float>(X) * SailOcean::CellSize - HalfExtent,
                static_cast<float>(Y) * SailOcean::CellSize - HalfExtent,
                -20.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(
                static_cast<float>(X) / static_cast<float>(SailOcean::Resolution - 1),
                static_cast<float>(Y) / static_cast<float>(SailOcean::Resolution - 1)));
            const float Shade = (X + Y) % 3 == 0 ? 0.018f : 0.0f;
            Colors.Add(FLinearColor(0.01f + Shade, 0.13f + Shade, 0.22f + Shade));
        }
    }
    for (int32 Y = 0; Y < SailOcean::Resolution - 1; ++Y)
    {
        for (int32 X = 0; X < SailOcean::Resolution - 1; ++X)
        {
            const int32 A = Y * SailOcean::Resolution + X;
            const int32 B = A + 1;
            const int32 C = A + SailOcean::Resolution;
            const int32 D = C + 1;
            Triangles.Append({A, C, B, B, C, D});
        }
    }
    OceanMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        Colors,
        {},
        false);
    OceanMesh->SetMaterial(0, OceanMaterial);
}

void ASailOceanActor::UpdateWaves()
{
    const float HalfExtent =
        static_cast<float>(SailOcean::Resolution - 1) * SailOcean::CellSize * 0.5f;
    for (int32 Y = 0; Y < SailOcean::Resolution; ++Y)
    {
        for (int32 X = 0; X < SailOcean::Resolution; ++X)
        {
            const int32 Index = Y * SailOcean::Resolution + X;
            const float WorldX = static_cast<float>(X) * SailOcean::CellSize - HalfExtent;
            const float WorldY = static_cast<float>(Y) * SailOcean::CellSize - HalfExtent;
            Vertices[Index].Z =
                -24.0f +
                FMath::Sin(WorldX * 0.00115f + OceanTime * 0.72f) * 22.0f +
                FMath::Sin(WorldY * 0.0017f + OceanTime * 0.93f) * 12.0f +
                FMath::Sin((WorldX + WorldY) * 0.0022f + OceanTime * 1.35f) * 5.0f;

            const float Dx =
                FMath::Cos(WorldX * 0.00115f + OceanTime * 0.72f) * 22.0f * 0.00115f +
                FMath::Cos((WorldX + WorldY) * 0.0022f + OceanTime * 1.35f) * 5.0f * 0.0022f;
            const float Dy =
                FMath::Cos(WorldY * 0.0017f + OceanTime * 0.93f) * 12.0f * 0.0017f +
                FMath::Cos((WorldX + WorldY) * 0.0022f + OceanTime * 1.35f) * 5.0f * 0.0022f;
            Normals[Index] = FVector(-Dx, -Dy, 1.0f).GetSafeNormal();
            const float Crest = FMath::Clamp((Vertices[Index].Z + 20.0f) / 48.0f, 0.0f, 1.0f);
            Colors[Index] = FLinearColor(
                0.01f + Crest * 0.05f,
                0.11f + Crest * 0.17f,
                0.20f + Crest * 0.18f);
        }
    }
    OceanMesh->UpdateMeshSection_LinearColor(0, Vertices, Normals, UVs, Colors, {});
}
