#include "ShipWakeActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "SailShip.h"

AShipWakeActor::AShipWakeActor()
{
    PrimaryActorTick.bCanEverTick = true;
    WakeMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WakeMesh"));
    SetRootComponent(WakeMesh);
    WakeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WakeMesh->SetCastShadow(false);
}

void AShipWakeActor::BeginPlay()
{
    Super::BeginPlay();
    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Materials/M_ShipWake.M_ShipWake"));
    if (!BaseMaterial)
    {
        BaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }
    WakeMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    WakeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.68f, 0.91f, 0.96f));
}

void AShipWakeActor::FollowShip(ASailShip* InShip)
{
    Ship = InShip;
}

void AShipWakeActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (FWakePoint& Point : Points)
    {
        Point.Age += DeltaSeconds;
    }
    Points.RemoveAll([](const FWakePoint& Point)
    {
        return Point.Age > 7.0f;
    });

    ASailShip* FollowedShip = Ship.Get();
    if (!FollowedShip || !FollowedShip->IsAfloat())
    {
        RebuildWake();
        return;
    }

    SampleTime -= DeltaSeconds;
    if (SampleTime <= 0.0f && FollowedShip->GetCurrentSpeed() > 55.0f)
    {
        SampleTime = 0.18f;
        FWakePoint Point;
        Point.Center =
            FollowedShip->GetActorLocation() -
            FollowedShip->GetActorForwardVector() * 850.0f +
            FVector(0.0f, 0.0f, 8.0f);
        Point.Right = FollowedShip->GetActorRightVector();
        Point.Width = FMath::Lerp(170.0f, 430.0f, FollowedShip->GetCurrentSpeed() / 620.0f);
        Points.Insert(Point, 0);
    }
    RebuildWake();
}

void AShipWakeActor::RebuildWake()
{
    if (Points.Num() < 2)
    {
        WakeMesh->ClearAllMeshSections();
        return;
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    Vertices.Reserve(Points.Num() * 4);

    for (int32 Index = 0; Index < Points.Num(); ++Index)
    {
        const FWakePoint& Point = Points[Index];
        const float Fade = FMath::Square(1.0f - FMath::Clamp(Point.Age / 7.0f, 0.0f, 1.0f));
        const float Spread = Point.Width + Point.Age * 72.0f;
        const float RibbonHalfWidth = 32.0f + Point.Age * 12.0f;
        for (int32 Side : {-1, 1})
        {
            const FVector RibbonCenter = Point.Center + Point.Right * static_cast<float>(Side) * Spread;
            Vertices.Add(RibbonCenter - Point.Right * RibbonHalfWidth);
            Vertices.Add(RibbonCenter + Point.Right * RibbonHalfWidth);
            Normals.Append({FVector::UpVector, FVector::UpVector});
            UVs.Append({
                FVector2D(0.0f, static_cast<float>(Index) * 0.25f),
                FVector2D(1.0f, static_cast<float>(Index) * 0.25f)
            });
            const FLinearColor FoamColor(0.52f * Fade, 0.82f * Fade, 0.92f * Fade, Fade);
            Colors.Append({FoamColor, FoamColor});
        }
    }

    for (int32 Index = 0; Index < Points.Num() - 1; ++Index)
    {
        for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
        {
            const int32 A = Index * 4 + SideIndex * 2;
            const int32 B = A + 1;
            const int32 C = A + 4;
            const int32 D = C + 1;
            Triangles.Append({A, C, B, B, C, D});
        }
    }

    WakeMesh->ClearAllMeshSections();
    WakeMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        Colors,
        {},
        false);
    WakeMesh->SetMaterial(0, WakeMaterial);
}
