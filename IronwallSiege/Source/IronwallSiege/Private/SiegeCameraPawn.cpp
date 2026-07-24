#include "SiegeCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"

ASiegeCameraPawn::ASiegeCameraPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Root);
    Camera->FieldOfView = 65.0f;
}

void ASiegeCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASiegeCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASiegeCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &ASiegeCameraPawn::MoveUp);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASiegeCameraPawn::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASiegeCameraPawn::LookUp);
}

void ASiegeCameraPawn::MoveForward(const float Value)
{
    AddActorWorldOffset(GetActorForwardVector() * Value * 1000.0f * GetWorld()->GetDeltaSeconds(), true);
}

void ASiegeCameraPawn::MoveRight(const float Value)
{
    AddActorWorldOffset(GetActorRightVector() * Value * 1000.0f * GetWorld()->GetDeltaSeconds(), true);
}

void ASiegeCameraPawn::MoveUp(const float Value)
{
    AddActorWorldOffset(FVector::UpVector * Value * 1000.0f * GetWorld()->GetDeltaSeconds(), true);
}

void ASiegeCameraPawn::Turn(const float Value)
{
    AddActorLocalRotation(FRotator(0, Value, 0));
}

void ASiegeCameraPawn::LookUp(const float Value)
{
    const FRotator Current = GetActorRotation();
    SetActorRotation(FRotator(FMath::Clamp(Current.Pitch + Value, -80.0f, 25.0f), Current.Yaw, 0));
}
