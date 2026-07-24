#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CannonballActor.generated.h"

class ASailShip;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class AGEOFSAILFLEET_API ACannonballActor : public AActor
{
    GENERATED_BODY()

public:
    ACannonballActor();
    virtual void Tick(float DeltaSeconds) override;
    void Launch(ASailShip* InSourceShip, const FVector& Velocity, float InDamage);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY()
    TWeakObjectPtr<ASailShip> SourceShip;

    float Damage = 45.0f;
    bool bImpactPlayed = false;

    UFUNCTION()
    void HandleHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);
};
