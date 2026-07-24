#include "CannonballActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "FlipbookEffectActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SailShip.h"
#include "UObject/ConstructorHelpers.h"

namespace CannonballVisuals
{
    constexpr int32 TrailSegmentCount = 7;
    constexpr float TrailSampleInterval = 0.035f;
    constexpr float BaseBallScale = 0.72f;
}

ACannonballActor::ACannonballActor()
{
    PrimaryActorTick.bCanEverTick = true;
    InitialLifeSpan = 8.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(22.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetNotifyRigidBodyCollision(true);
    Collision->OnComponentHit.AddDynamic(this, &ACannonballActor::HandleHit);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(CannonballVisuals::BaseBallScale));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BallMaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (SphereFinder.Succeeded())
    {
        Visual->SetStaticMesh(SphereFinder.Object);
    }
    if (BallMaterialFinder.Succeeded())
    {
        Visual->SetMaterial(0, BallMaterialFinder.Object);
    }

    for (int32 Index = 0; Index < CannonballVisuals::TrailSegmentCount; ++Index)
    {
        UStaticMeshComponent* Trail = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("TrailSegment_%02d"), Index));
        Trail->SetupAttachment(Collision);
        Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Trail->SetCastShadow(false);
        Trail->SetVisibility(false);
        Trail->SetAbsolute(true, true, true);
        if (CylinderFinder.Succeeded())
        {
            Trail->SetStaticMesh(CylinderFinder.Object);
        }
        if (BallMaterialFinder.Succeeded())
        {
            Trail->SetMaterial(0, BallMaterialFinder.Object);
        }
        TrailSegments.Add(Trail);
    }

    BallLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BallLight"));
    BallLight->SetupAttachment(Collision);
    BallLight->SetLightColor(FLinearColor(1.0f, 0.22f, 0.025f));
    BallLight->SetIntensity(9000.0f);
    BallLight->SetAttenuationRadius(720.0f);
    BallLight->SetCastShadows(false);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 2500.0f;
    Movement->MaxSpeed = 3200.0f;
    Movement->ProjectileGravityScale = 0.42f;
    Movement->bRotationFollowsVelocity = true;
}

void ACannonballActor::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInterface* ParentMaterial = Visual->GetMaterial(0))
    {
        BallMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
        BallMaterial->SetVectorParameterValue(
            TEXT("Color"),
            FLinearColor(1.0f, 0.12f, 0.012f, 1.0f));
        Visual->SetMaterial(0, BallMaterial);
        for (UStaticMeshComponent* Trail : TrailSegments)
        {
            if (Trail)
            {
                Trail->SetMaterial(0, BallMaterial);
            }
        }
    }

    TrailPositions.Init(GetActorLocation(), TrailSegments.Num() + 1);
}

void ACannonballActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateTrail(DeltaSeconds);

    if (!bImpactPlayed && GetActorLocation().Z <= 5.0f)
    {
        bImpactPlayed = true;
        SpawnWaterImpact(GetActorLocation());
        Destroy();
    }
}

void ACannonballActor::Launch(
    ASailShip* InSourceShip,
    const FVector& Velocity,
    const float InDamage,
    const float InVisualScale)
{
    SourceShip = InSourceShip;
    Damage = InDamage;
    VisualScale = FMath::Clamp(InVisualScale, 0.85f, 1.5f);
    Visual->SetRelativeScale3D(
        FVector(CannonballVisuals::BaseBallScale * VisualScale));
    BallLight->SetIntensity(9000.0f * VisualScale);
    BallLight->SetAttenuationRadius(720.0f * VisualScale);
    Movement->Velocity = Velocity;
    if (InSourceShip)
    {
        Collision->IgnoreActorWhenMoving(InSourceShip, true);
    }
}

void ACannonballActor::HandleHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    (void)HitComponent;
    (void)OtherComponent;
    (void)NormalImpulse;

    ASailShip* TargetShip = Cast<ASailShip>(OtherActor);
    ASailShip* FiringShip = SourceShip.Get();
    if (TargetShip && FiringShip && TargetShip != FiringShip)
    {
        bImpactPlayed = true;
        if (TargetShip->GetTeam() != FiringShip->GetTeam())
        {
            TargetShip->ReceiveCannonImpact(Damage, FiringShip, Hit.ImpactPoint);
        }
        else if (AFlipbookEffectActor* Effect =
            GetWorld()->SpawnActor<AFlipbookEffectActor>(
                AFlipbookEffectActor::StaticClass(),
                Hit.ImpactPoint,
                FRotator::ZeroRotator))
        {
            Effect->PlayEffect(
                ESailFlipbookEffect::HullImpact,
                4.2f * VisualScale,
                1.05f);
        }
    }
    else if (!bImpactPlayed)
    {
        bImpactPlayed = true;
        SpawnWaterImpact(Hit.ImpactPoint);
    }
    Destroy();
}

void ACannonballActor::SpawnWaterImpact(const FVector& ImpactLocation)
{
    const FVector SurfaceLocation(
        ImpactLocation.X,
        ImpactLocation.Y,
        FMath::Max(ImpactLocation.Z, 35.0f));
    if (AFlipbookEffectActor* Effect = GetWorld()->SpawnActor<AFlipbookEffectActor>(
        AFlipbookEffectActor::StaticClass(),
        SurfaceLocation,
        FRotator::ZeroRotator))
    {
        Effect->PlayEffect(
            ESailFlipbookEffect::WaterImpact,
            5.8f * VisualScale,
            1.2f);
    }
}

void ACannonballActor::UpdateTrail(const float DeltaSeconds)
{
    if (TrailPositions.Num() != TrailSegments.Num() + 1)
    {
        TrailPositions.Init(GetActorLocation(), TrailSegments.Num() + 1);
    }

    TrailSampleAccumulator += DeltaSeconds;
    while (TrailSampleAccumulator >= CannonballVisuals::TrailSampleInterval)
    {
        for (int32 Index = TrailPositions.Num() - 1; Index > 0; --Index)
        {
            TrailPositions[Index] = TrailPositions[Index - 1];
        }
        TrailPositions[0] = GetActorLocation();
        TrailSampleAccumulator -= CannonballVisuals::TrailSampleInterval;
    }
    TrailPositions[0] = GetActorLocation();

    for (int32 Index = 0; Index < TrailSegments.Num(); ++Index)
    {
        UStaticMeshComponent* Trail = TrailSegments[Index];
        if (!Trail)
        {
            continue;
        }

        const FVector Start = TrailPositions[Index];
        const FVector End = TrailPositions[Index + 1];
        const FVector Segment = End - Start;
        const float Length = Segment.Size();
        if (Length < 4.0f)
        {
            Trail->SetVisibility(false);
            continue;
        }

        const float TrailAlpha =
            static_cast<float>(Index) /
            FMath::Max(1.0f, static_cast<float>(TrailSegments.Num() - 1));
        const float Radius =
            FMath::Lerp(13.0f, 4.5f, TrailAlpha) * VisualScale;
        Trail->SetWorldLocation((Start + End) * 0.5f);
        Trail->SetWorldRotation(
            FQuat::FindBetweenNormals(
                FVector::UpVector,
                Segment / Length));
        Trail->SetWorldScale3D(FVector(
            Radius / 50.0f,
            Radius / 50.0f,
            Length / 100.0f));
        Trail->SetVisibility(true);
    }
}
