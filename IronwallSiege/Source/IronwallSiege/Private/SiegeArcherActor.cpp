#include "SiegeArcherActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "SiegeArrowProjectile.h"
#include "SiegeWorldDirector.h"

namespace SiegeArcher
{
    constexpr float AwarenessRange = 2050.0f;
    constexpr float AttackRange = 1500.0f;
    constexpr float PreferredMinimumRange = 420.0f;
    constexpr float AttackCooldown = 1.72f;
    constexpr float TargetScanInterval = 0.24f;
    constexpr float AttackerAdvanceLimitY = 410.0f;
    constexpr float ProjectileSpeed = 1550.0f;
}

ASiegeArcherActor::ASiegeArcherActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void ASiegeArcherActor::BeginPlay()
{
    Super::BeginPlay();

    for (TActorIterator<ASiegeWorldDirector> It(GetWorld()); It; ++It)
    {
        BattleDirector = *It;
        break;
    }

    SetActorTickEnabled(bConfigured);
}

void ASiegeArcherActor::ConfigureArcher(
    const ESiegeFaction NewFaction,
    const bool bNewHoldPosition)
{
    const bool bIsDefender = NewFaction == ESiegeFaction::Defenders;
    bHoldPosition = bNewHoldPosition;
    ConfigureAsset(ESiegeAssetSlot::Infantry);
    InitializeCombatant(
        NewFaction,
        bIsDefender ? 105.0f : 95.0f,
        bIsDefender ? 0.0f : 165.0f,
        bIsDefender ? 24.0f : 23.0f,
        SiegeArcher::AttackRange);
    TargetScanRemaining = FMath::FRandRange(0.0f, SiegeArcher::TargetScanInterval);
    bConfigured = true;
    SetActorTickEnabled(true);
}

void ASiegeArcherActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bConfigured || !IsCombatAlive() || !IsBattleActive())
    {
        return;
    }

    AdvanceCombatTime(DeltaSeconds);
    TargetScanRemaining -= DeltaSeconds;

    ASiegeAssetProxyActor* CurrentTarget = AttackTarget.Get();
    const bool bTargetInvalid =
        !CurrentTarget ||
        !CurrentTarget->IsCombatAlive() ||
        CurrentTarget->GetFaction() == GetFaction() ||
        FVector::DistSquared2D(GetActorLocation(), CurrentTarget->GetActorLocation()) >
            FMath::Square(SiegeArcher::AwarenessRange);
    if (bTargetInvalid || TargetScanRemaining <= 0.0f)
    {
        AttackTarget = FindNearestRangedTarget(SiegeArcher::AwarenessRange);
        CurrentTarget = AttackTarget.Get();
        TargetScanRemaining = SiegeArcher::TargetScanInterval;
    }

    const float Distance = CurrentTarget
        ? FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation())
        : BIG_NUMBER;
    if (CurrentTarget && Distance <= GetAttackRange())
    {
        SetCombatVelocity(FMath::VInterpTo(
            GetCombatVelocity(),
            FVector::ZeroVector,
            DeltaSeconds,
            7.0f));

        const FVector LookDirection = CurrentTarget->GetActorLocation() - GetActorLocation();
        if (!LookDirection.IsNearlyZero())
        {
            SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
        }

        if (GetAttackCooldownRemaining() <= 0.0f)
        {
            FireArrow(CurrentTarget);
            StartAttackCooldown(SiegeArcher::AttackCooldown);
            TriggerActionPulse();
        }
    }
    else
    {
        TickMovement(CurrentTarget, DeltaSeconds);
    }
}

ASiegeAssetProxyActor* ASiegeArcherActor::FindNearestRangedTarget(
    const float MaxDistance) const
{
    ASiegeAssetProxyActor* Nearest = nullptr;
    float BestDistanceSquared = FMath::Square(MaxDistance);
    for (TActorIterator<ASiegeAssetProxyActor> It(GetWorld()); It; ++It)
    {
        ASiegeAssetProxyActor* Candidate = *It;
        if (!Candidate ||
            Candidate == this ||
            !Candidate->IsCombatAlive() ||
            Candidate->GetFaction() == GetFaction() ||
            Candidate->GetFaction() == ESiegeFaction::Neutral ||
            Candidate->AssetSlot != ESiegeAssetSlot::Infantry)
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(
            GetActorLocation(),
            Candidate->GetActorLocation());
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Nearest = Candidate;
        }
    }
    return Nearest;
}

