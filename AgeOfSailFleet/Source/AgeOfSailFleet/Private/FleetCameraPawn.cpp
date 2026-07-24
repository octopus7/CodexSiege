#include "FleetCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

AFleetCameraPawn::AFleetCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(Root);
    SpringArm->TargetArmLength = TargetArmLength;
    SpringArm->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->FieldOfView = 55.0f;
}

void AFleetCameraPawn::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FRotator FlatRotation(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f);
    const FVector Forward = FlatRotation.RotateVector(FVector::ForwardVector);
    const FVector Right = FlatRotation.RotateVector(FVector::RightVector);
    const float MoveSpeed = FMath::Lerp(1700.0f, 5200.0f, (TargetArmLength - 2600.0f) / 7800.0f);
    FVector Location = GetActorLocation() +
        (Forward * ForwardInput + Right * RightInput).GetClampedToMaxSize(1.0f) *
        MoveSpeed *
        DeltaSeconds;
    Location.X = FMath::Clamp(Location.X, -13500.0f, 13500.0f);
    Location.Y = FMath::Clamp(Location.Y, -13500.0f, 13500.0f);
    Location.Z = 0.0f;
    SetActorLocation(Location);

    TargetArmLength = FMath::Clamp(TargetArmLength - ZoomInput * 900.0f, 2600.0f, 10400.0f);
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
    SetActorLocation(FVector(WorldLocation.X, WorldLocation.Y, 0.0f));
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
