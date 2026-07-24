#include "CannonballActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FlipbookEffectActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SailShip.h"
#include "UObject/ConstructorHelpers.h"

ACannonballActor::ACannonballActor()
{
    PrimaryActorTick.bCanEverTick = true;
    InitialLifeSpan = 8.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(13.0f);
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
    Visual->SetRelativeScale3D(FVector(0.26f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereFinder.Succeeded())
    {
        Visual->SetStaticMesh(SphereFinder.Object);
    }

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 2500.0f;
    Movement->MaxSpeed = 3200.0f;
    Movement->ProjectileGravityScale = 0.42f;
    Movement->bRotationFollowsVelocity = true;
}

void ACannonballActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bImpactPlayed && GetActorLocation().Z <= 5.0f)
    {
        bImpactPlayed = true;
        if (AFlipbookEffectActor* Effect = GetWorld()->SpawnActor<AFlipbookEffectActor>(
            AFlipbookEffectActor::StaticClass(),
            FVector(GetActorLocation().X, GetActorLocation().Y, 40.0f),
            FRotator::ZeroRotator))
        {
            Effect->PlayEffect(ESailFlipbookEffect::WaterImpact, 5.2f, 1.15f);
        }
        Destroy();
    }
}

void ACannonballActor::Launch(
    ASailShip* InSourceShip,
    const FVector& Velocity,
    const float InDamage)
{
    SourceShip = InSourceShip;
    Damage = InDamage;
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
        TargetShip->ReceiveCannonImpact(Damage, FiringShip, Hit.ImpactPoint);
    }
    Destroy();
}
