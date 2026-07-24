#include "SiegeDefenderArtilleryActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SiegeAssetProxyActor.h"
#include "SiegeProjectileActor.h"
#include "SiegeWorldDirector.h"
#include "UObject/ConstructorHelpers.h"

namespace DefenderArtillery
{
    constexpr float TargetScanInterval = 0.28f;
    constexpr float MaximumRange = 4300.0f;
    constexpr float MinimumRange = 240.0f;
    constexpr float Damage = 74.0f;
    constexpr float FireInterval = 3.1f;
    constexpr float RecoilDuration = 0.34f;
}

ASiegeDefenderArtilleryActor::ASiegeDefenderArtilleryActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Carriage = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Carriage"));
    Carriage->SetupAttachment(Root);
    Carriage->SetRelativeLocation(FVector(0.0f, 0.0f, 38.0f));
    Carriage->SetRelativeScale3D(FVector(1.45f, 1.05f, 0.36f));

    LeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWheel"));
    LeftWheel->SetupAttachment(Root);
    LeftWheel->SetRelativeLocation(FVector(-12.0f, -75.0f, 48.0f));
    LeftWheel->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
    LeftWheel->SetRelativeScale3D(FVector(0.52f, 0.52f, 0.18f));

    RightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWheel"));
    RightWheel->SetupAttachment(Root);
    RightWheel->SetRelativeLocation(FVector(-12.0f, 75.0f, 48.0f));
    RightWheel->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
    RightWheel->SetRelativeScale3D(FVector(0.52f, 0.52f, 0.18f));

    AimPivot = CreateDefaultSubobject<USceneComponent>(TEXT("AimPivot"));
    AimPivot->SetupAttachment(Root);
    AimPivot->SetRelativeLocation(FVector(0.0f, 0.0f, 112.0f));

    Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
    Barrel->SetupAttachment(AimPivot);
    Barrel->SetRelativeLocation(FVector(108.0f, 0.0f, 0.0f));
    Barrel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    Barrel->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.42f));

    MuzzleFlash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleFlash"));
    MuzzleFlash->SetupAttachment(AimPivot);
    MuzzleFlash->SetRelativeLocation(FVector(254.0f, 0.0f, 0.0f));
    MuzzleFlash->SetRelativeScale3D(FVector::ZeroVector);
    MuzzleFlash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MuzzleFlash->SetCastShadow(false);

    FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
    FlashLight->SetupAttachment(MuzzleFlash);
    FlashLight->SetLightColor(FLinearColor(1.0f, 0.26f, 0.025f));
    FlashLight->SetAttenuationRadius(620.0f);
    FlashLight->SetIntensity(0.0f);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    if (CubeMesh.Succeeded())
    {
        Carriage->SetStaticMesh(CubeMesh.Object);
    }
    if (CylinderMesh.Succeeded())
    {
        LeftWheel->SetStaticMesh(CylinderMesh.Object);
        RightWheel->SetStaticMesh(CylinderMesh.Object);
        Barrel->SetStaticMesh(CylinderMesh.Object);
    }
    if (SphereMesh.Succeeded())
    {
        MuzzleFlash->SetStaticMesh(SphereMesh.Object);
    }

    Carriage->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftWheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightWheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASiegeDefenderArtilleryActor::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BaseMaterial)
    {
        CarriageMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        MetalMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        FlashMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);

        CarriageMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(0.12f, 0.055f, 0.018f));
        MetalMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(0.055f, 0.065f, 0.075f));
        FlashMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(1.0f, 0.22f, 0.015f));

        Carriage->SetMaterial(0, CarriageMaterial);
        LeftWheel->SetMaterial(0, CarriageMaterial);
        RightWheel->SetMaterial(0, CarriageMaterial);
        Barrel->SetMaterial(0, MetalMaterial);
        MuzzleFlash->SetMaterial(0, FlashMaterial);
    }

    if (!bPositionedOnWall)
    {
        TryPositionOnWall();
    }
}

