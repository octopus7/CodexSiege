#include "FleetCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace FleetCameraTuning
{
    constexpr float MinArmLength = 4200.0f;
    constexpr float MaxArmLength = 46000.0f;
    constexpr float WorldHalfExtent = 34000.0f;
    constexpr float ZoomStep = 1800.0f;
}

AFleetCameraPawn::AFleetCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(Root);
    SpringArm->TargetArmLength = TargetArmLength;
    // The fleets deploy primarily along world X. Looking across that axis keeps
    // both battle lines horizontal in a widescreen viewport instead of pushing
    // the opposing corners off-screen.
    SpringArm->SetRelativeRotation(FRotator(-70.0f, -90.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->FieldOfView = 60.0f;
}

void AFleetCameraPawn::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FRotator FlatRotation(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f);
    const FVector Forward = FlatRotation.RotateVector(FVector::ForwardVector);
    const FVector Right = FlatRotation.RotateVector(FVector::RightVector);
    const float ZoomAlpha = FMath::GetRangePct(
        FleetCameraTuning::MinArmLength,
        FleetCameraTuning::MaxArmLength,
        TargetArmLength);
    const float MoveSpeed = FMath::Lerp(2200.0f, 12000.0f, ZoomAlpha);
    FVector Location = GetActorLocation() +
        (Forward * ForwardInput + Right * RightInput).GetClampedToMaxSize(1.0f) *
        MoveSpeed *
        DeltaSeconds;
    Location.X = FMath::Clamp(
        Location.X,
        -FleetCameraTuning::WorldHalfExtent,
        FleetCameraTuning::WorldHalfExtent);
    Location.Y = FMath::Clamp(
        Location.Y,
        -FleetCameraTuning::WorldHalfExtent,
        FleetCameraTuning::WorldHalfExtent);
    Location.Z = 0.0f;
    SetActorLocation(Location);

    TargetArmLength = FMath::Clamp(
        TargetArmLength - ZoomInput * FleetCameraTuning::ZoomStep,
        FleetCameraTuning::MinArmLength,
        FleetCameraTuning::MaxArmLength);
    SpringArm->TargetArmLength = FMath::FInterpTo(
        SpringArm->TargetArmLength,
        TargetArmLength,
        DeltaSeconds,
        8.0f);
    ZoomInput = 0.0f;
}

void AFleetCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("CameraForward"), this, &AFleetCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("CameraRight"), this, &AFleetCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("CameraZoom"), this, &AFleetCameraPawn::Zoom);
}

void AFleetCameraPawn::FocusLocation(const FVector& WorldLocation)
{
    SetActorLocation(FVector(
        FMath::Clamp(
            WorldLocation.X,
            -FleetCameraTuning::WorldHalfExtent,
            FleetCameraTuning::WorldHalfExtent),
        FMath::Clamp(
            WorldLocation.Y,
            -FleetCameraTuning::WorldHalfExtent,
            FleetCameraTuning::WorldHalfExtent),
        0.0f));
}

void AFleetCameraPawn::MoveForward(const float Value)
{
    ForwardInput = Value;
}

void AFleetCameraPawn::MoveRight(const float Value)
{
    RightInput = Value;
}

void AFleetCameraPawn::Zoom(const float Value)
{
    ZoomInput += Value;
}
