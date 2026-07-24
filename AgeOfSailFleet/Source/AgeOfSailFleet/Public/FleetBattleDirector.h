#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SailGraphicsMode.h"
#include "FleetBattleDirector.generated.h"

class ASailShip;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

UENUM(BlueprintType)
enum class EFleetBattleState : uint8
{
    AwaitingOrders,
    InBattle,
    BlueVictory,
    RedVictory
};

UCLASS()
class AGEOFSAILFLEET_API AFleetBattleDirector : public AActor
{
    GENERATED_BODY()

public:
    AFleetBattleDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable)
    void StartBattle();

    UFUNCTION(BlueprintCallable)
    void SetGraphicsMode(ESailGraphicsMode InGraphicsMode)
    {
        GraphicsMode = InGraphicsMode;
    }

    EFleetBattleState GetBattleState() const { return BattleState; }
    int32 CountAfloat(int32 Team) const;
    float GetBattleElapsedTime() const { return BattleElapsedTime; }
    FVector GetWindDirection() const;
    float GetWindStrength() const { return WindStrength; }
    int32 GetVoyageDay() const { return 24; }
    int32 GetVoyageMonth() const { return 7; }
    int32 GetVoyageYear() const { return 1715; }

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UDirectionalLightComponent> SunLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkyLightComponent> SkyLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UExponentialHeightFogComponent> HeightFog;

    UPROPERTY()
    TArray<TWeakObjectPtr<ASailShip>> FleetShips;

    EFleetBattleState BattleState = EFleetBattleState::AwaitingOrders;
    float BattleElapsedTime = 0.0f;
    float OutcomeCheckTime = 0.0f;
    float WindHeadingDegrees = 35.0f;
    float WindStrength = 0.82f;
    ESailGraphicsMode GraphicsMode = ESailGraphicsMode::ThreeDimensional;

    void SpawnFleet(int32 Team);
    ASailShip* SpawnShip(
        int32 Team,
        int32 FleetIndex,
        const FVector& Location,
        const FRotator& Rotation,
        bool bFlagship);
};