void ASiegeDefenderArtilleryActor::InitializeBattery(const int32 NewBatteryIndex)
{
    BatteryIndex = FMath::Clamp(NewBatteryIndex, -1, 1);
    bPositionedOnWall = false;
    TryPositionOnWall();
}

void ASiegeDefenderArtilleryActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateFireFeedback(DeltaSeconds);

    if (!bPositionedOnWall)
    {
        TryPositionOnWall();
    }

    if (!IsBattleActive())
    {
        CurrentTarget.Reset();
        bObservedBattleStart = false;
        return;
    }

    if (!bObservedBattleStart)
    {
        bObservedBattleStart = true;
        FireCooldownRemaining =
            0.55f + static_cast<float>(BatteryIndex + 1) * 0.38f;
    }

    FireCooldownRemaining = FMath::Max(0.0f, FireCooldownRemaining - DeltaSeconds);
    TargetScanRemaining -= DeltaSeconds;
    if (TargetScanRemaining <= 0.0f ||
        !CurrentTarget.IsValid() ||
        !CurrentTarget->IsCombatAlive())
    {
        CurrentTarget = FindTarget();
        TargetScanRemaining = DefenderArtillery::TargetScanInterval;
    }

    ASiegeAssetProxyActor* Target = CurrentTarget.Get();
    if (!Target)
    {
        return;
    }

    AimAtTarget(*Target, DeltaSeconds);
    if (FireCooldownRemaining <= 0.0f)
    {
        FireAt(*Target);
    }
}

bool ASiegeDefenderArtilleryActor::TryPositionOnWall()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    ASiegeAssetProxyActor* Gate = nullptr;
    for (TActorIterator<ASiegeAssetProxyActor> It(World); It; ++It)
    {
        if (It->AssetSlot == ESiegeAssetSlot::Gate)
        {
            Gate = *It;
            break;
        }
    }
    if (!Gate)
    {
        return false;
    }

    AActor* Mount = Gate;
    if (BatteryIndex != 0)
    {
        const FVector GateLocation = Gate->GetActorLocation();
        const float DesiredX =
            GateLocation.X + static_cast<float>(BatteryIndex) * 900.0f;
        float BestScore = BIG_NUMBER;

        for (TActorIterator<ASiegeAssetProxyActor> It(World); It; ++It)
        {
            ASiegeAssetProxyActor* Candidate = *It;
            if (!Candidate || Candidate->AssetSlot != ESiegeAssetSlot::Wall)
            {
                continue;
            }

            const FVector CandidateLocation = Candidate->GetActorLocation();
            const float Score =
                FMath::Abs(CandidateLocation.X - DesiredX) +
                FMath::Abs(CandidateLocation.Y - GateLocation.Y) * 2.5f;
            if (Score < BestScore)
            {
                BestScore = Score;
                Mount = Candidate;
            }
        }
    }

    FVector BoundsOrigin = Mount->GetActorLocation();
    FVector BoundsExtent = FVector::ZeroVector;
    Mount->GetActorBounds(true, BoundsOrigin, BoundsExtent);
    SetActorLocation(FVector(
        BoundsOrigin.X,
        BoundsOrigin.Y,
        BoundsOrigin.Z + BoundsExtent.Z + 8.0f));
    SetActorRotation(FRotator(0.0f, -90.0f, 0.0f));
    bPositionedOnWall = true;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeArtillery battery=%d mounted_on=%s location=(%.0f,%.0f,%.0f)"),
        BatteryIndex,
        *Mount->GetName(),
        GetActorLocation().X,
        GetActorLocation().Y,
        GetActorLocation().Z);
    return true;
}

bool ASiegeDefenderArtilleryActor::IsBattleActive() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    for (TActorIterator<ASiegeWorldDirector> It(World); It; ++It)
    {
        return It->IsBattleStarted();
    }
    return false;
}

