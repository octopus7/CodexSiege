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

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    float ForwardInput = 0.0f;
    float RightInput = 0.0f;
    float ZoomInput = 0.0f;
    float TargetArmLength = 7200.0f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Zoom(float Value);
};
