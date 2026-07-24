#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeAssetProxyActor.generated.h"

class UProceduralMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class IRONWALLSIEGE_API ASiegeAssetProxyActor : public AActor
{
    GENERATED_BODY()

public:
    ASiegeAssetProxyActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Siege")
    ESiegeAssetSlot AssetSlot = ESiegeAssetSlot::Wall;

    UFUNCTION(BlueprintCallable, Category="Siege")
    void ConfigureAsset(ESiegeAssetSlot NewSlot);

    UFUNCTION(BlueprintCallable, Category="Siege")
    void RefreshVisual();

    UFUNCTION(BlueprintCallable, Category="Siege|Combat")
    void InitializeCombatant(
        ESiegeFaction NewFaction,
        float NewMaxHealth,
        float NewMoveSpeed,
        float NewAttackDamage,
        float NewAttackRange);

    UFUNCTION(BlueprintCallable, Category="Siege|Combat")
    bool ApplyCombatDamage(float Damage, AActor* DamageSource);

    UFUNCTION(BlueprintPure, Category="Siege|Combat")
    bool IsCombatAlive() const { return bCombatEnabled && !bDefeated; }

    UFUNCTION(BlueprintPure, Category="Siege|Combat")
    ESiegeFaction GetFaction() const { return Faction; }

    UFUNCTION(BlueprintPure, Category="Siege|Combat")
    float GetHealthRatio() const;

    float GetMoveSpeed() const { return MoveSpeed; }
    float GetAttackDamage() const { return AttackDamage; }
    float GetAttackRange() const { return AttackRange; }
    float GetAttackCooldownRemaining() const { return AttackCooldownRemaining; }
    const FVector& GetCombatVelocity() const { return CombatVelocity; }
    const FVector& GetCombatHomeLocation() const { return CombatHomeLocation; }

    void SetCombatVelocity(const FVector& NewVelocity) { CombatVelocity = NewVelocity; }
    void AdvanceCombatTime(float DeltaSeconds);
    void StartAttackCooldown(float Duration);
    void TriggerActionPulse();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Siege|Combat")
    ESiegeFaction Faction = ESiegeFaction::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Siege|Combat")
    float CurrentHealth = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Siege|Combat")
    float MaxHealth = 0.0f;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> ReplacementMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> FactionMarker;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> TrebuchetArmPivot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> TrebuchetArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> TrebuchetCounterweight;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FactionMarkerMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> TrebuchetArmMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> TrebuchetCounterweightMaterial;

    bool bCombatEnabled = false;
    bool bDefeated = false;
    bool bEmitValidationLog = false;
    float MoveSpeed = 0.0f;
    float AttackDamage = 0.0f;
    float AttackRange = 0.0f;
    float AttackCooldownRemaining = 0.0f;
    float ActionPulse = 0.0f;
    float TrebuchetMotionTime = 0.0f;
    FVector CombatVelocity = FVector::ZeroVector;
    FVector CombatHomeLocation = FVector::ZeroVector;
    FVector MarkerBaseScale = FVector(0.12f);

    void BuildProceduralMesh();
    void RefreshFactionMarker();
    void RefreshTrebuchetRig();
    void UpdateTrebuchetMotion(float MotionTime);
    void HandleDefeat(AActor* DamageSource);
};
