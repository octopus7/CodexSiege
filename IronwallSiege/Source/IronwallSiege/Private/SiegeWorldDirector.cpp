#include "SiegeWorldDirector.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "SiegeAssetProxyActor.h"
#include "SiegeGameInstance.h"
#include "SiegeProjectileActor.h"

namespace SiegeBattle
{
    constexpr float InfantryAttackCooldown = 0.82f;
    constexpr float InfantryAwarenessRange = 1750.0f;
    constexpr float SeparationRadius = 165.0f;
    constexpr float CohesionRadius = 620.0f;
    constexpr float GateApproachOffset = 235.0f;
    constexpr int32 AttackerRanks = 12;
    constexpr int32 AttackerFiles = 18;
    constexpr int32 DefenderRanks = 8;
    constexpr int32 DefenderFiles = 16;
    constexpr float AttackerFileSpacing = 140.0f;
    constexpr float AttackerRankSpacing = 175.0f;
    constexpr float DefenderFileSpacing = 145.0f;
    constexpr float DefenderRankSpacing = 120.0f;
}

ASiegeWorldDirector::ASiegeWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void ASiegeWorldDirector::BeginPlay()
{
    Super::BeginPlay();

    if (USiegeGameInstance* GameInstance = Cast<USiegeGameInstance>(GetGameInstance()))
    {
        GameInstance->OnResourceSetChanged.AddDynamic(this, &ASiegeWorldDirector::HandleResourceSetChanged);
    }

    ConfigureStaticBattlefield();
    SpawnForces();

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeBattle ready attackers=%d defenders=%d gate=%s"),
        AttackerCombatants.Num(),
        DefenderInfantry.Num(),
        Gate.IsValid() ? TEXT("ready") : TEXT("missing"));

    if (FParse::Param(FCommandLine::Get(), TEXT("SkipTitle")))
    {
        StartBattle();
    }
}

void ASiegeWorldDirector::StartBattle()
{
    if (bBattleStarted || bBattleResolved)
    {
        return;
    }

    bBattleStarted = true;
    BattleElapsedTime = 0.0f;
    OutcomeCheckTime = 0.0f;
    NextTelemetryTime = 5.0f;
    SetActorTickEnabled(true);

    if (Trebuchet.IsValid())
    {
        Trebuchet->StartAttackCooldown(1.25f);
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeBattle started attackers=%d defenders=%d"),
        CountLiving(AttackerCombatants),
        CountLiving(DefenderInfantry));
}

void ASiegeWorldDirector::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bBattleStarted || bBattleResolved)
    {
        return;
    }

    BattleElapsedTime += DeltaSeconds;
    TickInfantryGroup(
        AttackerInfantry,
        DefenderInfantry,
        ESiegeFaction::Attackers,
        DeltaSeconds);
    TickInfantryGroup(
        DefenderInfantry,
        AttackerCombatants,
        ESiegeFaction::Defenders,
        DeltaSeconds);
    TickSiegeEngines(DeltaSeconds);

    if (Gate.IsValid())
    {
        Gate->AdvanceCombatTime(DeltaSeconds);
    }

    OutcomeCheckTime += DeltaSeconds;
    if (OutcomeCheckTime >= 0.5f)
    {
        OutcomeCheckTime = 0.0f;
        CheckBattleOutcome();
    }

    if (BattleElapsedTime >= NextTelemetryTime)
    {
        float AttackerFrontY = -BIG_NUMBER;
        for (const TWeakObjectPtr<ASiegeAssetProxyActor>& UnitPtr : AttackerInfantry)
        {
            const ASiegeAssetProxyActor* Unit = UnitPtr.Get();
            if (Unit && Unit->IsCombatAlive())
            {
                AttackerFrontY = FMath::Max(AttackerFrontY, Unit->GetActorLocation().Y);
            }
        }

        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeBattle state time=%.1f attackers=%d defenders=%d front_y=%.0f ram_y=%.0f gate_health=%.2f"),
            BattleElapsedTime,
            CountLiving(AttackerCombatants),
            CountLiving(DefenderInfantry),
            AttackerFrontY > -BIG_NUMBER ? AttackerFrontY : 0.0f,
            BatteringRam.IsValid() ? BatteringRam->GetActorLocation().Y : 0.0f,
            Gate.IsValid() ? Gate->GetHealthRatio() : 0.0f);
        NextTelemetryTime += 5.0f;
    }
}