void ASiegeArcherActor::TickMovement(
    ASiegeAssetProxyActor* CurrentTarget,
    const float DeltaSeconds)
{
    if (bHoldPosition)
    {
        SetCombatVelocity(FVector::ZeroVector);
        return;
    }

    FVector DesiredDirection = FVector::YAxisVector;
    if (CurrentTarget)
    {
        DesiredDirection = CurrentTarget->GetActorLocation() - GetActorLocation();
        DesiredDirection.Z = 0.0f;

        if (DesiredDirection.SizeSquared2D() <
            FMath::Square(SiegeArcher::PreferredMinimumRange))
        {
            DesiredDirection *= -1.0f;
        }
    }
    else if (GetActorLocation().Y >= SiegeArcher::AttackerAdvanceLimitY)
    {
        DesiredDirection = FVector::ZeroVector;
    }

    const FVector DesiredVelocity =
        DesiredDirection.GetSafeNormal2D() * GetMoveSpeed();
    FVector SmoothedVelocity = FMath::VInterpTo(
        GetCombatVelocity(),
        DesiredVelocity,
        DeltaSeconds,
        4.2f);
    SmoothedVelocity.Z = 0.0f;
    SetCombatVelocity(SmoothedVelocity);

    FVector NewLocation = GetActorLocation() + SmoothedVelocity * DeltaSeconds;
    NewLocation.X = FMath::Clamp(NewLocation.X, -2200.0f, 2200.0f);
    NewLocation.Y = FMath::Clamp(NewLocation.Y, -2100.0f, SiegeArcher::AttackerAdvanceLimitY);
    NewLocation.Z = GetCombatHomeLocation().Z;
    SetActorLocation(NewLocation);

    if (SmoothedVelocity.SizeSquared2D() > 25.0f)
    {
        SetActorRotation(FRotator(0.0f, SmoothedVelocity.Rotation().Yaw, 0.0f));
    }
}

void ASiegeArcherActor::FireArrow(ASiegeAssetProxyActor* CurrentTarget)
{
    if (!CurrentTarget)
    {
        return;
    }

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 215.0f);
    const float Distance = FVector::Dist(Start, CurrentTarget->GetActorLocation());
    const float FlightDuration = FMath::Clamp(
        Distance / SiegeArcher::ProjectileSpeed,
        0.38f,
        1.10f);

    if (ASiegeArrowProjectile* Arrow = GetWorld()->SpawnActor<ASiegeArrowProjectile>(
        ASiegeArrowProjectile::StaticClass(),
        Start,
        GetActorRotation()))
    {
        const bool bIsFirstShot = !bLoggedFirstShot;
        Arrow->Launch(
            Start,
            CurrentTarget,
            this,
            GetAttackDamage(),
            GetFaction(),
            FlightDuration,
            bIsFirstShot);

        if (bIsFirstShot)
        {
            UE_LOG(
                LogTemp,
                Display,
                TEXT("IronwallSiegeArcher first_shot=%s faction=%s target=%s range=%.0f"),
                *GetName(),
                *StaticEnum<ESiegeFaction>()->GetNameStringByValue(
                    static_cast<int64>(GetFaction())),
                *CurrentTarget->GetName(),
                Distance);
            bLoggedFirstShot = true;
        }
    }
}

bool ASiegeArcherActor::IsBattleActive()
{
    if (!BattleDirector.IsValid())
    {
        for (TActorIterator<ASiegeWorldDirector> It(GetWorld()); It; ++It)
        {
            BattleDirector = *It;
            break;
        }
    }
    return BattleDirector.IsValid() && BattleDirector->IsBattleStarted();
}
