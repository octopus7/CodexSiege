#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SiegeCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;

UCLASS()
class IRONWALLSIEGE_API ASiegeCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ASiegeCameraPawn();
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void MoveUp(float Value);
    void Turn(float Value);
    void LookUp(float Value);
};