void ASiegeWorldDirector::ConfigureStaticBattlefield()
{
    for (TActorIterator<ASiegeAssetProxyActor> It(GetWorld()); It; ++It)
    {
        ASiegeAssetProxyActor* Actor = *It;
        if (Actor && Actor->AssetSlot == ESiegeAssetSlot::Gate)
        {
            Gate = Actor;
            Actor->InitializeCombatant(
                ESiegeFaction::Defenders,
                3400.0f,
                0.0f,
                0.0f,
                0.0f);
            break;
        }
    }
}

void ASiegeWorldDirector::SpawnForces()
{
    Trebuchet = SpawnCombatant(
        ESiegeAssetSlot::Trebuchet,
        ESiegeFaction::Attackers,
        FVector(-1180.0f, -1940.0f, 0.0f),
        FRotator(0.0f, 8.0f, 0.0f),
        FVector(1.25f),
        650.0f,
        0.0f,
        185.0f,
        5000.0f,
        TEXT("Attacker_Trebuchet"));

    BatteringRam = SpawnCombatant(
        ESiegeAssetSlot::BatteringRam,
        ESiegeFaction::Attackers,
        FVector(0.0f, -1280.0f, 0.0f),
        FRotator::ZeroRotator,
        FVector(1.15f),
        4000.0f,
        105.0f,
        145.0f,
        430.0f,
        TEXT("Attacker_BatteringRam"));

    if (Trebuchet.IsValid())
    {
        AttackerCombatants.Add(Trebuchet);
    }
    if (BatteringRam.IsValid())
    {
        AttackerCombatants.Add(BatteringRam);
    }

    int32 SoldierIndex = 0;
    for (int32 Rank = 0; Rank < SiegeBattle::AttackerRanks; ++Rank)
    {
        for (int32 File = 0; File < SiegeBattle::AttackerFiles; ++File)
        {
            const float FileOffset =
                static_cast<float>(File) -
                (static_cast<float>(SiegeBattle::AttackerFiles) - 1.0f) * 0.5f;
            const float X = FileOffset * SiegeBattle::AttackerFileSpacing;

            // Leave a broad central lane so the battering ram remains visible
            // while the army advances in dense companies on both flanks.
            if (Rank >= 3 && Rank <= 8 && FMath::Abs(X) < 260.0f)
            {
                continue;
            }

            const FVector Position(
                X,
                -320.0f - static_cast<float>(Rank) * SiegeBattle::AttackerRankSpacing,
                0.0f);
            ASiegeAssetProxyActor* Soldier = SpawnCombatant(
                ESiegeAssetSlot::Infantry,
                ESiegeFaction::Attackers,
                Position,
                FRotator(0.0f, 90.0f, 0.0f),
                FVector(1.08f),
                125.0f,
                215.0f,
                19.0f,
                145.0f,
                FString::Printf(TEXT("Attacker_Infantry_%03d"), SoldierIndex++));
            if (Soldier)
            {
                AttackerInfantry.Add(Soldier);
                AttackerCombatants.Add(Soldier);
            }
        }
    }

    SoldierIndex = 0;
    for (int32 Rank = 0; Rank < SiegeBattle::DefenderRanks; ++Rank)
    {
        for (int32 File = 0; File < SiegeBattle::DefenderFiles; ++File)
        {
            const float FileOffset =
                static_cast<float>(File) -
                (static_cast<float>(SiegeBattle::DefenderFiles) - 1.0f) * 0.5f;
            const FVector Position(
                FileOffset * SiegeBattle::DefenderFileSpacing,
                300.0f + static_cast<float>(Rank) * SiegeBattle::DefenderRankSpacing,
                0.0f);
            ASiegeAssetProxyActor* Soldier = SpawnCombatant(
                ESiegeAssetSlot::Infantry,
                ESiegeFaction::Defenders,
                Position,
                FRotator(0.0f, -90.0f, 0.0f),
                FVector(1.08f),
                140.0f,
                195.0f,
                18.0f,
                150.0f,
                FString::Printf(TEXT("Defender_Infantry_%03d"), SoldierIndex++));
            if (Soldier)
            {
                DefenderInfantry.Add(Soldier);
            }
        }
    }
}

