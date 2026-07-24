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
    constexpr float FreeFlightHalfExtent = 100000.0f;
    constexpr float FreeFlightMinHeight = 250.0f;
    constexpr float FreeFlightMaxHeight = 60000.0f;
    constexpr float FreeFlightMinSpeed = 2500.0f;
    constexpr float FreeFlightMaxSpeed = 50000.0f;
    constexpr float FreeFlightSpeedStep = 2500.0f;
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

    if (bFreeFlightEnabled)
    {
        FreeFlightSpeed = FMath::Clamp(
            FreeFlightSpeed + ZoomInput * FleetCameraTuning::FreeFlightSpeedStep,
            FleetCameraTuning::FreeFlightMinSpeed,
            FleetCameraTuning::FreeFlightMaxSpeed);

        const FVector Forward = FreeFlightRotation.Vector();
        const FVector Right = FRotationMatrix(FreeFlightRotation).GetUnitAxis(EAxis::Y);
        const FVector MoveDirection =
            (Forward * ForwardInput + Right * RightInput + FVector::UpVector * UpInput)
            .GetClampedToMaxSize(1.0f);
        FVector Location = GetActorLocation() + MoveDirection * FreeFlightSpeed * DeltaSeconds;
        Location.X = FMath::Clamp(
            Location.X,
            -FleetCameraTuning::FreeFlightHalfExtent,
            FleetCameraTuning::FreeFlightHalfExtent);
        Location.Y = FMath::Clamp(
            Location.Y,
            -FleetCameraTuning::FreeFlightHalfExtent,
            FleetCameraTuning::FreeFlightHalfExtent);
        Location.Z = FMath::Clamp(
            Location.Z,
            FleetCameraTuning::FreeFlightMinHeight,
            FleetCameraTuning::FreeFlightMaxHeight);
        SetActorLocation(Location);
        SpringArm->SetRelativeRotation(FreeFlightRotation);
        ZoomInput = 0.0f;
        return;
    }

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
    PlayerInputComponent->BindAxis(TEXT("CameraUp"), this, &AFleetCameraPawn::MoveUp);
    PlayerInputComponent->BindAxis(TEXT("CameraZoom"), this, &AFleetCameraPawn::Zoom);
    PlayerInputComponent->BindAxis(TEXT("CameraYaw"), this, &AFleetCameraPawn::LookYaw);
    PlayerInputComponent->BindAxis(TEXT("CameraPitch"), this, &AFleetCameraPawn::LookPitch);
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

void AFleetCameraPawn::MoveUp(const float Value)
{
    UpInput = Value;
}

void AFleetCameraPawn::Zoom(const float Value)
{
    ZoomInput += Value;
}

void AFleetCameraPawn::LookYaw(const float Value)
{
    if (!bFreeFlightEnabled || FMath::IsNearlyZero(Value))
    {
        return;
    }
    FreeFlightRotation.Yaw += Value;
    FreeFlightRotation.Normalize();
}

void AFleetCameraPawn::LookPitch(const float Value)
{
    if (!bFreeFlightEnabled || FMath::IsNearlyZero(Value))
    {
        return;
    }
    FreeFlightRotation.Pitch = FMath::Clamp(
        FreeFlightRotation.Pitch + Value,
        -89.0f,
        89.0f);
}

void AFleetCameraPawn::ToggleFreeFlight()
{
    if (bFreeFlightEnabled)
    {
        ExitFreeFlight();
    }
    else
    {
        EnterFreeFlight();
    }
}

void AFleetCameraPawn::EnterFreeFlight()
{
    StrategyLocation = GetActorLocation();
    StrategyRotation = GetActorRotation();
    StrategyArmRotation = SpringArm->GetRelativeRotation();
    StrategyArmLength = SpringArm->TargetArmLength;
    StrategyTargetArmLength = TargetArmLength;

    const FVector CurrentViewLocation = Camera->GetComponentLocation();
    FreeFlightRotation = Camera->GetComponentRotation();
    SetActorLocationAndRotation(CurrentViewLocation, FRotator::ZeroRotator);
    SpringArm->TargetArmLength = 0.0f;
    SpringArm->SetRelativeRotation(FreeFlightRotation);
    ForwardInput = 0.0f;
    RightInput = 0.0f;
    UpInput = 0.0f;
    ZoomInput = 0.0f;
    bFreeFlightEnabled = true;
}

void AFleetCameraPawn::ExitFreeFlight()
{
    bFreeFlightEnabled = false;
    SetActorLocationAndRotation(StrategyLocation, StrategyRotation);
    SpringArm->SetRelativeRotation(StrategyArmRotation);
    SpringArm->TargetArmLength = StrategyArmLength;
    TargetArmLength = StrategyTargetArmLength;
    ForwardInput = 0.0f;
    RightInput = 0.0f;
    UpInput = 0.0f;
    ZoomInput = 0.0f;
}
