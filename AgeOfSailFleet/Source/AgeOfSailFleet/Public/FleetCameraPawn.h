#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FleetCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class AGEOFSAILFLEET_API AFleetCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AFleetCameraPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void FocusLocation(const FVector& WorldLocation);
    void ToggleFreeFlight();
    bool IsFreeFlightEnabled() const { return bFreeFlightEnabled; }

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    float ForwardInput = 0.0f;
    float RightInput = 0.0f;
    float UpInput = 0.0f;
    float ZoomInput = 0.0f;
    float TargetArmLength = 32000.0f;
    float FreeFlightSpeed = 12000.0f;
    bool bFreeFlightEnabled = false;

    FVector StrategyLocation = FVector::ZeroVector;
    FRotator StrategyRotation = FRotator::ZeroRotator;
    FRotator StrategyArmRotation = FRotator::ZeroRotator;
    FRotator FreeFlightRotation = FRotator::ZeroRotator;
    float StrategyArmLength = 32000.0f;
    float StrategyTargetArmLength = 32000.0f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void MoveUp(float Value);
    void Zoom(float Value);
    void LookYaw(float Value);
    void LookPitch(float Value);
    void EnterFreeFlight();
    void ExitFreeFlight();
};