ASiegeAssetProxyActor* ASiegeWorldDirector::SpawnAsset(
    const ESiegeAssetSlot Slot,
    const FVector& Location,
    const FRotator& Rotation,
    const FVector& Scale,
    const FString& Label)
{
    FActorSpawnParameters Parameters;
    Parameters.Owner = this;

    ASiegeAssetProxyActor* Actor = GetWorld()->SpawnActor<ASiegeAssetProxyActor>(
        ASiegeAssetProxyActor::StaticClass(),
        Location,
        Rotation,
        Parameters);
    if (Actor)
    {
        Actor->SetActorScale3D(Scale);
#if WITH_EDITOR
        Actor->SetActorLabel(Label);
#endif
        Actor->ConfigureAsset(Slot);
    }
    return Actor;
}

ASiegeAssetProxyActor* ASiegeWorldDirector::SpawnCombatant(
    const ESiegeAssetSlot Slot,
    const ESiegeFaction Faction,
    const FVector& Location,
    const FRotator& Rotation,
    const FVector& Scale,
    const float Health,
    const float MoveSpeed,
    const float AttackDamage,
    const float AttackRange,
    const FString& Label)
{
    ASiegeAssetProxyActor* Actor = SpawnAsset(Slot, Location, Rotation, Scale, Label);
    if (Actor)
    {
        Actor->InitializeCombatant(
            Faction,
            Health,
            MoveSpeed,
            AttackDamage,
            AttackRange);
    }
    return Actor;
}

void ASiegeWorldDirector::TickInfantryGroup(
    TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& Group,
    const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& EnemyGroup,
    const ESiegeFaction Faction,
    const float DeltaSeconds)
{
    for (const TWeakObjectPtr<ASiegeAssetProxyActor>& UnitPtr : Group)
    {
        ASiegeAssetProxyActor* Unit = UnitPtr.Get();
        if (!Unit || !Unit->IsCombatAlive())
        {
            continue;
        }

        Unit->AdvanceCombatTime(DeltaSeconds);
        ASiegeAssetProxyActor* Enemy = nullptr;
        if (Faction == ESiegeFaction::Defenders)
        {
            Enemy = FindNearestEnemy(
                Unit,
                AttackerInfantry,
                SiegeBattle::InfantryAwarenessRange);
        }
        if (!Enemy)
        {
            Enemy = FindNearestEnemy(
                Unit,
                EnemyGroup,
                SiegeBattle::InfantryAwarenessRange);
        }

        ASiegeAssetProxyActor* AttackTarget = Enemy;
        if (!AttackTarget &&
            Faction == ESiegeFaction::Attackers &&
            Gate.IsValid() &&
            Gate->IsCombatAlive())
        {
            AttackTarget = Gate.Get();
        }

        const FVector UnitLocation = Unit->GetActorLocation();
        FVector TargetLocation = Unit->GetCombatHomeLocation();
        if (AttackTarget)
        {
            TargetLocation = AttackTarget->GetActorLocation();
            if (AttackTarget == Gate.Get())
            {
                TargetLocation.Y -= SiegeBattle::GateApproachOffset;
                TargetLocation.X = FMath::Clamp(
                    Unit->GetCombatHomeLocation().X * 0.45f,
                    -260.0f,
                    260.0f);
            }
        }

        const float TargetDistance = FVector::Dist2D(UnitLocation, TargetLocation);
        const float EffectiveAttackRange =
            AttackTarget == Gate.Get() ? Unit->GetAttackRange() + 120.0f : Unit->GetAttackRange();
        if (AttackTarget &&
            AttackTarget->IsCombatAlive() &&
            TargetDistance <= EffectiveAttackRange)
        {
            Unit->SetCombatVelocity(FMath::VInterpTo(
                Unit->GetCombatVelocity(),
                FVector::ZeroVector,
                DeltaSeconds,
                8.0f));

            const FVector LookDirection = AttackTarget->GetActorLocation() - UnitLocation;
            if (!LookDirection.IsNearlyZero())
            {
                Unit->SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
            }

            if (Unit->GetAttackCooldownRemaining() <= 0.0f)
            {
                const float AppliedDamage =
                    AttackTarget == Gate.Get()
                        ? Unit->GetAttackDamage() * 0.18f
                        : Unit->GetAttackDamage();
                AttackTarget->ApplyCombatDamage(AppliedDamage, Unit);
                Unit->StartAttackCooldown(SiegeBattle::InfantryAttackCooldown);
                Unit->TriggerActionPulse();
            }
            continue;
        }

        const FVector DesiredVelocity = ComputeSwarmVelocity(Unit, Group, TargetLocation);
        FVector SmoothedVelocity = FMath::VInterpTo(
            Unit->GetCombatVelocity(),
            DesiredVelocity,
            DeltaSeconds,
            3.8f);
        SmoothedVelocity.Z = 0.0f;
        Unit->SetCombatVelocity(SmoothedVelocity);

        FVector NewLocation = UnitLocation + SmoothedVelocity * DeltaSeconds;
        NewLocation.X = FMath::Clamp(NewLocation.X, -3000.0f, 3000.0f);
        NewLocation.Y = FMath::Clamp(NewLocation.Y, -2600.0f, 1650.0f);
        NewLocation.Z = 0.0f;
        Unit->SetActorLocation(NewLocation);

        if (SmoothedVelocity.SizeSquared2D() > 100.0f)
        {
            Unit->SetActorRotation(FRotator(0.0f, SmoothedVelocity.Rotation().Yaw, 0.0f));
        }
    }
}

