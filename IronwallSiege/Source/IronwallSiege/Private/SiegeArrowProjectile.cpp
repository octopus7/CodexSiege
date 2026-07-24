#include "SiegeArrowProjectile.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SiegeAssetProxyActor.h"
#include "UObject/ConstructorHelpers.h"

namespace SiegeArrow
{
    constexpr float ArcHeight = 190.0f;
}

ASiegeArrowProjectile::ASiegeArrowProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    InitialLifeSpan = 4.0f;

    ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
    SetRootComponent(ArrowMesh);
    ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrowMesh->SetCastShadow(true);
    ArrowMesh->SetRelativeScale3D(FVector(0.025f, 0.025f, 0.60f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        ArrowMesh->SetStaticMesh(CylinderMesh.Object);
    }
}

void ASiegeArrowProjectile::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BaseMaterial)
    {
        ArrowMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        ArrowMesh->SetMaterial(0, ArrowMaterial);
    }
}

void ASiegeArrowProjectile::Launch(
    const FVector& Start,
    ASiegeAssetProxyActor* NewTarget,
    AActor* NewDamageSource,
    const float NewDamage,
    const ESiegeFaction NewFaction,
    const float NewFlightDuration,
    const bool bNewEmitImpactLog)
{
    if (!NewTarget)
    {
        Destroy();
        return;
    }

    StartLocation = Start;
    PreviousLocation = Start;
    Target = NewTarget;
    TargetLocation = NewTarget->GetActorLocation() + FVector(0.0f, 0.0f, 145.0f);
    DamageSource = NewDamageSource;
    Damage = FMath::Max(0.0f, NewDamage);
    Faction = NewFaction;
    bEmitImpactLog = bNewEmitImpactLog;
    FlightDuration = FMath::Max(0.25f, NewFlightDuration);
    ElapsedTime = 0.0f;
    bLaunched = true;
    SetActorLocation(StartLocation);
    SetActorTickEnabled(true);

    if (ArrowMaterial)
    {
        const FLinearColor Color =
            Faction == ESiegeFaction::Attackers
                ? FLinearColor(0.30f, 0.045f, 0.012f)
                : FLinearColor(0.015f, 0.055f, 0.28f);
        ArrowMaterial->SetVectorParameterValue(TEXT("Color"), Color);
    }
}

void ASiegeArrowProjectile::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bLaunched)
    {
        return;
    }

    if (const ASiegeAssetProxyActor* CurrentTarget = Target.Get())
    {
        if (CurrentTarget->IsCombatAlive())
        {
            TargetLocation = CurrentTarget->GetActorLocation() + FVector(0.0f, 0.0f, 145.0f);
        }
    }

    ElapsedTime += DeltaSeconds;
    const float Alpha = FMath::Clamp(ElapsedTime / FlightDuration, 0.0f, 1.0f);
    FVector Position = FMath::Lerp(StartLocation, TargetLocation, Alpha);
    Position.Z += 4.0f * SiegeArrow::ArcHeight * Alpha * (1.0f - Alpha);
    SetActorLocation(Position);

    const FVector FlightDirection = Position - PreviousLocation;
    if (!FlightDirection.IsNearlyZero())
    {
        SetActorRotation(FRotationMatrix::MakeFromZ(FlightDirection).Rotator());
    }
    PreviousLocation = Position;

    if (Alpha >= 1.0f)
    {
        if (ASiegeAssetProxyActor* ImpactTarget = Target.Get())
        {
            ImpactTarget->ApplyCombatDamage(Damage, DamageSource.Get());
            if (bEmitImpactLog)
            {
                UE_LOG(
                    LogTemp,
                    Display,
                    TEXT("IronwallSiegeArrow impact target=%s damage=%.0f health_ratio=%.2f"),
                    *ImpactTarget->GetName(),
                    Damage,
                    ImpactTarget->GetHealthRatio());
            }
        }
        Destroy();
    }
}
