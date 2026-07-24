#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SailShip.generated.h"

class UBoxComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class AGEOFSAILFLEET_API ASailShip : public APawn
{
    GENERATED_BODY()

public:
    ASailShip();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void ConfigureShip(int32 InTeam, bool bInPlayerFlagship, const FLinearColor& InTrimColor);
    void FireBroadside(int32 Side);
    void ReceiveCannonImpact(float Damage, ASailShip* Attacker, const FVector& ImpactPoint);

    bool IsAfloat() const { return Health > 0.0f; }
    int32 GetTeam() const { return Team; }
    float GetHealthRatio() const { return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f); }
    float GetSailSetting() const { return SailSetting; }
    float GetCurrentSpeed() const { return CurrentSpeed; }
    float GetPortReloadRatio() const;
    float GetStarboardReloadRatio() const;
    bool IsPlayerFlagship() const { return bPlayerFlagship; }

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> CollisionBox;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> VisualRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> HullMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> DetailMeshes;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> WoodMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> DarkWoodMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MetalMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> SailMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> TrimMaterial;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;

    int32 Team = 0;
    bool bPlayerFlagship = false;
    bool bVisualsBuilt = false;
    bool bSinking = false;
    float MaxHealth = 1200.0f;
    float Health = 1200.0f;
    float SailSetting = 0.65f;
    float CurrentSpeed = 0.0f;
    float SteeringInput = 0.0f;
    float PortReload = 0.0f;
    float StarboardReload = 0.0f;
    float BattleAge = 0.0f;
    float SinkTime = 0.0f;
    float AIThinkTime = 0.0f;
    float CameraYaw = 0.0f;
    float CameraPitch = -15.0f;
    float CameraDistance = 1900.0f;
    FLinearColor TrimColor = FLinearColor(0.08f, 0.22f, 0.55f);

    static constexpr float ReloadDuration = 7.5f;

    void BuildVisuals();
    void BuildHull();
    void BuildDeckAndRailings();
    void BuildGunDeck();
    void BuildMastsAndRigging();
    void BuildSails();
    void BuildDecorations();

    UStaticMeshComponent* AddBox(
        FName Name,
        const FVector& Location,
        const FVector& Size,
        const FRotator& Rotation,
        UMaterialInstanceDynamic* Material);
    UStaticMeshComponent* AddBeam(
        FName Name,
        const FVector& Start,
        const FVector& End,
        float Radius,
        UMaterialInstanceDynamic* Material);
    UStaticMeshComponent* AddSphere(
        FName Name,
        const FVector& Location,
        float Radius,
        UMaterialInstanceDynamic* Material);

    void ChangeSails(float Value);
    void Steer(float Value);
    void LookYaw(float Value);
    void LookPitch(float Value);
    void ZoomCamera(float Value);
    void FirePort();
    void FireStarboard();
    void SetFullSails();
    void ResetCamera();

    void TickMovement(float DeltaSeconds);
    void TickAI(float DeltaSeconds);
    void TickSinking(float DeltaSeconds);
    ASailShip* FindNearestEnemy() const;
    void AimAIAt(const FVector& DesiredDirection, float DeltaSeconds);
    void UpdateCamera();
};