ASiegeAssetProxyActor* ASiegeWorldDirector::FindNearestEnemy(
    const ASiegeAssetProxyActor* Unit,
    const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& EnemyGroup,
    const float MaxDistance) const
{
    if (!Unit)
    {
        return nullptr;
    }

    ASiegeAssetProxyActor* Nearest = nullptr;
    float BestDistanceSquared = FMath::Square(MaxDistance);
    for (const TWeakObjectPtr<ASiegeAssetProxyActor>& EnemyPtr : EnemyGroup)
    {
        ASiegeAssetProxyActor* Enemy = EnemyPtr.Get();
        if (!Enemy || !Enemy->IsCombatAlive() || Enemy->GetFaction() == Unit->GetFaction())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            Unit->GetActorLocation(),
            Enemy->GetActorLocation());
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Nearest = Enemy;
        }
    }
    return Nearest;
}

FVector ASiegeWorldDirector::ComputeSwarmVelocity(
    const ASiegeAssetProxyActor* Unit,
    const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& Group,
    const FVector& TargetLocation) const
{
    const FVector Position = Unit->GetActorLocation();
    FVector TargetDirection = TargetLocation - Position;
    TargetDirection.Z = 0.0f;
    TargetDirection = TargetDirection.GetSafeNormal();

    FVector Separation = FVector::ZeroVector;
    FVector CohesionCenter = FVector::ZeroVector;
    FVector Alignment = FVector::ZeroVector;
    int32 CohesionCount = 0;

    for (const TWeakObjectPtr<ASiegeAssetProxyActor>& OtherPtr : Group)
    {
        const ASiegeAssetProxyActor* Other = OtherPtr.Get();
        if (!Other || Other == Unit || !Other->IsCombatAlive())
        {
            continue;
        }

        const FVector Offset = Position - Other->GetActorLocation();
        const float Distance = Offset.Size2D();
        if (Distance > KINDA_SMALL_NUMBER && Distance < SiegeBattle::SeparationRadius)
        {
            const float Strength = 1.0f - Distance / SiegeBattle::SeparationRadius;
            Separation += Offset.GetSafeNormal2D() * Strength;
        }

        if (Distance < SiegeBattle::CohesionRadius)
        {
            CohesionCenter += Other->GetActorLocation();
            Alignment += Other->GetCombatVelocity();
            ++CohesionCount;
        }
    }

    FVector Cohesion = FVector::ZeroVector;
    if (CohesionCount > 0)
    {
        CohesionCenter /= static_cast<float>(CohesionCount);
        Cohesion = (CohesionCenter - Position).GetSafeNormal2D();
        Alignment = (Alignment / static_cast<float>(CohesionCount)).GetSafeNormal2D();
    }

    FVector Steering =
        TargetDirection * 1.35f +
        Separation * 2.25f +
        Cohesion * 0.30f +
        Alignment * 0.24f;
    Steering.Z = 0.0f;
    return Steering.GetSafeNormal() * Unit->GetMoveSpeed();
}

