#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SiegeTypes.h"
#include "SiegeArcherSpawner.generated.h"

UCLASS()
class IRONWALLSIEGE_API ASiegeArcherSpawner : public AActor
{
    GENERATED_BODY()

public:
    ASiegeArcherSpawner();
    virtual void BeginPlay() override;

private:
    void SpawnFormation(
        ESiegeFaction Faction,
        const FVector& FormationOrigin,
        int32 Rows,
        int32 Columns,
        float ColumnSpacing,
        float RowSpacing,
        bool bHoldPosition,
        const TCHAR* NamePrefix);
};