ASiegeAssetProxyActor* ASiegeDefenderArtilleryActor::FindTarget() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    ASiegeAssetProxyActor* BestTarget = nullptr;
    float BestDistanceSquared = FMath::Square(DefenderArtillery::MaximumRange);
    const float MinimumDistanceSquared = FMath::Square(DefenderArtillery::MinimumRange);

    for (TActorIterator<ASiegeAssetProxyActor> It(World); It; ++It)
    {
        ASiegeAssetProxyActor* Candidate = *It;
        if (!Candidate ||
            !Candidate->IsCombatAlive() ||
            Candidate->GetFaction() != ESiegeFaction::Attackers)
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(
            GetActorLocation(),
            Candidate->GetActorLocation());
        if (DistanceSquared >= MinimumDistanceSquared &&
            DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }
    return BestTarget;
}

void ASiegeDefenderArtilleryActor::AimAtTarget(
    const ASiegeAssetProxyActor& Target,
    const float DeltaSeconds)
{
    const FVector PivotLocation = AimPivot->GetComponentLocation();
    const FVector AimLocation = Target.GetActorLocation() + FVector(0.0f, 0.0f, 105.0f);
    const FRotator DesiredRotation = (AimLocation - PivotLocation).Rotation();
    const FRotator SmoothedRotation = FMath::RInterpTo(
        AimPivot->GetComponentRotation(),
        DesiredRotation,
        DeltaSeconds,
        3.5f);
    AimPivot->SetWorldRotation(SmoothedRotation);
}

void ASiegeDefenderArtilleryActor::FireAt(ASiegeAssetProxyActor& Target)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector Start = AimPivot->GetComponentLocation() +
        AimPivot->GetForwardVector() * 258.0f;
    const FVector End = Target.GetActorLocation() + FVector(
        FMath::FRandRange(-34.0f, 34.0f),
        FMath::FRandRange(-28.0f, 28.0f),
        95.0f);
    const float Distance = FVector::Distance(Start, End);
    const float FlightDuration = FMath::Clamp(Distance / 1850.0f, 0.8f, 2.1f);

    ASiegeProjectileActor* Projectile = World->SpawnActor<ASiegeProjectileActor>(
        ASiegeProjectileActor::StaticClass(),
        Start,
        AimPivot->GetComponentRotation());
    if (!Projectile)
    {
        return;
    }

    Projectile->Launch(
        Start,
        End,
        &Target,
        this,
        DefenderArtillery::Damage,
        ESiegeFaction::Defenders,
        FlightDuration);

    FireCooldownRemaining =
        DefenderArtillery::FireInterval + FMath::FRandRange(-0.24f, 0.28f);
    RecoilRemaining = DefenderArtillery::RecoilDuration;
    MuzzleFlash->SetRelativeScale3D(FVector(0.34f));
    FlashLight->SetIntensity(7200.0f);

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeCombat wall_artillery_fired battery=%d target=%s damage=%.0f"),
        BatteryIndex,
        *Target.GetName(),
        DefenderArtillery::Damage);
}

void ASiegeDefenderArtilleryActor::UpdateFireFeedback(const float DeltaSeconds)
{
    RecoilRemaining = FMath::Max(0.0f, RecoilRemaining - DeltaSeconds);
    if (RecoilRemaining <= 0.0f)
    {
        Barrel->SetRelativeLocation(FVector(108.0f, 0.0f, 0.0f));
        MuzzleFlash->SetRelativeScale3D(FVector::ZeroVector);
        FlashLight->SetIntensity(0.0f);
        return;
    }

    const float Alpha = RecoilRemaining / DefenderArtillery::RecoilDuration;
    const float RecoilOffset = FMath::Sin(Alpha * PI) * 32.0f;
    Barrel->SetRelativeLocation(FVector(108.0f - RecoilOffset, 0.0f, 0.0f));
    const float FlashAlpha = FMath::Clamp((Alpha - 0.45f) / 0.55f, 0.0f, 1.0f);
    MuzzleFlash->SetRelativeScale3D(FVector(0.34f * FlashAlpha));
    FlashLight->SetIntensity(7200.0f * FlashAlpha);
}
