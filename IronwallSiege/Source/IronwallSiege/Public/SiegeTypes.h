#pragma once

#include "CoreMinimal.h"
#include "SiegeTypes.generated.h"

UENUM(BlueprintType)
enum class ESiegeAssetSlot : uint8
{
    Ground,
    Wall,
    Gate,
    Tower,
    Trebuchet,
    BatteringRam,
    Infantry
};

UENUM(BlueprintType)
enum class ESiegeResourceFlavor : uint8
{
    Prototype,
    BlenderProduction
};
