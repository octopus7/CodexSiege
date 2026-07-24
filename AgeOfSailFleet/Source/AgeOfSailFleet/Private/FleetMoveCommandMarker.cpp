#include "FleetMoveCommandMarker.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace FleetMoveMarker
{
    constexpr int32 ArrowCount = 8;
    constexpr float Duration = 1.15f;
    constexpr float StartRadius = 950.0f;
    constexpr float EndRadius = 130.0f;
    constexpr float StartHeight = 660.0f;
    constexpr float EndHeight = 34.0f;
    constexpr float ShaftLength = 265.0f;
    constexpr float HeadLength = 92.0f;
    constexpr float ShaftThickness = 0.24f;
    constexpr float HeadThickness = 0.62f;
    constexpr float FadeStart = 0.68f;
}

AFleetMoveCommandMarker::AFleetMoveCommandMarker()
{
    PrimaryActorTick.bCanEverTick = true;
    InitialLifeSpan = FleetMoveMarker::Duration + 0.08f;
    SetActorEnableCollision(false);

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(
        TEXT("/Engine/BasicShapes/Cone.Cone"));

    ArrowShafts.Reserve(FleetMoveMarker::ArrowCount);
    ArrowHeads.Reserve(FleetMoveMarker::ArrowCount);
    for (int32 Index = 0; Index < FleetMoveMarker::ArrowCount; ++Index)
    {
        UStaticMeshComponent* Shaft = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("ArrowShaft_%02d"), Index));
        Shaft->SetupAttachment(Root);
        Shaft->SetStaticMesh(CylinderFinder.Object);
        Shaft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shaft->SetCastShadow(false);
        ArrowShafts.Add(Shaft);

        UStaticMeshComponent* Head = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("ArrowHead_%02d"), Index));
        Head->SetupAttachment(Root);
        Head->SetStaticMesh(ConeFinder.Object);
        Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Head->SetCastShadow(false);
        ArrowHeads.Add(Head);
    }
}

void AFleetMoveCommandMarker::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BaseMaterial)
    {
        MarkerMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        MarkerMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(0.08f, 0.42f, 1.0f, 1.0f));
        for (UStaticMeshComponent* Shaft : ArrowShafts)
        {
            if (Shaft)
            {
                Shaft->SetMaterial(0, MarkerMaterial);
            }
        }
        for (UStaticMeshComponent* Head : ArrowHeads)
        {
            if (Head)
            {
                Head->SetMaterial(0, MarkerMaterial);
            }
        }
    }

    UpdateMarker(0.0f);
}

void AFleetMoveCommandMarker::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    EffectAge += DeltaSeconds;
    const float NormalizedAge = FMath::Clamp(
        EffectAge / FleetMoveMarker::Duration,
        0.0f,
        1.0f);
    UpdateMarker(NormalizedAge);
    if (NormalizedAge >= 1.0f)
    {
        Destroy();
    }
}

void AFleetMoveCommandMarker::UpdateMarker(const float NormalizedAge)
{
    const float TravelAlpha = FMath::InterpEaseInOut(
        0.0f,
        1.0f,
        NormalizedAge,
        2.15f);
    const float Radius = FMath::Lerp(
        FleetMoveMarker::StartRadius,
        FleetMoveMarker::EndRadius,
        TravelAlpha);
    const float Height = FMath::Lerp(
        FleetMoveMarker::StartHeight,
        FleetMoveMarker::EndHeight,
        TravelAlpha);
    const float FadeAlpha = FMath::Clamp(
        (NormalizedAge - FleetMoveMarker::FadeStart) /
            (1.0f - FleetMoveMarker::FadeStart),
        0.0f,
        1.0f);
    const float DisappearScale = 1.0f - FMath::SmoothStep(0.0f, 1.0f, FadeAlpha);
    const float PulseScale =
        1.0f + FMath::Sin(NormalizedAge * PI) * 0.12f;
    const float VisualScale = DisappearScale * PulseScale;

    if (MarkerMaterial)
    {
        MarkerMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(
                0.08f + 0.32f * (1.0f - NormalizedAge),
                0.42f + 0.30f * (1.0f - NormalizedAge),
                1.0f,
                DisappearScale));
    }

    for (int32 Index = 0; Index < FleetMoveMarker::ArrowCount; ++Index)
    {
        const float AngleRadians =
            2.0f * PI * static_cast<float>(Index) /
            static_cast<float>(FleetMoveMarker::ArrowCount);
        const FVector Radial(
            FMath::Cos(AngleRadians),
            FMath::Sin(AngleRadians),
            0.0f);
        const FVector Inward = -Radial;
        const FRotator ArrowRotation = FRotationMatrix::MakeFromZ(Inward).Rotator();
        const FVector ArrowOrigin = Radial * Radius + FVector(0.0f, 0.0f, Height);

        if (ArrowShafts.IsValidIndex(Index) && ArrowShafts[Index])
        {
            ArrowShafts[Index]->SetRelativeLocation(
                ArrowOrigin + Inward * FleetMoveMarker::ShaftLength * 0.5f);
            ArrowShafts[Index]->SetRelativeRotation(ArrowRotation);
            ArrowShafts[Index]->SetRelativeScale3D(FVector(
                FleetMoveMarker::ShaftThickness * VisualScale,
                FleetMoveMarker::ShaftThickness * VisualScale,
                FleetMoveMarker::ShaftLength / 100.0f * VisualScale));
        }
        if (ArrowHeads.IsValidIndex(Index) && ArrowHeads[Index])
        {
            ArrowHeads[Index]->SetRelativeLocation(
                ArrowOrigin +
                Inward *
                    (FleetMoveMarker::ShaftLength +
                     FleetMoveMarker::HeadLength * 0.5f));
            ArrowHeads[Index]->SetRelativeRotation(ArrowRotation);
            ArrowHeads[Index]->SetRelativeScale3D(FVector(
                FleetMoveMarker::HeadThickness * VisualScale,
                FleetMoveMarker::HeadThickness * VisualScale,
                FleetMoveMarker::HeadLength / 100.0f * VisualScale));
        }
    }
}
