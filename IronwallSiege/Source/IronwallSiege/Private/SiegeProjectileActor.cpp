#include "SiegeProjectileActor.h"

#include "Components/SceneComponent.h"
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

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(Root);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMesh->SetCastShadow(true);
    ProjectileMesh->SetRelativeScale3D(FVector(0.34f));

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
    bShattering = false;
    ProjectileMesh->SetVisibility(true);
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

    if (bShattering)
    {
        ShatterElapsedTime += DeltaSeconds;
        const float ShatterAlpha = FMath::Clamp(ShatterElapsedTime / 0.72f, 0.0f, 1.0f);
        for (int32 Index = 0; Index < ImpactFragments.Num(); ++Index)
        {
            UStaticMeshComponent* Fragment = ImpactFragments[Index];
            if (!Fragment || !FragmentVelocities.IsValidIndex(Index))
            {
                continue;
            }

            FragmentVelocities[Index].Z -= 1450.0f * DeltaSeconds;
            Fragment->AddRelativeLocation(FragmentVelocities[Index] * DeltaSeconds);
            Fragment->AddLocalRotation(FRotator(
                410.0f * DeltaSeconds,
                (190.0f + static_cast<float>(Index) * 37.0f) * DeltaSeconds,
                275.0f * DeltaSeconds));

            const float BaseScale = 0.075f + 0.018f * static_cast<float>(Index % 3);
            Fragment->SetRelativeScale3D(FVector(BaseScale * (1.0f - ShatterAlpha)));
        }

        if (ShatterAlpha >= 1.0f)
        {
            Destroy();
        }
        return;
    }

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
        BeginShatter();
    }
}

void ASiegeProjectileActor::BeginShatter()
{
    bLaunched = false;
    bShattering = true;
    ShatterElapsedTime = 0.0f;
    ProjectileMesh->SetVisibility(false);

    UStaticMesh* RockMesh = ProjectileMesh->GetStaticMesh();
    for (int32 Index = 0; Index < 8; ++Index)
    {
        UStaticMeshComponent* Fragment = NewObject<UStaticMeshComponent>(this);
        Fragment->SetupAttachment(Root);
        Fragment->SetStaticMesh(RockMesh);
        Fragment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Fragment->SetCastShadow(true);
        Fragment->SetRelativeLocation(FVector::ZeroVector);
        Fragment->SetRelativeScale3D(FVector(0.09f));
        if (ProjectileMaterial)
        {
            Fragment->SetMaterial(0, ProjectileMaterial);
        }
        Fragment->RegisterComponent();
        ImpactFragments.Add(Fragment);

        const float Angle = UE_TWO_PI * static_cast<float>(Index) / 8.0f;
        const float HorizontalSpeed = 165.0f + 28.0f * static_cast<float>(Index % 4);
        FragmentVelocities.Add(FVector(
            FMath::Cos(Angle) * HorizontalSpeed,
            FMath::Sin(Angle) * HorizontalSpeed,
            220.0f + 42.0f * static_cast<float>(Index % 3)));
    }

    UE_LOG(LogTemp, Display, TEXT("IronwallSiegeCombat projectile_shattered"));
}
