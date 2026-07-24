#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeWorldDirector.generated.h"

class ASiegeAssetProxyActor;

UCLASS()
class IRONWALLSIEGE_API ASiegeWorldDirector : public AActor
{
    GENERATED_BODY()

public:
    ASiegeWorldDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Siege|Battle")
    void StartBattle();

    UFUNCTION(BlueprintPure, Category="Siege|Battle")
    bool IsBattleStarted() const { return bBattleStarted; }

private:
    ASiegeAssetProxyActor* SpawnAsset(
        ESiegeAssetSlot Slot,
        const FVector& Location,
        const FRotator& Rotation,
        const FVector& Scale,
        const FString& Label);

    ASiegeAssetProxyActor* SpawnCombatant(
        ESiegeAssetSlot Slot,
        ESiegeFaction Faction,
        const FVector& Location,
        const FRotator& Rotation,
        const FVector& Scale,
        float Health,
        float MoveSpeed,
        float AttackDamage,
        float AttackRange,
        const FString& Label);

    void ConfigureStaticBattlefield();
    void SpawnForces();
    void TickInfantryGroup(
        TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& Group,
        const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& EnemyGroup,
        ESiegeFaction Faction,
        float DeltaSeconds);
    ASiegeAssetProxyActor* FindNearestEnemy(
        const ASiegeAssetProxyActor* Unit,
        const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& EnemyGroup,
        float MaxDistance) const;
    FVector ComputeSwarmVelocity(
        const ASiegeAssetProxyActor* Unit,
        const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& Group,
        const FVector& TargetLocation) const;
    void TickSiegeEngines(float DeltaSeconds);
    void LaunchTrebuchetProjectile();
    void CheckBattleOutcome();
    int32 CountLiving(const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& Group) const;

    UFUNCTION()
    void HandleResourceSetChanged(FName ResourceSetId);

    TArray<TWeakObjectPtr<ASiegeAssetProxyActor>> AttackerInfantry;
    TArray<TWeakObjectPtr<ASiegeAssetProxyActor>> DefenderInfantry;
    TArray<TWeakObjectPtr<ASiegeAssetProxyActor>> AttackerCombatants;
    TWeakObjectPtr<ASiegeAssetProxyActor> Gate;
    TWeakObjectPtr<ASiegeAssetProxyActor> BatteringRam;
    TWeakObjectPtr<ASiegeAssetProxyActor> Trebuchet;
    float BattleElapsedTime = 0.0f;
    float OutcomeCheckTime = 0.0f;
    float NextTelemetryTime = 5.0f;
    bool bBattleStarted = false;
    bool bBattleResolved = false;
};
