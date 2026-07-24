#include "SiegeArcherSpawner.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "SiegeArcherActor.h"

ASiegeArcherSpawner::ASiegeArcherSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASiegeArcherSpawner::BeginPlay()
{
    Super::BeginPlay();

    TActorIterator<ASiegeArcherActor> ExistingArcher(GetWorld());
    if (ExistingArcher)
    {
        return;
    }

    SpawnFormation(
        ESiegeFaction::Attackers,
        FVector(0.0f, -880.0f, 0.0f),
        2,
        6,
        235.0f,
        210.0f,
        false,
        TEXT("Attacker_Archer"));
    SpawnFormation(
        ESiegeFaction::Defenders,
        FVector(0.0f, 930.0f, 0.0f),
        2,
        6,
        235.0f,
        180.0f,
        true,
        TEXT("Defender_Archer"));

    UE_LOG(
        LogTemp,
        Display,
        TEXT("IronwallSiegeArchers spawned attackers=12 defenders=12"));
}

void ASiegeArcherSpawner::SpawnFormation(
    const ESiegeFaction Faction,
    const FVector& FormationOrigin,
    const int32 Rows,
    const int32 Columns,
    const float ColumnSpacing,
    const float RowSpacing,
    const bool bHoldPosition,
    const TCHAR* NamePrefix)
{
    const float HalfColumns = static_cast<float>(Columns - 1) * 0.5f;
    int32 UnitIndex = 0;
    for (int32 Row = 0; Row < Rows; ++Row)
    {
        for (int32 Column = 0; Column < Columns; ++Column)
        {
            FVector Location = FormationOrigin;
            Location.X += (static_cast<float>(Column) - HalfColumns) * ColumnSpacing;
            Location.Y += static_cast<float>(Row) * RowSpacing *
                (Faction == ESiegeFaction::Attackers ? -1.0f : 1.0f);

            FActorSpawnParameters Parameters;
            Parameters.Owner = this;
            Parameters.Name = FName(*FString::Printf(
                TEXT("%s_%02d"),
                NamePrefix,
                UnitIndex++));

            const FRotator Rotation(
                0.0f,
                Faction == ESiegeFaction::Attackers ? 90.0f : -90.0f,
                0.0f);
            if (ASiegeArcherActor* Archer = GetWorld()->SpawnActor<ASiegeArcherActor>(
                ASiegeArcherActor::StaticClass(),
                Location,
                Rotation,
                Parameters))
            {
                Archer->SetActorScale3D(FVector(1.02f));
                Archer->ConfigureArcher(Faction, bHoldPosition);
            }
        }
    }
}