void ASiegeWorldDirector::TickSiegeEngines(const float DeltaSeconds)
{
    if (ASiegeAssetProxyActor* Ram = BatteringRam.Get())
    {
        if (Ram->IsCombatAlive())
        {
            Ram->AdvanceCombatTime(DeltaSeconds);
            FVector TargetLocation(0.0f, 880.0f, 0.0f);
            if (Gate.IsValid() && Gate->IsCombatAlive())
            {
                TargetLocation = Gate->GetActorLocation() + FVector(0.0f, -365.0f, 0.0f);
                TargetLocation.Z = 0.0f;
            }
            else
            {
                TargetLocation.Y = 1550.0f;
            }

            const float Distance = FVector::Dist2D(Ram->GetActorLocation(), TargetLocation);
            if (Gate.IsValid() && Gate->IsCombatAlive() && Distance <= 95.0f)
            {
                Ram->SetCombatVelocity(FVector::ZeroVector);
                if (Ram->GetAttackCooldownRemaining() <= 0.0f)
                {
                    Gate->ApplyCombatDamage(Ram->GetAttackDamage(), Ram);
                    Ram->StartAttackCooldown(1.35f);
                    Ram->TriggerActionPulse();
                    UE_LOG(
                        LogTemp,
                        Display,
                        TEXT("IronwallSiegeCombat battering_ram_strike target=Gate"));
                }
            }
            else
            {
                const FVector Direction = (TargetLocation - Ram->GetActorLocation()).GetSafeNormal2D();
                const FVector Velocity = Direction * Ram->GetMoveSpeed();
                Ram->SetCombatVelocity(Velocity);
                Ram->SetActorLocation(Ram->GetActorLocation() + Velocity * DeltaSeconds);
                if (!Direction.IsNearlyZero())
                {
                    Ram->SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
                }
            }
        }
    }

    if (ASiegeAssetProxyActor* Engine = Trebuchet.Get())
    {
        if (Engine->IsCombatAlive())
        {
            Engine->AdvanceCombatTime(DeltaSeconds);
            if (Gate.IsValid() &&
                Gate->IsCombatAlive() &&
                Engine->GetAttackCooldownRemaining() <= 0.0f)
            {
                LaunchTrebuchetProjectile();
                Engine->StartAttackCooldown(4.4f);
                Engine->TriggerActionPulse();
            }
        }
    }
}

void ASiegeWorldDirector::LaunchTrebuchetProjectile()
{
    ASiegeAssetProxyActor* Engine = Trebuchet.Get();
    ASiegeAssetProxyActor* TargetGate = Gate.Get();
    if (!Engine || !TargetGate)
    {
        return;
    }

    const FVector Start = Engine->GetActorLocation() + FVector(230.0f, 0.0f, 760.0f);
    const FVector End = TargetGate->GetActorLocation() + FVector(
        FMath::FRandRange(-120.0f, 120.0f),
        -30.0f,
        330.0f);

    if (ASiegeProjectileActor* Projectile = GetWorld()->SpawnActor<ASiegeProjectileActor>(
        ASiegeProjectileActor::StaticClass(),
        Start,
        FRotator::ZeroRotator))
    {
        Projectile->Launch(
            Start,
            End,
            TargetGate,
            Engine,
            Engine->GetAttackDamage(),
            ESiegeFaction::Attackers,
            2.25f);
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeCombat trebuchet_fired target=Gate"));
    }
}

void ASiegeWorldDirector::CheckBattleOutcome()
{
    const int32 LivingAttackers = CountLiving(AttackerCombatants);
    const int32 LivingDefenders = CountLiving(DefenderInfantry);
    const bool bGateBreached = !Gate.IsValid() || !Gate->IsCombatAlive();

    if (LivingAttackers == 0)
    {
        bBattleResolved = true;
        SetActorTickEnabled(false);
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeBattle resolved winner=Defenders elapsed=%.1f"),
            BattleElapsedTime);
    }
    else if (bGateBreached && LivingDefenders == 0)
    {
        bBattleResolved = true;
        SetActorTickEnabled(false);
        UE_LOG(
            LogTemp,
            Display,
            TEXT("IronwallSiegeBattle resolved winner=Attackers elapsed=%.1f"),
            BattleElapsedTime);
    }
}

int32 ASiegeWorldDirector::CountLiving(
    const TArray<TWeakObjectPtr<ASiegeAssetProxyActor>>& Group) const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<ASiegeAssetProxyActor>& ActorPtr : Group)
    {
        const ASiegeAssetProxyActor* Actor = ActorPtr.Get();
        if (Actor && Actor->IsCombatAlive())
        {
            ++Count;
        }
    }
    return Count;
}

void ASiegeWorldDirector::HandleResourceSetChanged(FName ResourceSetId)
{
    (void)ResourceSetId;
    for (TActorIterator<ASiegeAssetProxyActor> It(GetWorld()); It; ++It)
    {
        It->RefreshVisual();
    }
}
