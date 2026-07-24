#include "SiegeProjectileActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SiegeAssetProxyActor.h"
#include "UObject/ConstructorHelpers.h"

ASiegeProjectileActor::ASiegeProjectileActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    SetRootComponent(ProjectileMesh);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMesh->SetCastShadow(true);
    ProjectileMesh->SetRelativeScale3D(FVector(0.18f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        ProjectileMesh->SetStaticMesh(SphereMesh.Object);
    }
}

void ASiegeProjectileActor::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BaseMaterial)
    {
        ProjectileMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        ProjectileMesh->SetMaterial(0, ProjectileMaterial);
    }
}

void ASiegeProjectileActor::Launch(
    const FVector& Start,
    const FVector& End,
    ASiegeAssetProxyActor* NewTarget,
    AActor* NewDamageSource,
    const float NewDamage,
    const ESiegeFaction NewFaction,
    const float NewFlightDuration)
{
    StartLocation = Start;
    EndLocation = End;
    Target = NewTarget;
    DamageSource = NewDamageSource;
    Damage = FMath::Max(0.0f, NewDamage);
    Faction = NewFaction;
    FlightDuration = FMath::Max(0.25f, NewFlightDuration);
    ElapsedTime = 0.0f;
    bLaunched = true;
    SetActorLocation(StartLocation);
    SetActorTickEnabled(true);

    if (ProjectileMaterial)
    {
        const FLinearColor Color =
            Faction == ESiegeFaction::Attackers
                ? FLinearColor(0.35f, 0.03f, 0.01f)
                : FLinearColor(0.02f, 0.08f, 0.38f);
        ProjectileMaterial->SetVectorParameterValue(TEXT("Color"), Color);
    }
}

void ASiegeProjectileActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bLaunched)
    {
        return;
    }

    ElapsedTime += DeltaSeconds;
    const float Alpha = FMath::Clamp(ElapsedTime / FlightDuration, 0.0f, 1.0f);
    FVector Position = FMath::Lerp(StartLocation, EndLocation, Alpha);
    Position.Z += 4.0f * ArcHeight * Alpha * (1.0f - Alpha);
    SetActorLocation(Position);

    if (Alpha >= 1.0f)
    {
        if (ASiegeAssetProxyActor* ImpactTarget = Target.Get())
        {
            ImpactTarget->ApplyCombatDamage(Damage, DamageSource.Get());
        }
        Destroy();
    }
}
