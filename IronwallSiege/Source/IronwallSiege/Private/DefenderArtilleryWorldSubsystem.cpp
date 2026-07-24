#include "DefenderArtilleryWorldSubsystem.h"

#include "Engine/World.h"
#include "SiegeDefenderArtilleryActor.h"

void UDefenderArtilleryWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    for (int32 BatteryIndex = -1; BatteryIndex <= 1; ++BatteryIndex)
    {
        ASiegeDefenderArtilleryActor* Battery =
            InWorld.SpawnActorDeferred<ASiegeDefenderArtilleryActor>(
                ASiegeDefenderArtilleryActor::StaticClass(),
                FTransform::Identity,
                nullptr,
                nullptr,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Battery)
        {
            continue;
        }

        Battery->InitializeBattery(BatteryIndex);
        Battery->FinishSpawning(FTransform::Identity);
#if WITH_EDITOR
        Battery->SetActorLabel(
            FString::Printf(TEXT("Defender_WallArtillery_%d"), BatteryIndex + 2));
#endif
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeArtillery spawned defender_wall_batteries=3"));
}
